#ifndef LUA_WIKI_PANEL_H
#define LUA_WIKI_PANEL_H

#include "MyGUI.h"
#include "HyperTextBox/HyperTextBox.h"

#include "OgreString.h"
#include <vector>

/**
 * @class LuaWikiPanel
 * @brief A MyGUI panel that shows a searchable, side-by-side Lua scripting
 *        wiki. Left side: ListBox with scenario names. Right side:
 *        HyperTextBox with formatted description + code snippets.
 *
 * Usage (from MainMenuBar::buttonHit or any render-thread callback):
 *     LuaWikiPanel::getInstance()->show();
 */
class LuaWikiPanel
{
public:
    static LuaWikiPanel* getInstance(void);

    /** Lazy-creates the window (first call) and makes it visible. */
    void show(void);

    /** Hides the window without destroying it. */
    void hide(void);

private:
    LuaWikiPanel();
    ~LuaWikiPanel() = default;

    // Non-copyable
    LuaWikiPanel(const LuaWikiPanel&) = delete;
    LuaWikiPanel& operator=(const LuaWikiPanel&) = delete;

    /** Creates all MyGUI widgets. Must run on the render thread. */
    void createWidgets(void);

    /** Fills this->entries with all wiki content. Called once at creation. */
    void buildEntries(void);

    // ── MyGUI callbacks (all fire on the render thread) ──────────────────
    void onEntrySelected(MyGUI::ListBox* sender, size_t index);
    void onCloseButton(MyGUI::Widget* sender);
    void onCopyButton(MyGUI::Widget* sender);
    void onMaximizeButton(MyGUI::Widget* sender);
    // Fired by WindowCS title-bar X button: name == "close"
    void onWindowButtonPressed(MyGUI::Window* sender, const std::string& name);

    /** Repositions the three bottom buttons centred in the current client area. */
    void repositionButtons(void);

    /** Strips HyperTextBox markup tags and returns plain readable text. */
    static Ogre::String stripMarkup(const Ogre::String& markup);

    // ── Data ──────────────────────────────────────────────────────────────
    struct WikiEntry
    {
        Ogre::String title;   ///< Shown in the ListBox
        Ogre::String content; ///< HyperTextBox-formatted markup
    };

    static LuaWikiPanel* sInstance;

    MyGUI::Window* wikiWindow = nullptr;
    MyGUI::ListBox* entryList = nullptr;
    MyGUI::HyperTextBox* contentBox = nullptr;
    MyGUI::Button* closeButton = nullptr;
    MyGUI::Button* copyButton = nullptr;
    MyGUI::Button* maxButton = nullptr;

    bool isMaximized = false;
    MyGUI::IntCoord savedCoord; // saved before maximize
    int chromeWidth = 0;        // outer - client, measured once
    int chromeHeight = 0;
    size_t currentEntryIndex = 0;

    std::vector<MyGUI::Widget*> widgetList;
    std::vector<WikiEntry> entries;
};

#endif // LUA_WIKI_PANEL_H
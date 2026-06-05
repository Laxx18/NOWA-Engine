#include "NOWAPrecompiled.h"
#include "LuaConsole.h"
#include "main/Events.h"
#include "main/AppStateManager.h"
#include "main/InputDeviceCore.h"
#include "modules/GraphicsModule.h"

#include <OgreOverlay.h>

namespace NOWA
{

#define CONSOLE_LINE_LENGTH 85
#define CONSOLE_LINE_COUNT 15
#define CONSOLE_MAX_LINES 32000
#define CONSOLE_MAX_HISTORY 64
#define CONSOLE_TAB_STOP 8

    LuaConsole::LuaConsole() : initialised(false), controlPressed(false), criticalLogCount(0)
    {
    }

    LuaConsole::~LuaConsole()
    {
        if (this->initialised)
        {
            this->shutdown();
        }
    }

    LuaConsole* LuaConsole::getSingletonPtr(void)
    {
        return msSingleton;
    }

    LuaConsole& LuaConsole::getSingleton(void)
    {
        assert(msSingleton);
        return (*msSingleton);
    }

    void LuaConsole::init(lua_State* lua)
    {
        if (this->initialised)
        {
            shutdown();
        }

        Ogre::v1::OverlayManager& overlayManager = Ogre::v1::OverlayManager::getSingleton();

        this->height = 1;
        this->startLine = 0;
        this->cursorBlinkTime = 0;
        this->cursorBlink = false;
        this->visible = false;

        this->pInterpreter = new LuaConsoleInterpreter(lua);
        this->print(this->pInterpreter->getOutput());
        this->pInterpreter->clearOutput();

        this->pTextbox = overlayManager.createOverlayElement("TextArea", "ConsoleText");
        this->pTextbox->setMetricsMode(Ogre::v1::GMM_RELATIVE);
        this->pTextbox->setPosition(0, 0);
        this->pTextbox->setParameter("font_name", "LuaConsole");
        this->pTextbox->setParameter("colour_top", "1 1 1");
        this->pTextbox->setParameter("colour_bottom", "1 1 1");
        this->pTextbox->setParameter("char_height", "0.03");

        this->pPanel = static_cast<Ogre::v1::OverlayContainer*>(overlayManager.createOverlayElement("Panel", "ConsolePanel"));
        this->pPanel->setMetricsMode(Ogre::v1::GMM_RELATIVE);
        this->pPanel->setPosition(0, 0);
        this->pPanel->setDimensions(1, 0);
        this->pPanel->setMaterialName("Materials/OverlayMaterial");

        this->pPanel->addChild(this->pTextbox);

        this->pOverlay = overlayManager.create("Console");
        this->pOverlay->add2D(this->pPanel);
        this->pOverlay->show();

        Ogre::LogManager::getSingleton().getDefaultLog()->addListener(this);

        this->initialised = true;
    }

    void LuaConsole::shutdown(void)
    {
        if (this->initialised)
        {
            delete this->pInterpreter;
            Ogre::LogManager::getSingleton().getDefaultLog()->removeListener(this);
        }
        this->initialised = false;
    }

    void LuaConsole::setVisible(bool visible)
    {
        this->visible = visible;
    }

    bool LuaConsole::isVisible(void)
    {
        return this->visible;
    }

    void LuaConsole::messageLogged(const Ogre::String& message, Ogre::LogMessageLevel lml, bool maskDebug, const Ogre::String& logName, bool& skipThisMessage)
    {
        // NOTE: Previously this line erroneously overwrote lml with LML_CRITICAL,
        // causing every log message to be treated as critical. Removed.
        this->print(message);

        // Deactivated, because performance issues
#if 0
        if (lml == Ogre::LML_CRITICAL)
        {
            if (this->criticalLogs.size() > CONSOLE_MAX_LINES - 1)
            {
                this->criticalLogs.pop_front();
            }
            this->criticalLogs.push_back(message);
            this->criticalLogCount++;

            // Relay the event to the main-thread event manager so MainMenuBar
            // (and any other listener) can react without polling.
            this->fireOgreLogEvent(message);
        }
#endif
    }

    void LuaConsole::fireOgreLogEvent(const Ogre::String& message)
    {
        // queueEvent is safe to call from any thread in NOWA's EventManager.
        // The event is processed during the next main-thread update tick.
        if (nullptr != NOWA::AppStateManager::getSingletonPtr())
        {
            boost::shared_ptr<EventDataPrintOgreLog> eventData(new EventDataPrintOgreLog(message, this->criticalLogCount));
            NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventData);
        }
    }

    // ---------- Critical log interface ----------

    unsigned int LuaConsole::getCriticalLogCount(void) const
    {
        return this->criticalLogCount;
    }

    const std::list<std::string>& LuaConsole::getCriticalLogs(void) const
    {
        return this->criticalLogs;
    }

    void LuaConsole::clearCriticalLogs(void)
    {
        this->criticalLogs.clear();
        this->criticalLogCount = 0;
    }

    // -------------------------------------------

    void LuaConsole::update(Ogre::Real dt)
    {
        if (this->visible)
        {
            this->cursorBlinkTime += dt;

            if (this->cursorBlinkTime > 0.5f)
            {
                this->cursorBlinkTime -= 0.5f;
                this->cursorBlink = !this->cursorBlink;
                this->textChanged = true;
            }

            if (InputDeviceCore::getSingletonPtr()->getKeyboard()->isKeyDown(OIS::KC_LCONTROL))
            {
                this->controlPressed = true;
            }
            if (InputDeviceCore::getSingletonPtr()->getKeyboard()->isKeyDown(OIS::KC_V) && this->controlPressed)
            {
                HANDLE clip;
                if (OpenClipboard(NULL))
                {
                    clip = GetClipboardData(CF_TEXT);
                    CloseClipboard();
                }

                this->clipText = (char*)clip;
                this->editLine.setText(this->clipText);
                this->textChanged = true;
            }
            this->controlPressed = false;

            this->editLine.updateKeyPress(InputDeviceCore::getSingletonPtr()->getKeyboard(), dt);
        }

        if (this->visible && this->height < 1.0f)
        {
            this->height += dt * 10.0f;

            auto pTextbox = this->pTextbox;
            auto pPanel = this->pPanel;
            ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("LuaConsole::update1", _2(pTextbox, pPanel), {
                pPanel->show();
                pTextbox->show();
            });

            NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->setMoveCameraWeight(0.0f);
            NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->setRotateCameraWeight(0.0f);

            if (this->height >= 1.0f)
            {
                this->height = 1.0f;
            }
        }
        else if (!this->visible && this->height > 0.0f)
        {
            this->height -= dt * 10.0f;
            if (this->height <= 0.0f)
            {
                this->height = 0.0f;
                auto pTextbox = this->pTextbox;
                auto pPanel = this->pPanel;
                ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("LuaConsole::update2", _2(pTextbox, pPanel), {
                    pPanel->hide();
                    pTextbox->hide();
                });
                NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->setMoveCameraWeight(1.0f);
                NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->setRotateCameraWeight(1.0f);
            }
        }

        if (visible)
        {
            auto pTextbox = this->pTextbox;
            auto pPanel = this->pPanel;
            Ogre::Real height = this->height;
            ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("LuaConsole::update3", _3(pTextbox, pPanel, height), {
                pTextbox->setPosition(0.0f, (height - 1.0f) * 0.5f);
                pPanel->setDimensions(1.0f, height * 0.5f);
            });
        }

        if (visible && this->textChanged)
        {
            Ogre::String text("");
            std::list<std::string>::iterator i;
            std::list<std::string>::iterator start;
            std::list<std::string>::iterator end;

            if (this->startLine < 0)
            {
                this->startLine = 0;
            }
            if ((unsigned)this->startLine > this->lines.size())
            {
                this->startLine = static_cast<unsigned int>(this->lines.size());
            }
            start = this->lines.begin();

            for (int c = 0; c < this->startLine; c++)
            {
                ++start;
            }
            end = start;

            for (int c = 0; c < CONSOLE_LINE_COUNT; c++)
            {
                if (end == lines.end())
                {
                    break;
                }
                ++end;
            }

            for (i = start; i != end; ++i)
            {
                text += (*i) + "\n";
            }

            std::string editLineText(this->editLine.getText() + " ");
            if (cursorBlink)
            {
                editLineText[this->editLine.getPosition()] = '_';
            }
            text += this->pInterpreter->getPrompt() + editLineText;
            auto pTextbox = this->pTextbox;
            ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("LuaConsole::update4", _2(pTextbox, text), {
                if (pTextbox)
                {
                    // Bad UTF-8 continuation byte may happen at any time, so catch it.
                    pTextbox->setCaption(text);
                }
            });

            this->textChanged = false;
        }
    }

    void LuaConsole::print(std::string text)
    {
        std::string line;
        std::string::iterator pos;
        int column;

        pos = text.begin();
        column = 1;

        while (pos != text.end())
        {
            if (*pos == '\n' || column > CONSOLE_LINE_LENGTH)
            {
                this->lines.push_back(line);
                line.clear();
                if (*pos != '\n')
                {
                    --pos;
                }
                column = 0;
            }
            else if (*pos == '\t')
            {
                line.push_back(' ');
                column++;

                while ((column % CONSOLE_TAB_STOP) != 0)
                {
                    line.push_back(' ');
                    column++;
                }
            }
            else
            {
                line.push_back(*pos);
                column++;
            }
            ++pos;
        }
        if (line.length())
        {
            if (this->lines.size() > CONSOLE_MAX_LINES - 1)
            {
                this->lines.pop_front();
            }
            this->lines.push_back(line);
        }

        if (this->lines.size() > CONSOLE_LINE_COUNT)
        {
            startLine = static_cast<unsigned int>(lines.size()) - CONSOLE_LINE_COUNT;
        }
        textChanged = true;

        return;
    }

    void LuaConsole::addToHistory(const std::string& cmd)
    {
        this->history.remove(cmd);
        this->history.push_back(cmd);

        if (this->history.size() > CONSOLE_MAX_HISTORY)
        {
            this->history.pop_front();
        }
        this->historyLine = this->history.end();
    }

    bool LuaConsole::injectKeyPress(const OIS::KeyEvent& evt)
    {
        switch (evt.key)
        {
        case OIS::KC_RETURN:
            print(this->pInterpreter->getPrompt() + this->editLine.getText());
            this->pInterpreter->insertLine(this->editLine.getText());
            this->addToHistory(this->editLine.getText());
            this->print(this->pInterpreter->getOutput());
            this->pInterpreter->clearOutput();
            this->editLine.clear();
            break;

        case OIS::KC_PGUP:
            this->startLine -= CONSOLE_LINE_COUNT;
            this->textChanged = true;
            break;

        case OIS::KC_PGDOWN:
            this->startLine += CONSOLE_LINE_COUNT;
            this->textChanged = true;
            break;

        case OIS::KC_UP:
            if (!this->history.empty())
            {
                if (this->historyLine == this->history.begin())
                {
                    this->historyLine = this->history.end();
                }
                this->historyLine--;
                this->editLine.setText(*this->historyLine);
                this->textChanged = true;
            }
            break;

        case OIS::KC_DOWN:
            if (!this->history.empty())
            {
                if (this->historyLine != history.end())
                {
                    this->historyLine++;
                }
                if (this->historyLine == this->history.end())
                {
                    this->historyLine = this->history.begin();
                }
                this->editLine.setText(*this->historyLine);
                this->textChanged = true;
            }
            break;
        default:
            this->textChanged = this->editLine.injectKeyPress(evt);
            break;
        }

        return true;
    }

    void LuaConsole::clearHistory()
    {
        this->history.clear();
    }

    void LuaConsole::clearConsole()
    {
        this->lines.clear();
        this->textChanged = true;
    }

}; // Namespace end
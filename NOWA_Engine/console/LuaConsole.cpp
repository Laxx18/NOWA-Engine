#include "NOWAPrecompiled.h"
#include "LuaConsole.h"
#include "main/AppStateManager.h"
#include "main/Events.h"
#include "main/InputDeviceCore.h"
#include "modules/GraphicsModule.h"
#include "utilities/MyGUIUtilities.h"

namespace NOWA
{

#define CONSOLE_LINE_LENGTH 400
#define CONSOLE_LINE_COUNT 30
#define CONSOLE_MAX_LINES 500
#define CONSOLE_MAX_HISTORY 64
#define CONSOLE_TAB_STOP 8

    static const Ogre::String CONSOLE_LAYER_NAME = "Statistic";

    // Slightly transparent black background, per spec.
    static const float CONSOLE_BACKGROUND_ALPHA = 0.9f;

    // No MyGUI font resource matching the old "LuaConsole" Ogre font was
    // confirmed to exist — falls back to whatever MyGUI's default font is,
    // sized via MyGUIUtilities (same mechanism MyGUITextComponent::setFontHeight
    // uses). Tune this constant to taste.
    static const unsigned int CONSOLE_FONT_HEIGHT_PX = 14;

    // Matches the original's implicit speed: height += dt * 10 reached 1.0 in
    // exactly 0.1s regardless of frame rate.
    static const float CONSOLE_ANIM_DURATION_SEC = 0.1f;

    namespace
    {
        // Open (fully visible) height in pixels: enough for CONSOLE_LINE_COUNT
        // lines plus the prompt line, at CONSOLE_FONT_HEIGHT_PX. The old
        // version hardcoded the panel height to always be exactly half the
        // viewport (t * 0.5f) — completely independent of CONSOLE_LINE_COUNT,
        // which is why raising the line count just crammed more text into
        // the same fixed box instead of growing it. 1.25x accounts for line
        // leading/spacing beyond the raw glyph height — tune if lines look
        // too cramped or too spaced.
        int consoleOpenHeightPx()
        {
            return static_cast<int>((CONSOLE_LINE_COUNT + 1) * (CONSOLE_FONT_HEIGHT_PX * 1.25f)) + 25;
        }

        // t=0 -> fully closed, t=1 -> fully open. Same two independent
        // formulas the original per-frame code used, now evaluated only at
        // the two endpoints instead of every frame. Render-thread only.
        MyGUI::IntCoord panelCoordAt(float t)
        {
            MyGUI::IntSize viewSize = MyGUI::RenderManager::getInstance().getViewSize();
            int height = static_cast<int>(consoleOpenHeightPx() * t);
            return MyGUI::IntCoord(0, 0, viewSize.width, height);
        }

        MyGUI::IntCoord textboxCoordAt(float t)
        {
            MyGUI::IntSize viewSize = MyGUI::RenderManager::getInstance().getViewSize();
            int openHeight = consoleOpenHeightPx();
            // Slides from -openHeight (closed, off-screen above) to 0 (open).
            int y = static_cast<int>((t - 1.0f) * openHeight);
            return MyGUI::IntCoord(0, y, viewSize.width, openHeight);
        }

        void linearMoveFunction(const MyGUI::IntCoord& _start, const MyGUI::IntCoord& _dest, MyGUI::IntCoord& _result, float _time)
        {
            _result.left = _start.left + static_cast<int>((_dest.left - _start.left) * _time);
            _result.top = _start.top + static_cast<int>((_dest.top - _start.top) * _time);
            _result.width = _start.width + static_cast<int>((_dest.width - _start.width) * _time);
            _result.height = _start.height + static_cast<int>((_dest.height - _start.height) * _time);
        }
    }

    LuaConsole::LuaConsole() : initialised(false), controlPressed(false), criticalLogCount(0), pPanel(nullptr), pTextbox(nullptr), pendingCameraWeightChange(false), pendingCameraWeightValue(1.0f)
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

        this->startLine = 0;
        this->cursorBlinkTime = 0;
        this->cursorBlink = false;
        this->visible = false;

        this->pInterpreter = new LuaConsoleInterpreter(lua);
        this->print(this->pInterpreter->getOutput());
        this->pInterpreter->clearOutput();

        // v2: two independent root-level MyGUI widgets (NOT parent/child)
        // replacing the old Ogre::v1::Overlay + OverlayContainer("Panel") +
        // OverlayElement("TextArea"). Created directly in the CLOSED state
        // (t=0) — see panelCoordAt()/textboxCoordAt() — since setVisible()
        // now drives the open/close animation via MyGUI::ControllerPosition
        // instead of update() polling a manually-lerped "height" value.
        NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
        {
            this->pPanel = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::Widget>("PanelSkin", 0.0f, 0.0f, 1.0f, 0.0f, MyGUI::Align::Default, CONSOLE_LAYER_NAME, "LuaConsole_Panel");
            this->pPanel->setColour(MyGUI::Colour(0.0f, 0.0f, 0.0f));
            this->pPanel->setAlpha(CONSOLE_BACKGROUND_ALPHA);
            this->pPanel->setNeedMouseFocus(false);
            this->pPanel->setNeedKeyFocus(false);
            this->pPanel->setVisible(false);

            this->pTextbox = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::EditBox>("EditBoxEmpty", 0.0f, -0.5f, 1.0f, 1.0f, MyGUI::Align::Default, CONSOLE_LAYER_NAME, "LuaConsole_Text");
            // Display surface only — injectKeyPress()/EditString remain the
            // sole input path, so MyGUI must never take keyboard focus here.
            this->pTextbox->setEditReadOnly(true);
            this->pTextbox->setEditMultiLine(true);
            this->pTextbox->setEditWordWrap(false);
            this->pTextbox->setTextColour(MyGUI::Colour::White);
            this->pTextbox->setNeedMouseFocus(false);
            this->pTextbox->setNeedKeyFocus(false);
            this->pTextbox->setVisible(false);

            MyGUIUtilities::getInstance()->setFontSize(this->pTextbox, CONSOLE_FONT_HEIGHT_PX);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "LuaConsole::init");

        Ogre::LogManager::getSingleton().getDefaultLog()->addListener(this);

        this->initialised = true;
    }

    void LuaConsole::shutdown(void)
    {
        if (this->initialised)
        {
            delete this->pInterpreter;
            Ogre::LogManager::getSingleton().getDefaultLog()->removeListener(this);

            // Defensive: in case shutdown() runs while the console was still
            // visible (closure still registered), don't leave it dangling.
            NOWA::GraphicsModule::getInstance()->removeTrackedClosure("LuaConsole::updateCaption");

            // NOTE: the original never explicitly destroyed pOverlay/pPanel/
            // pTextbox either — LuaConsole is a true app-lifetime singleton,
            // torn down only right before Ogre/MyGUI's own shutdown, so this
            // omission is preserved rather than "fixed".
        }
        this->initialised = false;
    }

    void LuaConsole::setVisible(bool visible)
    {
        if (this->visible == visible)
        {
            return;
        }
        this->visible = visible;

        if (true == visible)
        {
            // Applied on the logic thread by update() (see below) — matches
            // the original, which called these setters directly from
            // update(), not from a render-thread callback.
            this->pendingCameraWeightValue.store(0.0f);
            this->pendingCameraWeightChange.store(true);
        }
        else
        {
            NOWA::GraphicsModule::getInstance()->removeTrackedClosure("LuaConsole::updateCaption");
        }

        NOWA::GraphicsModule::RenderCommand renderCommand = [this, visible]()
        {
            // Unconditional remove + recreate — same pattern as
            // MyGUIPositionControllerComponent::onActivatedChanged.
            MyGUI::ControllerManager::getInstance().removeItem(this->pPanel);
            MyGUI::ControllerManager::getInstance().removeItem(this->pTextbox);

            if (true == visible)
            {
                this->pPanel->setVisible(true);
                this->pTextbox->setVisible(true);
            }

            float target = visible ? 1.0f : 0.0f;

            MyGUI::IntCoord panelStart = panelCoordAt(visible ? 0.0f : 1.0f);
            MyGUI::IntCoord textboxStart = textboxCoordAt(visible ? 0.0f : 1.0f);
            MyGUI::IntCoord panelDest = panelCoordAt(target);
            MyGUI::IntCoord textboxDest = textboxCoordAt(target);

            // Normalize to the correct opposite endpoint before animating —
            // don't trust the widgets' own getCoord() as the animation
            // start. If init() ran before MyGUI's RenderManager had a real
            // view size (getViewSize() still 0,0 at creation time), these
            // widgets could have been created with a stale/wrong pixel size
            // that was never corrected afterward, corrupting whatever
            // ControllerPosition::prepareItem() captures as mStartCoord.
            this->pPanel->setCoord(panelStart);
            this->pTextbox->setCoord(textboxStart);

            MyGUI::ControllerItem* panelItem = MyGUI::ControllerManager::getInstance().createItem(MyGUI::ControllerPosition::getClassTypeName());
            MyGUI::ControllerPosition* panelController = panelItem->castType<MyGUI::ControllerPosition>();
            panelController->setCoord(panelDest);
            panelController->setTime(CONSOLE_ANIM_DURATION_SEC);
            // setAction() is mandatory — see linearMoveFunction's comment
            // above. Without it, ControllerPosition applies an all-zero rect
            // every frame instead of interpolating.
            panelController->setAction(MyGUI::newDelegate(linearMoveFunction));
            MyGUI::ControllerManager::getInstance().addItem(this->pPanel, panelController);

            MyGUI::ControllerItem* textboxItem = MyGUI::ControllerManager::getInstance().createItem(MyGUI::ControllerPosition::getClassTypeName());
            MyGUI::ControllerPosition* textboxController = textboxItem->castType<MyGUI::ControllerPosition>();
            textboxController->setCoord(textboxDest);
            textboxController->setTime(CONSOLE_ANIM_DURATION_SEC);
            textboxController->setAction(MyGUI::newDelegate(linearMoveFunction));
            // Both controllers share the same duration — one completion
            // callback (on the textbox) is enough to know the slide is done.
            textboxController->eventPostAction += MyGUI::newDelegate(this, &LuaConsole::onSlideFinished);
            MyGUI::ControllerManager::getInstance().addItem(this->pTextbox, textboxController);
        };
        NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "LuaConsole::setVisible");
    }

    void LuaConsole::onSlideFinished(MyGUI::Widget* sender, MyGUI::ControllerItem* controller)
    {
        if (false == this->visible)
        {
            this->pPanel->setVisible(false);
            this->pTextbox->setVisible(false);

            this->pendingCameraWeightValue.store(1.0f);
            this->pendingCameraWeightChange.store(true);
        }
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

        // Applies whichever camera-weight change setVisible() (opening) or
        // onSlideFinished() (closing, render thread) requested. Keeps
        // CameraManager calls on the logic thread, matching the original.
        if (true == this->pendingCameraWeightChange.exchange(false))
        {
            float weight = this->pendingCameraWeightValue.load();
            NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->setMoveCameraWeight(weight);
            NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->setRotateCameraWeight(weight);
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

            // Tracked closures are only ever registered from update() (called
            // every logic frame) — updateTrackedClosure() overwrites the
            // existing entry for this id, so no removeTrackedClosure() is
            // needed here first. It only gets removed when the console
            // becomes hidden (see setVisible(false)) or is shut down.
            auto pTextbox = this->pTextbox;
            auto closureFunction = [pTextbox, text](Ogre::Real renderDt)
            {
                if (nullptr != pTextbox)
                {
                    // Real '\n' characters — EditBox is natively multi-line,
                    // this is not the setCaptionWithReplacing("\\n") escape
                    // trick used for XML/property-authored captions elsewhere.
                    pTextbox->setCaption(text);
                }
            };
            NOWA::GraphicsModule::getInstance()->updateTrackedClosure("LuaConsole::updateCaption", closureFunction, false);

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
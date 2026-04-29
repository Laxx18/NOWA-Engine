#include "NOWAPrecompiled.h"
#include "LuaWikiPanel.h"
#include "modules/GraphicsModule.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <cstring>

// ─────────────────────────────────────────────────────────────────────────────
// Content-building helpers  (anonymous namespace, .cpp only)
// ─────────────────────────────────────────────────────────────────────────────
namespace
{
    // Heading  (blue, h3)
    static Ogre::String wH(const Ogre::String& t)
    {
        return "<p><color value='#5BA3D9'><h3>" + t + "</h3></color></p><br/>";
    }
    // Normal paragraph (white)
    static Ogre::String wP(const Ogre::String& t)
    {
        return "<p><color value='#FFFFFF'>" + t + "</color></p><br/>";
    }
    // Code line (warm yellow) – one <p> per line keeps it readable
    static Ogre::String wC(const Ogre::String& t)
    {
        return "<p><color value='#F5C842'>" + t + "</color></p>";
    }
    // Tip / note (light green)
    static Ogre::String wTip(const Ogre::String& t)
    {
        return "<p><color value='#78D07A'><b>Tip: </b>" + t + "</color></p><br/>";
    }
    // Warning (soft red)
    static Ogre::String wWarn(const Ogre::String& t)
    {
        return "<p><color value='#E07070'><b>Warning: </b>" + t + "</color></p><br/>";
    }
    // Sub-section label (light purple)
    static Ogre::String wSub(const Ogre::String& t)
    {
        return "<p><color value='#C8A0FF'><b>" + t + "</b></color></p>";
    }
    // Blank line spacer
    static Ogre::String wBr()
    {
        return "<br/>";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Singleton
// ─────────────────────────────────────────────────────────────────────────────
LuaWikiPanel* LuaWikiPanel::sInstance = nullptr;

LuaWikiPanel* LuaWikiPanel::getInstance(void)
{
    if (nullptr == sInstance)
    {
        sInstance = new LuaWikiPanel();
    }
    return sInstance;
}

LuaWikiPanel::LuaWikiPanel()
{
    this->buildEntries();
}

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────
void LuaWikiPanel::show(void)
{
    NOWA::GraphicsModule::RenderCommand cmd = [this]()
    {
        if (nullptr == this->wikiWindow)
        {
            this->createWidgets();
        }

        this->wikiWindow->setVisible(true);
        // Show first entry by default if nothing selected yet
        if (this->entryList->getIndexSelected() == MyGUI::ITEM_NONE && !this->entries.empty())
        {
            this->entryList->setIndexSelected(0);
            this->contentBox->setCaption(this->entries[0].content);
        }
    };
    NOWA::GraphicsModule::getInstance()->enqueue(std::move(cmd), "LuaWikiPanel::show");
}

void LuaWikiPanel::hide(void)
{
    NOWA::GraphicsModule::RenderCommand cmd = [this]()
    {
        if (nullptr != this->wikiWindow)
        {
            this->wikiWindow->setVisible(false);
        }
    };
    NOWA::GraphicsModule::getInstance()->enqueue(std::move(cmd), "LuaWikiPanel::hide");
}

// ─────────────────────────────────────────────────────────────────────────────
// Widget creation  (runs on render thread)
// ─────────────────────────────────────────────────────────────────────────────
void LuaWikiPanel::createWidgets(void)
{
    // ── Main window ───────────────────────────────────────────────────────
    this->wikiWindow = MyGUI::Gui::getInstance().createWidget<MyGUI::Window>("WindowCS", MyGUI::IntCoord(10, 10, 1100, 660), MyGUI::Align::Default, "Popup");

    this->wikiWindow->setCaption("NOWA-Engine Lua Wiki");
    this->wikiWindow->setMinSize(600, 400);

    this->wikiWindow->eventWindowButtonPressed += MyGUI::newDelegate(this, &LuaWikiPanel::onWindowButtonPressed);

    MyGUI::IntCoord coord = this->wikiWindow->getClientCoord();

    const int listWidth = 280;
    const int gap = 4;
    const int btnH = 36;
    // Leave btnH + 10 px at the bottom for the button row
    const int contentH = coord.height - btnH - 10;

    // ── Left: entry list ──────────────────────────────────────────────────
    this->entryList = this->wikiWindow->createWidget<MyGUI::ListBox>("ListBox", MyGUI::IntCoord(0, 0, listWidth, contentH), MyGUI::Align::Left | MyGUI::Align::VStretch, "wikiEntryList");

    this->entryList->setInheritsAlpha(false);
    this->entryList->eventListChangePosition += MyGUI::newDelegate(this, &LuaWikiPanel::onEntrySelected);

    for (const auto& e : this->entries)
    {
        this->entryList->addItem(e.title);
    }

    // ── Right: content box ────────────────────────────────────────────────
    const int cboxX = listWidth + gap;
    this->contentBox = this->wikiWindow->createWidget<MyGUI::HyperTextBox>("HyperTextBox", MyGUI::IntCoord(cboxX, 0, coord.width - cboxX, contentH), MyGUI::Align::Stretch, "wikiContentBox");

    this->contentBox->setUrlColour(MyGUI::Colour::White);

    // ── Bottom buttons ────────────────────────────────────────────────────
    // Use Align::Default so MyGUI never auto-moves them.
    // repositionButtons() handles placement after every resize via
    // eventWindowChangeCoord.  We pass dummy coords (0,0,w,h) here.
    const int btnW = 110;

    this->maxButton = this->wikiWindow->createWidget<MyGUI::Button>("Button", MyGUI::IntCoord(0, 0, btnW, btnH), MyGUI::Align::Default, "wikiMaxButton");
    this->maxButton->setCaption("Maximize");
    this->maxButton->eventMouseButtonClick += MyGUI::newDelegate(this, &LuaWikiPanel::onMaximizeButton);

    this->copyButton = this->wikiWindow->createWidget<MyGUI::Button>("Button", MyGUI::IntCoord(0, 0, btnW, btnH), MyGUI::Align::Default, "wikiCopyButton");
    this->copyButton->setCaption("Copy Page");
    this->copyButton->eventMouseButtonClick += MyGUI::newDelegate(this, &LuaWikiPanel::onCopyButton);

    this->closeButton = this->wikiWindow->createWidget<MyGUI::Button>("Button", MyGUI::IntCoord(0, 0, btnW, btnH), MyGUI::Align::Default, "wikiCloseButton");
    this->closeButton->setCaption("Close");
    this->closeButton->eventMouseButtonClick += MyGUI::newDelegate(this, &LuaWikiPanel::onCloseButton);

    // Register widgets for tracking
    this->widgetList.push_back(this->wikiWindow);
    this->widgetList.push_back(this->entryList);
    this->widgetList.push_back(this->contentBox);
    this->widgetList.push_back(this->maxButton);
    this->widgetList.push_back(this->copyButton);
    this->widgetList.push_back(this->closeButton);

    // Force layout update (standard NOWA pattern)
    auto sz = this->wikiWindow->getSize();
    this->wikiWindow->setSize(sz.width - 1, sz.height - 1);
    this->wikiWindow->setSize(sz.width, sz.height);

    // Measure the window chrome ONCE (title bar + borders).
    // getSize() is always current; getClientCoord() may lag during events,
    // so we derive client dimensions as: getSize() - chrome instead.
    {
        MyGUI::IntSize outerSize = this->wikiWindow->getSize();
        MyGUI::IntCoord clientCoord = this->wikiWindow->getClientCoord();
        this->chromeWidth = outerSize.width - clientCoord.width;
        this->chromeHeight = outerSize.height - clientCoord.height;
    }

    // Initial button placement
    this->repositionButtons();

    // Centre window on screen
    MyGUI::FloatPoint pos;
    pos.left = 0.05f;
    pos.top = 0.05f;
    this->wikiWindow->setRealPosition(pos);
}

// ─────────────────────────────────────────────────────────────────────────────
// Callbacks
// ─────────────────────────────────────────────────────────────────────────────
void LuaWikiPanel::onEntrySelected(MyGUI::ListBox* /*sender*/, size_t index)
{
    NOWA::GraphicsModule::RenderCommand cmd = [this, index]()
    {
        if (index == MyGUI::ITEM_NONE || index >= this->entries.size())
        {
            return;
        }
        this->currentEntryIndex = index;
        this->contentBox->setCaption(this->entries[index].content);
    };
    NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "LuaWikiPanel::onEntrySelected");
}

void LuaWikiPanel::onCloseButton(MyGUI::Widget* /*sender*/)
{
    NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
    {
        this->wikiWindow->setVisible(false);
    };
    NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "LuaWikiPanel::onCloseButton");
}

void LuaWikiPanel::onWindowButtonPressed(MyGUI::Window* /*sender*/, const std::string& name)
{
    if (name == "close")
    {
        this->wikiWindow->setVisible(false);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Button repositioning  (called after every resize and at creation)
// ─────────────────────────────────────────────────────────────────────────────
void LuaWikiPanel::repositionButtons(void)
{
    if (nullptr == this->wikiWindow || nullptr == this->maxButton || nullptr == this->copyButton || nullptr == this->closeButton)
    {
        return;
    }

    // getClientCoord() can return stale values when called from
    // eventWindowChangeCoord (the event fires before the layout pass).
    // getSize() is always current, so we derive client dimensions from it.
    MyGUI::IntSize outer = this->wikiWindow->getSize();
    const int clientW = outer.width - this->chromeWidth;
    const int clientH = outer.height - this->chromeHeight;

    const int btnW = 110;
    const int btnH = 36;
    const int btnGap = 10;
    // Three buttons centred: [Maximize/Restore]  [Copy Page]  [Close]
    const int totalW = btnW * 3 + btnGap * 2;
    const int startX = (clientW - totalW) / 2;
    const int btnY = clientH - btnH - 4;

    this->maxButton->setCoord(startX, btnY, btnW, btnH);
    this->copyButton->setCoord(startX + (btnW + btnGap), btnY, btnW, btnH);
    this->closeButton->setCoord(startX + (btnW + btnGap) * 2, btnY, btnW, btnH);
}

// ─────────────────────────────────────────────────────────────────────────────
// Maximize / Restore
// ─────────────────────────────────────────────────────────────────────────────
void LuaWikiPanel::onMaximizeButton(MyGUI::Widget* /*sender*/)
{
    NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
    {
        if (!this->isMaximized)
        {
            // Save current coord, then fill the screen
            this->savedCoord = this->wikiWindow->getCoord();

            const MyGUI::IntSize& view = MyGUI::RenderManager::getInstance().getViewSize();
            this->wikiWindow->setCoord(0, 0, view.width, view.height);

            this->isMaximized = true;
            this->maxButton->setCaption("Restore");
        }
        else
        {
            // Restore to saved size and position
            this->wikiWindow->setCoord(this->savedCoord);
            this->isMaximized = false;
            this->maxButton->setCaption("Maximize");
        }
        // repositionButtons() is called automatically via eventWindowChangeCoord

        this->repositionButtons();
    };
    NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "LuaWikiPanel::onMaximizeButton");
}

// ─────────────────────────────────────────────────────────────────────────────
// Markup stripper
// ─────────────────────────────────────────────────────────────────────────────
/*static*/ Ogre::String LuaWikiPanel::stripMarkup(const Ogre::String& src)
{
    Ogre::String out;
    out.reserve(src.size());

    size_t i = 0;
    while (i < src.size())
    {
        if (src[i] == '<')
        {
            size_t end = src.find('>', i);
            if (end == Ogre::String::npos)
            {
                ++i;
                continue;
            }

            // Raw tag text between < and >
            Ogre::String tag = src.substr(i + 1, end - i - 1);
            Ogre::StringUtil::toLowerCase(tag);
            // Take only the tag name (stop at space or /)
            Ogre::String tagName = tag.substr(0, tag.find_first_of(" /\t"));

            if (tagName == "/p" || tagName == "br")
            {
                out += '\n';
            }
            // all other tags are silently dropped

            i = end + 1;
        }
        else
        {
            out += src[i++];
        }
    }

    // Collapse runs of more than two consecutive newlines into two
    Ogre::String result;
    result.reserve(out.size());
    int nlRun = 0;
    for (char ch : out)
    {
        if (ch == '\n')
        {
            if (++nlRun <= 2)
            {
                result += ch;
            }
        }
        else
        {
            nlRun = 0;
            result += ch;
        }
    }
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// Copy current page content to system clipboard
// ─────────────────────────────────────────────────────────────────────────────
void LuaWikiPanel::onCopyButton(MyGUI::Widget* /*sender*/)
{
    if (this->currentEntryIndex >= this->entries.size())
    {
        return;
    }

    Ogre::String plain = LuaWikiPanel::stripMarkup(this->entries[this->currentEntryIndex].content);

    if (plain.empty())
    {
        return;
    }

#ifdef _WIN32
    if (!::OpenClipboard(nullptr))
    {
        return;
    }

    ::EmptyClipboard();

    // CF_TEXT needs a null-terminated ANSI buffer.
    HGLOBAL hMem = ::GlobalAlloc(GMEM_MOVEABLE, plain.size() + 1);
    if (hMem)
    {
        char* dst = static_cast<char*>(::GlobalLock(hMem));
        std::memcpy(dst, plain.c_str(), plain.size() + 1);
        ::GlobalUnlock(hMem);
        ::SetClipboardData(CF_TEXT, hMem);
        // GlobalFree must NOT be called after SetClipboardData succeeds –
        // the OS now owns the memory.
    }
    ::CloseClipboard();
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
// Wiki content
// ─────────────────────────────────────────────────────────────────────────────
void LuaWikiPanel::buildEntries(void)
{
    // ── 1. Script Lifecycle ───────────────────────────────────────────────
    {
        Ogre::String c;
        c += wH("Script Lifecycle");
        c += wP("Every Lua script attached to a GameObject has three generated entry points. "
                "The module name must match the GameObject name exactly.");
        c += wBr();
        c += wSub("Skeleton");
        c += wC("module(\"MyObject_0\", package.seeall)");
        c += wC("MyObject_0 = {}");
        c += wBr();
        c += wC("MyObject_0[\"connect\"] = function(gameObject)");
        c += wC("    -- Simulation started: get components, register events");
        c += wC("end");
        c += wBr();
        c += wC("MyObject_0[\"disconnect\"] = function()");
        c += wC("    -- Simulation stopped");
        c += wC("    AppStateManager:getGameObjectController():undoAll()");
        c += wC("end");
        c += wBr();
        c += wC("MyObject_0[\"update\"] = function(dt)");
        c += wC("    -- Called every frame; dt = delta time in seconds");
        c += wC("end");
        c += wBr();
        c += wTip("undoAll() in disconnect() restores the scene to its pre-simulation state "
                  "when testing inside NOWA-Design. Always include it.");
        c += wWarn("The module name and table name must match the GameObject name. "
                   "A mismatch causes the script to silently not connect.");
        entries.push_back({"01 - Script Lifecycle", c});
    }

    // ── 2. Input Handling ─────────────────────────────────────────────────
    {
        Ogre::String c;
        c += wH("Input Handling");
        c += wP("Prefer the per-GameObject InputDeviceModule for multi-player safety. "
                "The global InputDeviceModule is only for single-player scenarios.");
        c += wBr();
        c += wSub("Per-object (recommended)");
        c += wC("inputDeviceModule = gameObject:getInputDeviceComponent():getInputDeviceModule()");
        c += wC("inputDeviceModule:setAnalogActionThreshold(0.15)  -- dead-zone for sticks");
        c += wBr();
        c += wSub("Digital actions");
        c += wC("if inputDeviceModule:isActionDown(NOWA_A_UP)    then ... end");
        c += wC("if inputDeviceModule:isActionDown(NOWA_A_DOWN)  then ... end");
        c += wC("if inputDeviceModule:isActionDown(NOWA_A_LEFT)  then ... end");
        c += wC("if inputDeviceModule:isActionDown(NOWA_A_RIGHT) then ... end");
        c += wC("if inputDeviceModule:isActionDown(NOWA_A_JUMP)  then ... end");
        c += wC("if inputDeviceModule:isActionDown(NOWA_A_ACTION)then ... end");
        c += wC("if inputDeviceModule:isActionDown(NOWA_A_COWER) then ... end");
        c += wBr();
        c += wSub("Analog axis  [-1 .. 1]");
        c += wC("local axis = inputDeviceModule:getSteerAxis()");
        c += wC("-- clamp + dead-zone:");
        c += wC("if math.abs(axis) < 0.10 then axis = 0.0 end");
        c += wBr();
        c += wSub("Timed press");
        c += wC("if inputDeviceModule:isActionDownAmount(NOWA_A_JUMP, dt, 0.2) then");
        c += wC("    -- held for at least 0.2 s");
        c += wC("end");
        c += wBr();
        c += wSub("Global (single-player only)");
        c += wC("if InputDeviceModule:isActionDown(NOWA_A_UP) then ... end");
        c += wTip("Call activatePlayerController() to route the correct input device "
                  "to the correct player object.");
        entries.push_back({"02 - Input Handling", c});
    }

    // ── 3. Physics - Active Component ────────────────────────────────────
    {
        Ogre::String c;
        c += wH("Physics – Active Component");
        c += wP("PhysicsActiveComponent drives dynamic rigid bodies. "
                "Use applyForce for continuous forces and applyImpulse for instant kicks.");
        c += wBr();
        c += wSub("Getting it");
        c += wC("phys = gameObject:getPhysicsActiveComponent()");
        c += wBr();
        c += wSub("Force / velocity");
        c += wC("phys:applyForce(Vector3(0, 100, 0))              -- continuous force");
        c += wC("-- applyImpulse: deltaVector + offsetPosition from body origin");
        c += wC("phys:applyImpulse(Vector3(0, 5, 0), Vector3.ZERO)  -- instant kick at centre");
        c += wC("phys:applyRequiredForceForVelocity(Vector3(5,0,0)) -- reach exact velocity");
        c += wC("phys:setVelocity(Vector3(0, 0, -10))             -- init/kinematic only");
        c += wC("local v = phys:getVelocity()");
        c += wBr();
        c += wSub("Rotation");
        c += wC("phys:applyOmegaForce(Vector3(0, 3, 0))           -- angular force");
        c += wC("phys:applyOmegaForceRotateTo(targetQuat, Vector3.UNIT_Y, 10)");
        c += wBr();
        c += wSub("Direction-based movement");
        c += wC("-- setDirectionVelocity: init/kinematic only (not during simulation)");
        c += wC("phys:setDirectionVelocity(speed)  -- set speed along own forward axis");
        c += wC("phys:setSpeed(5)                  -- speed used by AI components");
        c += wBr();
        c += wSub("Spatial bounds (e.g. 2-D space game)");
        c += wC("phys:setBounds(Vector3(-100, 0, -100), Vector3(100, 0, 100))");
        c += wBr();
        c += wSub("Misc");
        c += wC("phys:setCollidable(true / false)");
        c += wC("phys:translate(Vector3(0, -5, 0))  -- relative offset teleport");
        entries.push_back({"03 - Physics Active", c});
    }

    // ── 4. Physics Player Controller ─────────────────────────────────────
    {
        Ogre::String c;
        c += wH("Physics – Player Controller");
        c += wP("Provides character-style movement with built-in step-up, slope handling, "
                "and jump detection. Use move() every update frame.");
        c += wBr();
        c += wSub("Setup (connect)");
        c += wC("controller = gameObject:getPhysicsPlayerControllerComponent()");
        c += wC("-- or cast for intellisense:");
        c += wC("controller = AppStateManager:getGameObjectController()");
        c += wC("               :castPhysicsPlayerControllerComponent(controller)");
        c += wBr();
        c += wSub("Movement (update)");
        c += wC("-- move(forwardSpeed, sideSpeed, yawRadians)");
        c += wC("local rotQuat = Quaternion(Degree(rotation), Vector3.UNIT_Y)");
        c += wC("controller:move(moveSpeed, 0.0, rotQuat:getYaw(true))");
        c += wBr();
        c += wSub("Jump & ground detection");
        c += wC("controller:jump()");
        c += wC("-- isOnFloor / isInFreeFall are on PhysicsPlayerControllerComponent:");
        c += wC("if controller:isOnFloor()    then ... end  -- grounded");
        c += wC("if controller:isInFreeFall() then ... end  -- airborne");
        c += wC("-- For height above ground use PlayerControllerComponent.getHeight():");
        c += wC("local playerCtrl = gameObject:getPlayerControllerComponent()");
        c += wC("local h = playerCtrl:getHeight()  -- 500 when no ground below");
        c += wBr();
        c += wSub("Face a direction / target");
        c += wC("-- Slerp toward a world direction:");
        c += wC("local q = MathHelper:faceDirectionSlerp(");
        c += wC("    go:getOrientation(), Vector3.NEGATIVE_UNIT_Z,");
        c += wC("    go:getDefaultDirection(), dt, 1200)");
        c += wC("controller:move(0, 0, q:getYaw(true))");
        c += wBr();
        c += wSub("Friction (onContactFriction callback)");
        c += wC("MyScript[\"onContactFriction\"] = function(go0, go1, playerContact)");
        c += wC("    playerContact:setResultFriction(2)");
        c += wC("end");
        c += wTip("Always activate with AppStateManager:getGameObjectController()"
                  ":activatePlayerController(true, gameObject:getId(), true).");
        entries.push_back({"04 - Physics Player Controller", c});
    }

    // ── 5. Animation Blender V1 ───────────────────────────────────────────
    {
        Ogre::String c;
        c += wH("Animation Blender (V1 Entity)");
        c += wP("Used with AnimationComponent on classic Ogre v1 Entity meshes. "
                "Animation slot IDs use the AnimID enum. Blend transition constants "
                "live directly on AnimationBlender (BLEND_SWITCH, BLEND_WHILE_ANIMATING, BLEND_THEN_ANIMATE).");
        c += wBr();
        c += wSub("Setup");
        c += wC("anim = gameObject:getAnimationComponent():getAnimationBlender()");
        c += wC("anim:registerAnimation(AnimID.ANIM_IDLE_1,    \"idle-01\")");
        c += wC("anim:registerAnimation(AnimID.ANIM_WALK_NORTH,\"walk-01\")");
        c += wC("anim:registerAnimation(AnimID.ANIM_WALK_SOUTH,\"walk-back\")");
        c += wC("anim:registerAnimation(AnimID.ANIM_ATTACK_1,  \"attack-01\")");
        c += wC("anim:registerAnimation(AnimID.ANIM_DEAD_1,    \"death-01\")");
        c += wC("anim:init1(AnimID.ANIM_IDLE_1, true)  -- start looping");
        c += wBr();
        c += wSub("Blending  –  blend5(animId, transition, blendTime, loop)");
        c += wC("anim:blend5(AnimID.ANIM_WALK_NORTH,");
        c += wC("            AnimationBlender.BLEND_WHILE_ANIMATING, 0.2, true)");
        c += wC("anim:blend5(AnimID.ANIM_ATTACK_1,");
        c += wC("            AnimationBlender.BLEND_SWITCH, 0.05, false)");
        c += wBr();
        c += wSub("State queries");
        c += wC("if anim:isCompleted() then ... end              -- AnimationBlender method");
        c += wC("if gameObject:getAnimationComponent():isComplete() then ... end  -- AnimationComponent method");
        c += wC("if anim:isAnimationActive(AnimID.ANIM_IDLE_1) then ... end");
        c += wBr();
        c += wSub("Manual advance (needed when auto-update is off)");
        c += wC("anim:addTime(dt * 2 / anim:getLength())");
        entries.push_back({"05 - Animation Blender V1", c});
    }

    // ── 6. Animation Blender V2 ───────────────────────────────────────────
    {
        Ogre::String c;
        c += wH("Animation Blender (V2 Item)");
        c += wP("Used with AnimationComponentV2 on Ogre-Next V2 Item meshes. "
                "Uses AnimID for slot constants and AnimationBlender.BLEND_* for transition constants.");
        c += wBr();
        c += wSub("Setup");
        c += wC("anim = gameObject:getAnimationComponentV2():getAnimationBlender()");
        c += wC("anim:registerAnimation(AnimID.ANIM_IDLE_1,    \"Idle\")");
        c += wC("anim:registerAnimation(AnimID.ANIM_WALK_NORTH,\"Walk\")");
        c += wC("anim:registerAnimation(AnimID.ANIM_JUMP_START,\"Jump2\")");
        c += wC("anim:registerAnimation(AnimID.ANIM_RUN,       \"Run\")");
        c += wC("anim:init1(AnimID.ANIM_IDLE_1, true)");
        c += wBr();
        c += wSub("Blending  –  blend5(animId, transition, duration, loop)");
        c += wC("anim:blend5(AnimID.ANIM_WALK_NORTH,");
        c += wC("            AnimationBlender.BLEND_WHILE_ANIMATING, 0.2, true)");
        c += wBr();
        c += wSub("Exclusive blend  –  blendExclusive5(animId, transition, duration)");
        c += wC("-- Note: blendExclusive5 takes 3 args only – NO loop parameter!");
        c += wC("anim:blendExclusive5(AnimID.ANIM_JUMP_START,");
        c += wC("                     AnimationBlender.BLEND_SWITCH, 0.1)");
        c += wBr();
        c += wSub("Via PlayerControllerComponent");
        c += wC("playerController = gameObject:getPlayerControllerComponent()");
        c += wC("anim = playerController:getAnimationBlender()");
        c += wC("playerController:reactOnAnimationFinished(function()");
        c += wC("    anim:blend5(AnimID.ANIM_IDLE_1,");
        c += wC("                AnimationBlender.BLEND_WHILE_ANIMATING, 0.1, true)");
        c += wC("end, true)  -- true = one-shot");
        c += wBr();
        c += wTip("Always call addTime(dt * speed / anim:getLength()) in update() "
                  "when you advance animations manually.");
        entries.push_back({"06 - Animation Blender V2", c});
    }

    // ── 7. AI State Machine ───────────────────────────────────────────────
    {
        Ogre::String c;
        c += wH("AI – State Machine (AiLuaComponent)");
        c += wP("A classic Finite State Machine. Each state is a Lua table with "
                "enter / execute / exit functions. Use changeState() to transition.");
        c += wBr();
        c += wSub("Connect");
        c += wC("aiLua = gameObject:getAiLuaComponent()");
        c += wC("aiLua:getMovingBehavior():setBehavior(BehaviorType.STOP)");
        c += wBr();
        c += wSub("State definition");
        c += wC("PatrolState = {}");
        c += wC("PatrolState[\"enter\"] = function(gameObject)");
        c += wC("    aiLua:getMovingBehavior():setBehavior(BehaviorType.FOLLOW_PATH_2D)");
        c += wC("end");
        c += wC("PatrolState[\"execute\"] = function(gameObject, dt)");
        c += wC("    local dist = gameObject:getPosition()");
        c += wC("                    :squaredDistance(target:getPosition())");
        c += wC("    if dist < 4 then aiLua:changeState(AttackState) end");
        c += wC("end");
        c += wC("PatrolState[\"exit\"] = function(gameObject)");
        c += wC("    -- cleanup on leaving this state");
        c += wC("end");
        c += wBr();
        c += wSub("Global state (always active in background)");
        c += wC("aiLua:setGlobalState(ShootState)  -- fires execute() alongside current state");
        c += wBr();
        c += wTip("A global state is useful for things like shooting that must run "
                  "regardless of the main state (patrol, attack, etc.).");
        entries.push_back({"07 - AI State Machine", c});
    }

    // ── 8. AI Goal System ─────────────────────────────────────────────────
    {
        Ogre::String c;
        c += wH("AI – Goal System (AiLuaGoalComponent)");
        c += wP("Hierarchical goal-oriented AI. A composite root goal contains sub-goals "
                "that execute sequentially (FIFO). Each sub-goal reports its status "
                "via a GoalResult pointer passed as a parameter — NOT via a return value.");
        c += wBr();
        c += wSub("Key rules");
        c += wC("-- 1. Do NOT call setRootGoal / addSubGoal from connect().");
        c += wC("--    The C++ finds the root goal table by name automatically.");
        c += wC("-- 2. The RootGoalName attribute in NOWA-Design must match");
        c += wC("--    the Lua table name exactly (e.g. 'MyRootGoal').");
        c += wC("-- 3. Sub-goals execute in the ORDER they are added (FIFO).");
        c += wC("-- 4. Status is set via goalResult:setStatus(), NOT via return value.");
        c += wBr();
        c += wSub("connect() – only MovingBehavior setup");
        c += wC("aiGoal = gameObject:getAiLuaGoalComponent()");
        c += wC("aiGoal:getMovingBehavior():setBehavior(BehaviorType.STOP)");
        c += wC("aiGoal:getMovingBehavior():setGoalRadius(1.5)");
        c += wBr();
        c += wSub("Root goal (table name = RootGoalName attribute)");
        c += wC("MyRootGoal = {}");
        c += wC("MyRootGoal[\"activate\"] = function(gameObject, goalResult)");
        c += wC("    goalResult = AppStateManager:getGameObjectController()");
        c += wC("                     :castGoalResult(goalResult)");
        c += wC("    -- Add sub-goals in execution order (pass Lua TABLES, not LuaGoal objects)");
        c += wC("    aiGoal:addSubGoal(GoalA)   -- runs first");
        c += wC("    aiGoal:addSubGoal(GoalB)   -- runs second");
        c += wC("end");
        c += wC("MyRootGoal[\"terminate\"] = function(gameObject, goalResult)");
        c += wC("    goalResult = AppStateManager:getGameObjectController()");
        c += wC("                     :castGoalResult(goalResult)");
        c += wC("    aiGoal:getMovingBehavior():setBehavior(BehaviorType.STOP)");
        c += wC("end");
        c += wBr();
        c += wSub("Atomic sub-goal (all 3 callbacks required)");
        c += wC("GoalA = {}");
        c += wC("GoalA[\"activate\"] = function(gameObject, goalResult)");
        c += wC("    goalResult = AppStateManager:getGameObjectController()");
        c += wC("                     :castGoalResult(goalResult)");
        c += wC("    goalResult:setStatus(GoalResult.ACTIVE)");
        c += wC("    -- set up movement, timers etc.");
        c += wC("end");
        c += wBr();
        c += wC("GoalA[\"process\"] = function(gameObject, dt, goalResult)  -- 3 args!");
        c += wC("    goalResult = AppStateManager:getGameObjectController()");
        c += wC("                     :castGoalResult(goalResult)");
        c += wC("    if arrived then");
        c += wC("        goalResult:setStatus(GoalResult.COMPLETED)  -- hand off to GoalB");
        c += wC("    else");
        c += wC("        goalResult:setStatus(GoalResult.ACTIVE)");
        c += wC("    end");
        c += wC("    -- do NOT return anything – status is set via goalResult");
        c += wC("end");
        c += wBr();
        c += wC("GoalA[\"terminate\"] = function(gameObject, goalResult)");
        c += wC("    goalResult = AppStateManager:getGameObjectController()");
        c += wC("                     :castGoalResult(goalResult)");
        c += wC("    -- cleanup (stop movement, reset flags)");
        c += wC("end");
        c += wBr();
        c += wSub("GoalResult status constants");
        c += wC("GoalResult.ACTIVE     -- goal is running");
        c += wC("GoalResult.COMPLETED  -- goal done, next sub-goal starts");
        c += wC("GoalResult.FAILED     -- goal failed, removed from list");
        c += wC("GoalResult.INACTIVE   -- not yet started (default)");
        c += wBr();
        c += wSub("Cycling goals (restart when all sub-goals complete)");
        c += wC("-- In the last sub-goal's terminate(), re-add the goals:");
        c += wC("LastGoal[\"terminate\"] = function(gameObject, goalResult)");
        c += wC("    goalResult = AppStateManager:getGameObjectController()");
        c += wC("                     :castGoalResult(goalResult)");
        c += wC("    aiGoal:addSubGoal(GoalA)   -- restart cycle");
        c += wC("    aiGoal:addSubGoal(GoalB)");
        c += wC("end");
        c += wBr();
        c += wWarn("process() receives 3 args: (gameObject, dt, goalResult). "
                   "The return value is IGNORED by C++. Always use goalResult:setStatus().");
        c += wTip("addSubGoal() takes the Lua TABLE (e.g. GoalA), "
                  "NOT a LuaGoal() C++ object. The C++ creates the wrapper internally.");
        entries.push_back({"08 - AI Goal System", c});
    }

    // ── 9. AI Moving Behavior ─────────────────────────────────────────────
    {
        Ogre::String c;
        c += wH("AI – Moving Behavior");
        c += wP("Configure steering behaviors on AiLuaComponent or AiLuaGoalComponent. "
                "STOP forces the agent to zero. NONE lets other forces act freely.");
        c += wBr();
        c += wSub("Common behaviors");
        c += wC("b = aiLua:getMovingBehavior()");
        c += wC("b:setBehavior(BehaviorType.STOP)           -- halt, zero velocity");
        c += wC("b:setBehavior(BehaviorType.NONE)           -- no force, others can still move");
        c += wC("b:setBehavior(BehaviorType.SEEK)           -- head toward target");
        c += wC("b:setBehavior(BehaviorType.PURSUIT)        -- predict & intercept target");
        c += wC("b:setBehavior(BehaviorType.FLEE)           -- move away from target");
        c += wC("b:setBehavior(BehaviorType.HIDE)           -- hide behind obstacles");
        c += wC("b:setBehavior(BehaviorType.WANDER)         -- random walk (3-D)");
        c += wC("b:setBehavior(BehaviorType.WANDER_2D)      -- random walk (2-D)");
        c += wC("b:setBehavior(BehaviorType.FOLLOW_PATH)    -- follow NavMesh path");
        c += wC("b:setBehavior(BehaviorType.FOLLOW_PATH_2D) -- follow 2-D waypoints");
        c += wBr();
        c += wSub("Configuration");
        c += wC("b:setGoalRadius(1.5)                       -- arrival tolerance");
        c += wC("b:setTargetAgentId(enemy:getId())          -- seek/pursuit target");
        c += wC("b:setAutoOrientation(true)                 -- face direction of travel");
        c += wC("b:setAutoAnimation(true)                   -- drive animation from velocity");
        c += wC("b:setWanderRadius(6)");
        c += wC("b:setActualizePathDelaySec(0.1)          -- re-plan path every 0.1 s");
        c += wBr();
        c += wSub("Path-goal reached callback");
        c += wC("aiLua:reactOnPathGoalReached(function()");
        c += wC("    aiLua:changeState(IdleState)");
        c += wC("end)");
        c += wBr();
        c += wSub("Obstacle hide");
        c += wC("b:setBehavior(BehaviorType.HIDE)");
        c += wC("b:setObstacleHideData(\"Obstacle\", 2)  -- category, extra radius");
        entries.push_back({"09 - AI Moving Behavior", c});
    }

    // ── 10. Area of Interest ─────────────────────────────────────────────
    {
        Ogre::String c;
        c += wH("Area of Interest");
        c += wP("AreaOfInterestComponent fires Lua closures when another physics "
                "body enters or leaves its volume. Great for proximity checks.");
        c += wBr();
        c += wSub("Setup");
        c += wC("aoi = gameObject:getAreaOfInterestComponent()");
        c += wC("-- by name if multiple on the same object:");
        c += wC("aoi = gameObject:getAreaOfInterestComponentFromName(\"ScanArea\")");
        c += wBr();
        c += wSub("Callbacks");
        c += wC("aoi:reactOnEnter(function(otherGameObject)");
        c += wC("    otherGameObject = AppStateManager:getGameObjectController()");
        c += wC("                         :castGameObject(otherGameObject)");
        c += wC("    log(\"Entered: \" .. otherGameObject:getName())");
        c += wC("end)");
        c += wBr();
        c += wC("aoi:reactOnLeave(function(otherGameObject)");
        c += wC("    otherGameObject = AppStateManager:getGameObjectController()");
        c += wC("                         :castGameObject(otherGameObject)");
        c += wC("    log(\"Left: \" .. otherGameObject:getName())");
        c += wC("end)");
        c += wBr();
        c += wTip("You can enable/disable the component at runtime with "
                  "aoi:setActivated(true/false) to save performance.");
        entries.push_back({"10 - Area of Interest", c});
    }

    // ── 11. Physics Trigger ───────────────────────────────────────────────
    {
        Ogre::String c;
        c += wH("Physics Trigger");
        c += wP("PhysicsTriggerComponent is a pure overlap volume (no collision response). "
                "Use reactOn* closures in connect(), or define named callbacks.");
        c += wBr();
        c += wSub("Closure style (connect)");
        c += wC("trigger = gameObject:getPhysicsTriggerComponent()");
        c += wC("trigger:reactOnEnter(function(visitorGameObject)");
        c += wC("    visitorGameObject = AppStateManager:getGameObjectController()");
        c += wC("                            :castGameObject(visitorGameObject)");
        c += wC("    AppStateManager:getGameObjectController()");
        c += wC("        :deleteDelayedGameObject(visitorGameObject:getId(), 1)");
        c += wC("end)");
        c += wBr();
        c += wSub("Named callback style");
        c += wC("MyScript[\"onTriggerEnter\"] = function(visitorGameObject)");
        c += wC("    -- called when a physics body enters the trigger");
        c += wC("end");
        c += wC("MyScript[\"onTriggerInside\"] = function(visitorGameObject)");
        c += wC("    -- called every frame while body overlaps");
        c += wC("end");
        c += wC("MyScript[\"onTriggerLeave\"] = function(visitorGameObject)");
        c += wC("    -- called when the body exits");
        c += wC("end");
        c += wBr();
        c += wTip("Triggers have no physics response – they are overlap-only. "
                  "Use PhysicsActiveComponent contact callbacks for collision response.");
        entries.push_back({"11 - Physics Trigger", c});
    }

    // ── 12. Inventory & Item Box ──────────────────────────────────────────
    {
        Ogre::String c;
        c += wH("Inventory & Item Box");
        c += wP("MyGUIItemBoxComponent is a drag-and-drop inventory. Items are identified "
                "by resource name (string). Quantities are integer stacks.");
        c += wBr();
        c += wSub("Activate");
        c += wC("itemBox = gameObject:getMyGUIItemBoxComponent()");
        c += wC("itemBox:setActivated(true)");
        c += wBr();
        c += wSub("Drop request – decide if drop is allowed");
        c += wC("itemBox:reactOnDropItemRequest(function(dragDropData)");
        c += wC("    if dragDropData:getSenderReceiverIsSame() then");
        c += wC("        dragDropData:setCanDrop(true)   -- allow internal reorder");
        c += wC("    elseif dragDropData:getSenderInventoryId() == shopId then");
        c += wC("        dragDropData:setCanDrop(true)   -- allow from shop");
        c += wC("    else");
        c += wC("        dragDropData:setCanDrop(false)  -- reject everything else");
        c += wC("    end");
        c += wC("end)");
        c += wBr();
        c += wSub("Drop accepted – execute transfer / purchase");
        c += wC("itemBox:reactOnDropItemAccepted(function(dragDropData)");
        c += wC("    local coins = itemBox:getQuantity(\"CoinItem\")");
        c += wC("    local price = dragDropData:getBuyValue()");
        c += wC("    if coins >= price then");
        c += wC("        itemBox:removeQuantity(\"CoinItem\", price)");
        c += wC("    end");
        c += wC("end)");
        c += wBr();
        c += wSub("Add items from world (contact callback)");
        c += wC("local inv = pickedItem:getInventoryItemComponent()");
        c += wC("inv:addQuantityToInventory(player:getId(), 5, true)");
        c += wC("AppStateManager:getGameObjectController():deleteGameObject(pickedItem:getId())");
        c += wBr();
        c += wSub("Modify quantities programmatically");
        c += wC("itemBox:increaseQuantity(\"CoinItem\", amount)");
        c += wC("itemBox:decreaseQuantity(\"SwordItem\", 1)");
        c += wC("itemBox:removeQuantity(\"PotionItem\", 1)");
        entries.push_back({"12 - Inventory & Item Box", c});
    }

    // ── 13. Sound ─────────────────────────────────────────────────────────
    {
        Ogre::String c;
        c += wH("Sound");
        c += wP("SimpleSoundComponent plays a single audio file. A GameObject can have "
                "multiple sound components accessed by index or name.");
        c += wBr();
        c += wSub("Get component");
        c += wC("snd = gameObject:getSimpleSoundComponent()           -- first one");
        c += wC("snd = gameObject:getSimpleSoundComponentFromIndex(1) -- by index");
        c += wC("snd = gameObject:getSimpleSoundComponentFromName(\"EngineSound\")");
        c += wBr();
        c += wSub("Basic control");
        c += wC("snd:setActivated(true)    -- play / resume");
        c += wC("snd:setActivated(false)   -- stop");
        c += wC("snd:setVolume(100)        -- range 0..200 (100 = default)");
        c += wC("snd:setPitch(1.0)         -- 1.0 = normal speed");
        c += wBr();
        c += wSub("Tie pitch to physics (engine RPM feel)");
        c += wC("local motion = physComp:getVelocity():squaredLength() + 40");
        c += wC("if motion > 40.04 then");
        c += wC("    snd:setActivated(true)");
        c += wC("    snd:setPitch(motion * 0.01)");
        c += wC("end");
        c += wBr();
        c += wSub("Background music");
        c += wC("bgMusic = gameObject:getSimpleSoundComponent()");
        c += wC("bgMusic:setActivated(false)  -- start muted");
        c += wC("OgreALModule:setContinue(true)  -- keep audio running across states");
        entries.push_back({"13 - Sound", c});
    }

    // ── 14. Spectrum Analysis ─────────────────────────────────────────────
    {
        Ogre::String c;
        c += wH("Spectrum Analysis");
        c += wP("Performs FFT on an audio stream and triggers onSpectrumAnalysis() each "
                "analysis frame. Useful for music-reactive visuals and beat detection.");
        c += wBr();
        c += wSub("Enable in connect()");
        c += wC("sound:enableSpectrumAnalysis(");
        c += wC("    true,                          -- enabled");
        c += wC("    1024,                          -- processing size (internally doubled)");
        c += wC("    60,                            -- number of bands");
        c += wC("    MathWindows.HANNING,           -- window function");
        c += wC("    SpectrumPreparationType.LINEAR,");
        c += wC("    0.3)                           -- smoothFactor 0.0..1.0");
        c += wBr();
        c += wSub("Analysis callback");
        c += wC("MyScript[\"onSpectrumAnalysis\"] = function()");
        c += wC("    local levelData  = sound:getLevelData()");
        c += wC("    local size       = sound:getActualSpectrumSize()");
        c += wBr();
        c += wC("    -- Beat detection:");
        c += wC("    if sound:isSpectrumArea(SpectrumArea.KICK_DRUM) then");
        c += wC("        local intensity = sound:getSpectrumAreaIntensity(SpectrumArea.KICK_DRUM)");
        c += wC("    end");
        c += wBr();
        c += wC("    -- Visualize bars with RectangleComponent:");
        c += wC("    for i = 0, size - 1 do");
        c += wC("        local val = math.sqrt(math.max(0, levelData[i] or 0))");
        c += wC("        lines:setHeight(i, 0.05 + val * 8)");
        c += wC("    end");
        c += wC("end");
        c += wBr();
        c += wSub("Spectrum areas");
        c += wC("SpectrumArea.KICK_DRUM   SpectrumArea.SNARE_DRUM  SpectrumArea.DEEP_BASS");
        c += wC("SpectrumArea.LOWER_MIDRANGE              SpectrumArea.UPPER_MIDRANGE");
        c += wC("SpectrumArea.PRESENCE_RANGE              SpectrumArea.HI_HAT");
        entries.push_back({"14 - Spectrum Analysis", c});
    }

    // ── 15. Custom Events ─────────────────────────────────────────────────
    {
        Ogre::String c;
        c += wH("Custom Events (ScriptEventManager)");
        c += wP("A type-safe publish/subscribe system for Lua-to-Lua communication "
                "across scripts and GameObjects.");
        c += wBr();
        c += wSub("Register the event type – in init.lua");
        c += wC("AppStateManager:getScriptEventManager():registerEvent(\"EventData_MyEvent\")");
        c += wBr();
        c += wSub("Send an event");
        c += wC("local eventData = {}");
        c += wC("eventData[\"score\"] = 100");
        c += wC("eventData[\"playerId\"] = player:getId()");
        c += wC("AppStateManager:getScriptEventManager():queueEvent(");
        c += wC("    EventType.EventData_MyEvent, eventData)");
        c += wBr();
        c += wSub("Listen for an event (connect)");
        c += wC("if EventType.EventData_MyEvent ~= nil then");
        c += wC("    AppStateManager:getScriptEventManager():registerEventListener(");
        c += wC("        EventType.EventData_MyEvent,");
        c += wC("        function(eventData)");
        c += wC("            local score = eventData[\"score\"]");
        c += wC("            log(\"Score: \" .. tostring(score))");
        c += wC("        end)");
        c += wC("end");
        c += wBr();
        c += wSub("Named handler (alternative)");
        c += wC("AppStateManager:getScriptEventManager():registerEventListener(");
        c += wC("    EventType.EventData_MyEvent, MyScript[\"onMyEvent\"])");
        c += wBr();
        c += wTip("Always guard with ~= nil before registering. "
                  "The EventType table is populated at scene load from init.lua.");
        entries.push_back({"15 - Custom Events", c});
    }

    // ── 16. Spawning ──────────────────────────────────────────────────────
    {
        Ogre::String c;
        c += wH("Spawning GameObjects");
        c += wP("SpawnComponent clones a template GameObject. "
                "Set keepAlive to prevent auto-deletion of spawned instances.");
        c += wBr();
        c += wSub("Setup");
        c += wC("spawnComp = gameObject:getSpawnComponent()");
        c += wC("-- by name if multiple:");
        c += wC("spawnComp = gameObject:getSpawnComponentFromName(\"BulletSpawn\")");
        c += wC("spawnComp:setKeepAliveSpawnedGameObjects(true)");
        c += wBr();
        c += wSub("React on spawn (configure spawned object)");
        c += wC("spawnComp:reactOnSpawn(function(spawnedGO, originGO)");
        c += wC("    spawnedGO:getPhysicsActiveComponent()");
        c += wC("        :applyRequiredForceForVelocity(Vector3(0, 0, -100))");
        c += wC("    spawnedGO:getSimpleSoundComponent():setActivated(true)");
        c += wC("    spawnedGO:setVisible(true)");
        c += wC("    spawnedGO:getPhysicsComponent():setCollidable(true)");
        c += wC("end)");
        c += wBr();
        c += wSub("Trigger spawn");
        c += wC("spawnComp:setActivated(true)");
        c += wBr();
        c += wSub("Randomize spawn target");
        c += wC("spawnComp:setSpawnTargetId(goldTemplate:getId())  -- change template");
        c += wBr();
        c += wSub("Delayed deletion of spawned objects");
        c += wC("AppStateManager:getGameObjectController()");
        c += wC("    :deleteDelayedGameObject(spawnedGO:getId(), 3)  -- after 3 seconds");
        c += wBr();
        c += wSub("Cloned callback (fires when this GO is itself cloned)");
        c += wC("MyScript[\"cloned\"] = function(gameObject)");
        c += wC("    -- Activate spawn only for clones, not the template");
        c += wC("    gameObject:getSpawnComponent():setActivated(true)");
        c += wC("end");
        entries.push_back({"16 - Spawning", c});
    }

    // ── 17. Save & Load Progress ──────────────────────────────────────────
    {
        Ogre::String c;
        c += wH("Save & Load Progress");
        c += wP("GameProgressModule persists scene state and arbitrary key-value data "
                "across sessions. Use global values to pass data between scenes.");
        c += wBr();
        c += wSub("Save / load scene state");
        c += wC("local gpm = AppStateManager:getGameProgressModule()");
        c += wBr();
        c += wC("-- Save (saveName, crypted, withUndo):");
        c += wC("local ok = gpm:saveProgress(\"MySaveSlot\", false, true)");
        c += wBr();
        c += wC("-- Load (saveName, reloadSceneFromDisk, withUndo):");
        c += wC("local ok = gpm:loadProgress(\"MySaveSlot\", true, false)");
        c += wBr();
        c += wSub("Store cross-scene global values");
        c += wC("gpm:setGlobalStringValue(\"PlayerName\",    \"Hero\")");
        c += wC("gpm:setGlobalBoolValue(  \"Boss1Defeated\", true)");
        c += wC("gpm:setGlobalNumberValue(\"Score\",         1000)");
        c += wBr();
        c += wSub("Read global values");
        c += wC("local val = gpm:getGlobalValue(\"Score\")");
        c += wC("if val ~= nil then");
        c += wC("    log(\"Score: \" .. tostring(val:getValueNumber()))");
        c += wC("end");
        c += wBr();
        c += wSub("Change scene");
        c += wC("AppStateManager:getGameProgressModule():changeScene(\"Level2\")");
        c += wBr();
        c += wSub("Get save-game snapshots");
        c += wC("local snapshots = Core:getSceneSnapshotsInProject(Core:getProjectName())");
        c += wC("local first = snapshots[0]  -- 0-based");
        c += wTip("Use setGlobalStringValue / getGlobalValue to share device names, "
                  "player colors, and other config between menu and game states.");
        entries.push_back({"17 - Save & Load Progress", c});
    }

    // ── 18. Vehicle Control ───────────────────────────────────────────────
    {
        Ogre::String c;
        c += wH("Vehicle Control");
        c += wP("PhysicsActiveVehicleComponent uses dedicated callbacks for steering, "
                "motor, hand-brake, brake, and tire contact. Each fires on its own tick.");
        c += wBr();
        c += wSub("Connect");
        c += wC("physVehicle  = gameObject:getPhysicsActiveVehicleComponent()   -- V1");
        c += wC("-- physVehicle = gameObject:getPhysicsActiveVehicleComponentV2() -- V2");
        c += wC("inputDeviceModule = gameObject:getInputDeviceComponent():getInputDeviceModule()");
        c += wC("inputDeviceModule:setAnalogActionThreshold(0.55)");
        c += wBr();
        c += wSub("Steering (smooth, expo-decay)");
        c += wC("MyVehicle[\"onSteeringAngleChanged\"] = function(manip, dt)");
        c += wC("    local axis = inputDeviceModule:getSteerAxis()");
        c += wC("    if math.abs(axis) < 0.10 then axis = 0.0 end");
        c += wC("    local target = -axis * 45.0  -- max degrees");
        c += wC("    steerAmount  = steerAmount + (target - steerAmount)");
        c += wC("                   * (1 - math.exp(-10 * dt))");
        c += wC("    manip:setSteerAngle(steerAmount)");
        c += wC("end");
        c += wBr();
        c += wSub("Motor");
        c += wC("MyVehicle[\"onMotorForceChanged\"] = function(manip, dt)");
        c += wC("    if inputDeviceModule:isActionDown(NOWA_A_UP) then");
        c += wC("        manip:setMotorForce(5000 * 120 * dt)");
        c += wC("    elseif inputDeviceModule:isActionDown(NOWA_A_DOWN) then");
        c += wC("        manip:setMotorForce(-2000 * 120 * dt)");
        c += wC("    end");
        c += wC("end");
        c += wBr();
        c += wSub("Brake & hand-brake");
        c += wC("MyVehicle[\"onBrakeChanged\"] = function(manip, dt)");
        c += wC("    if inputDeviceModule:isActionDown(NOWA_A_COWER) then manip:setBrake(7.5) end");
        c += wC("end");
        c += wC("MyVehicle[\"onHandBrakeChanged\"] = function(manip, dt)");
        c += wC("    if inputDeviceModule:isActionDown(NOWA_A_JUMP) then manip:setHandBrake(5.5) end");
        c += wC("end");
        c += wBr();
        c += wSub("Special moves");
        c += wC("physVehicle:applyWheelie(2)              -- F key");
        c += wC("physVehicle:applyDrift(true, 40000, 2)   -- left drift");
        c += wC("physVehicle:applyPitch(-100, dt)         -- nose-down");
        entries.push_back({"18 - Vehicle Control", c});
    }

    // ── 19. Mesh Deformation ──────────────────────────────────────────────
    {
        Ogre::String c;
        c += wH("Mesh Deformation");
        c += wP("MeshModifyComponent deforms vertices at runtime based on world-space "
                "contact data. Useful for crash deformation on vehicles.");
        c += wBr();
        c += wSub("Setup in connect()");
        c += wC("meshMod = vehicleGameObject:getMeshModifyComponent()");
        c += wC("meshMod:setBrushMode(\"Push\")      -- push vertices inward");
        c += wC("meshMod:setBrushSize(2.0)           -- effect radius");
        c += wC("meshMod:setBrushIntensity(0.8)      -- deformation strength");
        c += wC("meshMod:setBrushFalloff(2.0)        -- smooth edge falloff");
        c += wBr();
        c += wSub("Deform in contact callback");
        c += wC("MyScript[\"onContact\"] = function(go0, go1, contact)");
        c += wC("    if contact == nil then return end");
        c += wC("    local data   = contact:getPositionAndNormal()");
        c += wC("    local pos    = data[0]   -- world-space contact point");
        c += wC("    local normal = data[1]   -- surface normal");
        c += wC("    local force  = contact:getNormalSpeed()");
        c += wBr();
        c += wC("    meshMod:deformAtWorldPositionByForceAndNormal(");
        c += wC("        pos, normal, force, 30.0, false)");
        c += wBr();
        c += wC("    if force > 20.0 then");
        c += wC("        meshMod:rebuildCollision()  -- update physics shape");
        c += wC("    end");
        c += wBr();
        c += wC("    if meshMod:getCurrentDamage() > 0.5 then");
        c += wC("        log(\"Heavily damaged!\")");
        c += wC("    end");
        c += wC("end");
        c += wBr();
        c += wTip("Use onContactOnce instead of onContact to avoid firing every physics "
                  "sub-step. Much cheaper for deformation.");
        entries.push_back({"19 - Mesh Deformation", c});
    }

    // ── 20. Camera Switching ──────────────────────────────────────────────
    {
        Ogre::String c;
        c += wH("Camera Switching");
        c += wP("Activate/deactivate CameraComponent on any GameObject. "
                "Only one camera should be active at a time.");
        c += wBr();
        c += wSub("Switch to a custom camera");
        c += wC("local camGO = AppStateManager:getGameObjectController()");
        c += wC("                  :getGameObjectFromName(\"IntroCamera\")");
        c += wC("local cam   = camGO:getCameraComponent()");
        c += wC("cam:setActivated(true)  -- makes this camera active");
        c += wBr();
        c += wSub("Activate third-person camera on a ghost object");
        c += wC("ghostBall = AppStateManager:getGameObjectController()");
        c += wC("                :getGameObjectFromId(\"3855377240\")");
        c += wC("local tpCam = ghostBall:getCameraBehaviorThirdPersonComponent()");
        c += wC("tpCam:setActivated(true)");
        c += wBr();
        c += wSub("Activate per-player camera (split-screen)");
        c += wC("AppStateManager:getGameObjectController()");
        c += wC("    :activatePlayerControllerForCamera(");
        c += wC("        true, player1:getId(), \"cameraGoId\", true)");
        c += wBr();
        c += wSub("Configure camera position/orientation");
        c += wC("cam:setCameraPosition(Vector3(0, 80, -35))");
        c += wC("cam:setCameraDegreeOrientation(Vector3(-90, 180, 180))");
        c += wBr();
        c += wTip("To return to the editor camera after simulation, "
                  "deactivate your custom camera in disconnect() or call undoAll().");
        entries.push_back({"20 - Camera Switching", c});
    }

    // ── 21. Undo All / Editor Safety ─────────────────────────────────────
    {
        Ogre::String c;
        c += wH("Undo All – Editor Safety Pattern");
        c += wP("When testing inside NOWA-Design, all scene changes made during simulation "
                "(deleted objects, moved transforms, changed properties) are tracked. "
                "undoAll() reverses all of them when the simulation stops.");
        c += wBr();
        c += wSub("Always add to disconnect()");
        c += wC("MyScript[\"disconnect\"] = function()");
        c += wC("    AppStateManager:getGameObjectController():undoAll()");
        c += wC("end");
        c += wBr();
        c += wSub("Undo-tracked deletion (deleted GO is restored on undo)");
        c += wC("AppStateManager:getGameObjectController()");
        c += wC("    :deleteGameObjectWithUndo(someGameObject:getId())");
        c += wBr();
        c += wSub("Non-tracked deletion (permanent, not restored on undo)");
        c += wC("AppStateManager:getGameObjectController()");
        c += wC("    :deleteGameObject(someGameObject:getId())");
        c += wBr();
        c += wSub("Delayed deletion");
        c += wC("AppStateManager:getGameObjectController()");
        c += wC("    :deleteDelayedGameObject(go:getId(), 3)  -- 3 seconds");
        c += wBr();
        c += wWarn("Forgetting undoAll() means spawned objects, deleted props, and "
                   "changed attributes are permanently modified in the editor after stopping.");
        c += wTip("Use deleteGameObjectWithUndo during testing so deleted objects "
                  "reappear when you stop the simulation. Switch to deleteGameObject "
                  "in the final shipped build.");
        entries.push_back({"21 - Undo All (Editor Safety)", c});
    }

    // ── 22. Lua Array / Table Access ──────────────────────────────────────
    {
        Ogre::String c;
        c += wH("Lua Array & Table Access");
        c += wP("NOWA exposes C++ collections in two ways. Understanding the difference "
                "prevents the most common Lua scripting bugs.");
        c += wBr();

        c += wSub("C++ std::vector (0-based, returned by getGameObjectsFrom*)");
        c += wC("-- # gives the LAST VALID INDEX (size - 1), NOT the count.");
        c += wC("-- Loop pattern:  for i = 0, #arr do");
        c += wC("local enemies = AppStateManager:getGameObjectController()");
        c += wC("                    :getGameObjectsFromCategory(\"Enemy\")");
        c += wC("for i = 0, #enemies do");
        c += wC("    local enemy = enemies[i]   -- 0-based access");
        c += wC("    log(enemy:getName())");
        c += wC("end");
        c += wC("-- Total count:  #arr + 1");
        c += wC("log(\"Count: \" .. (#enemies + 1))");
        c += wBr();

        c += wSub("Lua table (returned as pairs, 1-based)");
        c += wC("-- getGameObjectsFromCategory returns a Lua table (key/value pairs).");
        c += wC("-- Use pairs() – keys are integers starting at 1 (or arbitrary).");
        c += wC("local victims = AppStateManager:getGameObjectController()");
        c += wC("                    :getGameObjectsFromCategory(\"Victim\")");
        c += wC("for key, victim in pairs(victims) do");
        c += wC("    log(\"key=\" .. tostring(key) .. \" name=\" .. victim:getName())");
        c += wC("end");
        c += wBr();

        c += wSub("Device list (while-loop, 0-based C++ vector)");
        c += wC("local devices = inputDeviceComp:getActualizedDeviceList()");
        c += wC("local i = 0");
        c += wC("while i <= #devices do");
        c += wC("    comboBox:addItem(devices[i])");
        c += wC("    i = i + 1");
        c += wC("end");
        c += wBr();

        c += wSub("Component list (0-based C++ vector)");
        c += wC("local nodes = AppStateManager:getGameObjectController()");
        c += wC("                  :getGameObjectsFromComponent('NodeComponent')");
        c += wC("-- Count nodes:");
        c += wC("local count = #nodes + 1");
        c += wC("for i = 0, #nodes do");
        c += wC("    local go = nodes[i]");
        c += wC("    log(go:getId())");
        c += wC("end");
        c += wBr();

        c += wSub("Brush names (0-based C++ vector from getTerraComponent)");
        c += wC("local brushNames = terraGO:getTerraComponent():getAllBrushNames()");
        c += wC("for i = 0, #brushNames do");
        c += wC("    log(\"Brush: \" .. brushNames[i])");
        c += wC("end");
        c += wBr();

        c += wTip("Rule of thumb: if the function name starts with get...From..., "
                  "the result is a 0-based C++ vector – use # as last index. "
                  "If you built the table yourself with table.insert(), use ipairs() "
                  "and 1-based indexing as normal Lua.");
        c += wWarn("Never mix up the two: calling pairs() on a C++ vector proxy or "
                   "ipairs() on a 0-based vector will give wrong results or skip index 0.");
        entries.push_back({"22 - Lua Array & Table Access", c});
    }

    // ── 23. Pure Pursuit / Race Waypoints ────────────────────────────────
    {
        Ogre::String c;
        c += wH("Pure Pursuit & Race Waypoints");
        c += wP("The Pure Pursuit algorithm calculates the steering angle needed to follow "
                "a path of waypoints. It is ideal for AI race vehicles and autonomous agents. "
                "A LuaScriptComponent is required so calculateSteeringAngle / "
                "calculatePitchAngle can be called from script.");
        c += wBr();

        c += wSub("AI vehicle: read pre-computed values each frame");
        c += wC("purePursuit = gameObject:getPurePursuitComponent()");
        c += wBr();
        c += wC("MyAI[\"onSteeringAngleChanged\"] = function(manip, dt)");
        c += wC("    manip:setSteerAngle(purePursuit:getSteerAmount())");
        c += wC("end");
        c += wC("MyAI[\"onMotorForceChanged\"] = function(manip, dt)");
        c += wC("    manip:setMotorForce(purePursuit:getMotorForce())");
        c += wC("end");
        c += wBr();

        c += wSub("Generating waypoints from NodeComponents via Lua console");
        c += wP("Setting dozens of waypoints manually in the editor is tedious. "
                "Run this snippet once in the Lua console (^ key) to assign all "
                "NodeComponent GameObjects as waypoints automatically. "
                "Adapt the GameObject ID to match your vehicle.");
        c += wBr();
        c += wC("local thisGO = AppStateManager:getGameObjectController()");
        c += wC("                   :getGameObjectFromId('528669575')  -- your vehicle id");
        c += wC("local purePursuitComp = thisGO:getPurePursuitComponent()");
        c += wC("local nodes = AppStateManager:getGameObjectController()");
        c += wC("                  :getGameObjectsFromComponent('NodeComponent')");
        c += wC("purePursuitComp:setWaypointsCount(#nodes + 1)");
        c += wC("for i = 0, #nodes do");
        c += wC("    purePursuitComp:setWaypointId(i, nodes[i]:getId())");
        c += wC("end");
        c += wC("purePursuitComp:reorderWaypoints()");
        c += wBr();

        c += wSub("Race feedback callbacks (player vehicle)");
        c += wC("MyVehicle[\"onFeedbackRace\"] = function(currentLap, lapTimeSec, finished)");
        c += wC("    lapText:setCaption(\"Lap: \" .. currentLap)");
        c += wC("    lapTimeList:setItemText(currentLap - 1,");
        c += wC("        \"Lap \" .. currentLap .. \": \" .. lapTimeSec .. \" s\")");
        c += wC("    finishedText:setActivated(finished)");
        c += wC("end");
        c += wBr();
        c += wC("MyVehicle[\"onWrongDirectionDriving\"] = function(wrongDirection)");
        c += wC("    wrongDirText:setActivated(wrongDirection == true)");
        c += wC("end");
        c += wBr();
        c += wC("MyVehicle[\"onCountdown\"] = function(countdownNumber)");
        c += wC("    if countdownNumber > 0 then");
        c += wC("        countdownText:setCaption(tostring(countdownNumber))");
        c += wC("    else");
        c += wC("        countdownText:setActivated(false)");
        c += wC("    end");
        c += wC("end");
        c += wBr();

        c += wSub("Speed readout");
        c += wC("local kmh = raceGoalComponent:getSpeedInKmh()");
        c += wC("local pos  = raceGoalComponent:getRacingPosition()  -- -1 if unavailable");
        c += wBr();

        c += wTip("Check the orientation and global mesh direction of the checkpoint "
                  "NodeComponents so the vehicle drives in the correct direction. "
                  "reorderWaypoints() sorts them by their scene order automatically.");
        entries.push_back({"23 - Pure Pursuit & Race Waypoints", c});
    }

    // ── 24. Editor vs. Game Detection ────────────────────────────────────
    {
        Ogre::String c;
        c += wH("Editor vs. Game Detection  (Core:isGame())");
        c += wP("Core:isGame() returns true only when the exported game binary is running. "
                "Inside NOWA-Design it returns false. "
                "Use this to guard state transitions, save/load calls, or any logic "
                "that must not fire while testing in the editor.");
        c += wBr();

        c += wSub("Guard an app-state transition (e.g. after an intro fade)");
        c += wC("local fadeComponent = gameObject:getFadeComponent()");
        c += wC("fadeComponent:reactOnFadeCompleted(function()");
        c += wC("    if Core:isGame() == true then");
        c += wC("        AppStateManager:changeAppState(\"MenuState\")");
        c += wC("    end");
        c += wC("    -- In NOWA-Design the block above is skipped,");
        c += wC("    -- so the intro just plays without switching state.");
        c += wC("end)");
        c += wBr();

        c += wSub("Other common guards");
        c += wC("-- Only save/load in a real game session:");
        c += wC("if Core:isGame() == true then");
        c += wC("    AppStateManager:getGameProgressModule():loadProgress(");
        c += wC("        Core:getCurrentSaveGameName(), false, false)");
        c += wC("end");
        c += wBr();
        c += wC("-- Activate the player controller only in-game:");
        c += wC("if Core:isGame() == true then");
        c += wC("    AppStateManager:getGameObjectController()");
        c += wC("        :activatePlayerController(true, gameObject:getId(), true)");
        c += wC("end");
        c += wBr();
        c += wC("-- Show / hide the mouse cursor conditionally:");
        c += wC("if Core:isGame() == true then");
        c += wC("    PointerManager:showMouse(false)");
        c += wC("end");
        c += wBr();

        c += wTip("A good pattern: always write disconnect() so it works in both modes "
                  "(call undoAll()), but wrap irreversible transitions like "
                  "changeAppState() or exitGame() in Core:isGame() guards.");
        entries.push_back({"24 - Editor vs. Game Detection", c});
    }

    // ── 25. GameObject ────────────────────────────────────────────────────
    {
        Ogre::String c;
        c += wH("GameObject – Core Methods");
        c += wP("The central object type. Every entity in the scene is a GameObject "
                "composable from multiple components.");
        c += wBr();
        c += wSub("Identity");
        c += wC("local id   = go:getId()           -- unique string ID (primary key)");
        c += wC("local name = go:getName()");
        c += wC("local uname = go:getUniqueName()  -- name + id combined");
        c += wBr();
        c += wSub("Transform (read-only from Lua – write via physics component)");
        c += wC("local pos  = go:getPosition()     -- Vector3");
        c += wC("local ori  = go:getOrientation()  -- Quaternion");
        c += wC("local scl  = go:getScale()        -- Vector3");
        c += wC("local sz   = go:getSize()         -- bounding box size");
        c += wC("local ctr  = go:getCenterOffset() -- offset from mesh origin to AABB center");
        c += wC("local dir  = go:getDefaultDirection() -- model forward axis");
        c += wBr();
        c += wSub("Visibility");
        c += wC("go:setVisible(false)   -- hide");
        c += wC("go:setVisible(true)    -- show");
        c += wC("local v = go:getVisible()");
        c += wBr();
        c += wSub("Category & tag");
        c += wC("local cat  = go:getCategory()     -- e.g. \"Enemy\"");
        c += wC("local tag  = go:getTagName()      -- sub-category e.g. \"Boss\"");
        c += wC("local catId= go:getCategoryId()");
        c += wC("go:changeCategory2(\"NewCategory\")  -- change category at runtime");
        c += wBr();
        c += wSub("Debug helpers");
        c += wC("go:showBoundingBox(true)");
        c += wC("go:showDebugData()     -- toggles debug overlay for all components");
        c += wBr();
        c += wSub("SceneNode (for parent/child transforms)");
        c += wC("local node = go:getSceneNode()");
        c += wC("node:setPosition(x, y, z)   -- direct scene-node move (no physics)");
        c += wTip("Always use the PhysicsActiveComponent to move physics-enabled objects. "
                  "Directly moving the SceneNode bypasses physics and causes desync.");
        entries.push_back({"25 - GameObject Core Methods", c});
    }

    // ── 26. GameObjectController ──────────────────────────────────────────
    {
        Ogre::String c;
        c += wH("GameObjectController – Finding & Managing GameObjects");
        c += wP("The central registry for all GameObjects in the current scene. "
                "Always access via AppStateManager:getGameObjectController().");
        c += wBr();
        c += wSub("Shorthand alias (recommended)");
        c += wC("local goc = AppStateManager:getGameObjectController()");
        c += wBr();
        c += wSub("Finding single GameObjects");
        c += wC("local go = goc:getGameObjectFromId(\"1234567890\")");
        c += wC("local go = goc:getGameObjectFromName(\"Chest2_0\")");
        c += wC("local go = goc:getGameObjectFromNamePrefix(\"Enemy\") -- first match");
        c += wBr();
        c += wSub("Finding multiple GameObjects (returns Lua table – use pairs)");
        c += wC("local enemies = goc:getGameObjectsFromCategory(\"Enemy\")");
        c += wC("for key, enemy in pairs(enemies) do log(enemy:getName()) end");
        c += wBr();
        c += wC("local nodes = goc:getGameObjectsFromComponent(\"NodeComponent\")");
        c += wC("-- 0-based C++ vector: for i = 0, #nodes do ... nodes[i] ... end");
        c += wBr();
        c += wC("local tagged = goc:getGameObjectsFromTagName(\"Stone\")");
        c += wC("local named  = goc:getGameObjectsFromNamePrefix(\"Coin\")");
        c += wBr();
        c += wSub("Deletion");
        c += wC("goc:deleteGameObject(go:getId())              -- immediate");
        c += wC("goc:deleteGameObjectWithUndo(go:getId())      -- undo-tracked");
        c += wC("goc:deleteDelayedGameObject(go:getId(), 3)    -- after 3 seconds");
        c += wBr();
        c += wSub("Player controller activation");
        c += wC("-- (active, gameObjectId, onlyOneActive)");
        c += wC("goc:activatePlayerController(true, go:getId(), true)");
        c += wC("-- Split-screen with dedicated camera:");
        c += wC("goc:activatePlayerControllerForCamera(true, go:getId(), camGoId, true)");
        c += wBr();
        c += wSub("Type-cast helpers (for Lua auto-complete)");
        c += wC("go       = goc:castGameObject(go)");
        c += wC("contact  = goc:castContact(contact)");
        c += wC("phys     = goc:castPhysicsActiveComponent(phys)");
        c += wC("ctrl     = goc:castPhysicsPlayerControllerComponent(ctrl)");
        c += wBr();
        c += wSub("Cycling through a group (player switch)");
        c += wC("-- getNextGameObject cycles through objects matching the category");
        c += wC("local next = goc:getNextGameObject(\"Player\")");
        c += wC("goc:activatePlayerController(true, next:getId(), false)");
        entries.push_back({"26 - GameObjectController", c});
    }

    // ── 27. AppStateManager ───────────────────────────────────────────────
    {
        Ogre::String c;
        c += wH("AppStateManager – Application State Management");
        c += wP("Manages the stack of app states (scenes). Only one scene runs at a time. "
                "States can be pushed on top (e.g. a pause menu) and popped off to resume.");
        c += wBr();
        c += wSub("State transitions");
        c += wC("-- Replace current state entirely:");
        c += wC("AppStateManager:changeAppState(\"GameState\")");
        c += wBr();
        c += wC("-- Push a new state on top (current state pauses, not destroyed):");
        c += wC("AppStateManager:pushAppState(\"MenuState\")");
        c += wBr();
        c += wC("-- Pop the current state; prior state resumes:");
        c += wC("AppStateManager:popAppState()");
        c += wBr();
        c += wC("-- Clear all states and start fresh with a new one:");
        c += wC("AppStateManager:popAllAndPushAppState(\"GameState\")");
        c += wBr();
        c += wSub("State queries");
        c += wC("-- Has a given state been entered at least once?");
        c += wC("if AppStateManager:hasAppStateStarted(\"GameState\") then");
        c += wC("    continueButton:setActivated(true)");
        c += wC("end");
        c += wBr();
        c += wC("if AppStateManager:isInAppstate(\"MenuState\") then ... end");
        c += wBr();
        c += wSub("Exit game");
        c += wC("AppStateManager:exitGame()  -- guard with Core:isGame() == true!");
        c += wBr();
        c += wSub("Module accessors");
        c += wC("AppStateManager:getGameObjectController()   -- current scene objects");
        c += wC("AppStateManager:getGameProgressModule()     -- save/load, globals");
        c += wC("AppStateManager:getScriptEventManager()     -- Lua events");
        c += wBr();
        c += wTip("Use pushAppState for pause menus so the game scene is preserved. "
                  "Use changeAppState when truly switching scenes (level transitions).");
        entries.push_back({"27 - AppStateManager", c});
    }

    // ── 28. AttributesComponent + Variant ────────────────────────────────
    {
        Ogre::String c;
        c += wH("AttributesComponent & Variant");
        c += wP("AttributesComponent stores named typed values (number, bool, string, Vector) "
                "on a GameObject. Attributes persist across save/load and are exposed in the editor. "
                "getAttributeValueByName() returns a Variant object.");
        c += wBr();
        c += wSub("Get component");
        c += wC("local attrs = go:getAttributesComponent()");
        c += wBr();
        c += wSub("Read a Variant by name");
        c += wC("local energy = attrs:getAttributeValueByName(\"Energy\")");
        c += wC("if energy ~= nil then");
        c += wC("    log(\"Energy: \" .. energy:getValueNumber())");
        c += wC("end");
        c += wBr();
        c += wSub("Variant methods (all typed)");
        c += wC("energy:setValueNumber(100)");
        c += wC("energy:setValueBool(true)");
        c += wC("energy:setValueString(\"alive\")");
        c += wC("energy:setValueVector3(Vector3(1, 0, 0))");
        c += wBr();
        c += wC("local n = energy:getValueNumber()");
        c += wC("local b = energy:getValueBool()");
        c += wC("local s = energy:getValueString()");
        c += wC("local name = energy:getName()  -- attribute name");
        c += wBr();
        c += wSub("Add attributes dynamically (runtime only, not saved to editor)");
        c += wC("attrs:addAttributeNumber(\"Score\", 0)");
        c += wC("attrs:addAttributeBool(\"IsDead\", false)");
        c += wC("attrs:addAttributeString(\"PlayerName\", \"Hero\")");
        c += wBr();
        c += wSub("Change by name (no Variant reference needed)");
        c += wC("attrs:changeValueNumber(\"Energy\", 50)");
        c += wC("attrs:changeValueBool(\"IsActive\", true)");
        c += wBr();
        c += wSub("Read directly by name");
        c += wC("local hp = attrs:getAttributeValueNumber(\"Energy\")");
        c += wC("local nm = attrs:getAttributeValueString(\"PlayerName\")");
        c += wC("local fl = attrs:getAttributeValueBool(\"IsAlive\")");
        c += wBr();
        c += wTip("Keep a Variant reference from connect() for frequent read/write. "
                  "Direct name lookups (getAttributeValueNumber) are fine for one-off reads.");
        entries.push_back({"28 - AttributesComponent & Variant", c});
    }

    // ── 29. Contact (Physics Callbacks) ──────────────────────────────────
    {
        Ogre::String c;
        c += wH("Contact – Physics Collision Callbacks");
        c += wP("A Contact object is passed to onContact / onContactOnce / onContactFriction "
                "callbacks. It carries position, normal, impact speed, and lets you override "
                "friction, elasticity, and normal direction.");
        c += wBr();
        c += wSub("Callback signatures");
        c += wC("MyScript[\"onContact\"] = function(go0, go1, contact)");
        c += wC("    -- fires every physics sub-step while bodies touch");
        c += wC("end");
        c += wC("MyScript[\"onContactOnce\"] = function(go0, go1, contact)");
        c += wC("    -- fires only on first contact frame (cheaper for deform/sound)");
        c += wC("end");
        c += wC("MyScript[\"onContactFriction\"] = function(go0, go1, playerContact)");
        c += wC("    -- override friction per-contact");
        c += wC("    playerContact:setResultFriction(2)");
        c += wC("end");
        c += wBr();
        c += wSub("Position & normal");
        c += wC("local data   = contact:getPositionAndNormal()");
        c += wC("local pos    = data[0]   -- world-space contact point (Vector3)");
        c += wC("local normal = data[1]   -- surface normal (Vector3)");
        c += wBr();
        c += wSub("Impact strength");
        c += wC("local speed  = contact:getNormalSpeed()          -- velocity along normal");
        c += wC("local impact = contact:getContactMaxNormalImpact()");
        c += wC("local pen    = contact:getContactPenetration()");
        c += wBr();
        c += wSub("Tangent directions");
        c += wC("local dirs   = contact:getTangentDirections()");
        c += wC("local tang0  = dirs[0]   -- primary tangent");
        c += wC("local tang1  = dirs[1]   -- secondary tangent");
        c += wBr();
        c += wSub("Override physics response");
        c += wC("-- Disable friction along primary tangent:");
        c += wC("contact:setFrictionState(0, 0)");
        c += wC("contact:setFrictionCoefficient(0, 0, 0)");
        c += wC("-- Adjust elasticity (bounciness):");
        c += wC("contact:setElasticity(0.8)");
        c += wC("-- Add upward acceleration on contact (jump pad):");
        c += wC("contact:setNormalAcceleration(200)");
        c += wBr();
        c += wSub("Debug");
        c += wC("log(contact:print())  -- dumps all contact properties");
        c += wTip("Use onContactOnce for one-time effects (deformation, sounds, sparks). "
                  "Use onContact only when you need per-sub-step control (conveyors, ice).");
        entries.push_back({"29 - Contact (Physics Callbacks)", c});
    }

    // ── 30. LuaScriptComponent ────────────────────────────────────────────
    {
        Ogre::String c;
        c += wH("LuaScriptComponent – Delayed Calls");
        c += wP("LuaScriptComponent exposes callDelayedMethod(), which schedules a "
                "Lua closure to fire after a given number of seconds. "
                "Calls can be chained for multi-step timed sequences.");
        c += wBr();
        c += wSub("Basic usage");
        c += wC("local lua = go:getLuaScriptComponent()");
        c += wC("lua:callDelayedMethod(function()");
        c += wC("    anim:blend5(AnimID.ANIM_IDLE_1,");
        c += wC("                AnimationBlender.BLEND_WHILE_ANIMATING, 0.1, true)");
        c += wC("end, 2.0)   -- fires after 2 seconds");
        c += wBr();
        c += wSub("Chained sequence (multi-step animation pipeline)");
        c += wC("lua:callDelayedMethod(function()");
        c += wC("    anim:blend5(AnimID.ANIM_KNOCK_DOWN,");
        c += wC("                AnimationBlender.BLEND_WHILE_ANIMATING, 0.1, false)");
        c += wBr();
        c += wC("    lua:callDelayedMethod(function()  -- nested call");
        c += wC("        anim:blend5(AnimID.ANIM_STAND_UP,");
        c += wC("                    AnimationBlender.BLEND_WHILE_ANIMATING, 0.1, false)");
        c += wBr();
        c += wC("        lua:callDelayedMethod(function()");
        c += wC("            canAct = true  -- re-enable input after full sequence");
        c += wC("        end, 1.5)");
        c += wC("    end, 2.0)");
        c += wC("end, 2.0)");
        c += wBr();
        c += wSub("Combined with reactOnAnimationFinished");
        c += wC("playerController:reactOnAnimationFinished(function()");
        c += wC("    -- animation just ended; start a 1-second delay then do next action");
        c += wC("    lua:callDelayedMethod(function()");
        c += wC("        spawnComp:setActivated(true)");
        c += wC("    end, 1.0)");
        c += wC("end, true)  -- true = one-shot");
        c += wBr();
        c += wTip("Delays are wall-clock seconds in simulation time. They are not cancelled "
                  "automatically if the scene stops — call undoAll() from disconnect() to "
                  "ensure a clean slate.");
        entries.push_back({"30 - LuaScriptComponent (Delayed Calls)", c});
    }

    // ── 31. MathHelper ────────────────────────────────────────────────────
    {
        Ogre::String c;
        c += wH("MathHelper – Math & Orientation Utilities");
        c += wP("A singleton with geometry helpers, smoothing filters, "
                "quaternion utilities, and comparison functions.");
        c += wBr();
        c += wSub("Smoothing");
        c += wC("-- Low-pass filter: smooths a noisy value over time");
        c += wC("-- scale: 0.01 = very smooth, 0.5 = fast");
        c += wC("smoothVal = MathHelper:lowPassFilter(rawVal, smoothVal, 0.05)");
        c += wC("-- Vector3 version:");
        c += wC("smoothPos = MathHelper:lowPassFilter(rawPos, smoothPos, 0.05)");
        c += wBr();
        c += wSub("Clamping & rounding");
        c += wC("local clamped = MathHelper:clamp(value, 0, 100)");
        c += wC("local rounded = MathHelper:round(3.14159, 2)   -- -> 3.14");
        c += wC("local rounded = MathHelper:round(3.6)           -- -> 4");
        c += wBr();
        c += wSub("Orientation helpers");
        c += wC("-- Instantly face a target (no smoothing):");
        c += wC("local q = MathHelper:faceTarget(go:getSceneNode(), target:getSceneNode())");
        c += wC("-- With default model direction taken into account:");
        c += wC("local q = MathHelper:faceTarget(");
        c += wC("    go:getSceneNode(), target:getSceneNode(), go:getDefaultDirection())");
        c += wBr();
        c += wC("-- Smooth slerp toward a world direction (use in update loop):");
        c += wC("local q = MathHelper:faceDirectionSlerp(");
        c += wC("    go:getOrientation(),     -- current");
        c += wC("    Vector3.NEGATIVE_UNIT_Z, -- target direction");
        c += wC("    go:getDefaultDirection(), dt, 1200)  -- speed");
        c += wC("-- Then apply via physics:");
        c += wC("physComp:applyOmegaForceRotateTo(q, Vector3.UNIT_Y, 10)");
        c += wBr();
        c += wC("-- lookAt: orientation to face an unnormalized direction:");
        c += wC("local q = MathHelper:lookAt(targetPos - goPos)");
        c += wBr();
        c += wSub("Quaternion decomposition");
        c += wC("-- Extract Euler (heading, pitch, roll) in radians:");
        c += wC("local euler = MathHelper:extractEuler(go:getOrientation())");
        c += wC("-- euler.x = heading, euler.y = pitch, euler.z = roll");
        c += wBr();
        c += wSub("Angle utilities");
        c += wC("local d = MathHelper:normalizeDegreeAngle(370)  -- -> 10");
        c += wC("local r = MathHelper:normalizeRadianAngle(7.0)");
        c += wC("local eq = MathHelper:degreeAngleEquals(350, -10)  -- true");
        c += wC("local eq = MathHelper:vector3Equals(v1, v2, 0.001)");
        entries.push_back({"31 - MathHelper", c});
    }

    // ── 32. GameObjectTitleComponent ─────────────────────────────────────
    {
        Ogre::String c;
        c += wH("GameObjectTitleComponent – In-World Text");
        c += wP("Renders a text label in world space above (or at an offset from) "
                "a GameObject. Useful for name tags, status readouts, and health bars.");
        c += wBr();
        c += wSub("Get & use");
        c += wC("local title = go:getGameObjectTitleComponent()");
        c += wC("title:setCaption(\"Energy: 100\")");
        c += wC("title:setCaption(\"\")  -- hide text without deactivating");
        c += wBr();
        c += wSub("Appearance");
        c += wC("title:setColor(Vector4(1, 0.2, 0.2, 1))  -- RGBA red");
        c += wC("title:setCharHeight(0.3)   -- default is 0.2");
        c += wC("title:setAlwaysPresent(true)   -- visible through other geometry");
        c += wBr();
        c += wSub("Positioning");
        c += wC("title:setOffsetPosition(Vector3(0, 2, 0))   -- float above GO");
        c += wC("title:setOffsetOrientation(Vector3(0, 90, 0)) -- rotate label");
        c += wBr();
        c += wSub("Common patterns");
        c += wC("-- Show current energy as label (update each frame or on change):");
        c += wC("title:setCaption(go:getName() .. \"  HP: \" .. hp:getValueNumber())");
        c += wBr();
        c += wC("-- Colour code based on energy level:");
        c += wC("if energy > 50 then");
        c += wC("    title:setColor(Vector4(0, 1, 0, 1))  -- green");
        c += wC("else");
        c += wC("    title:setColor(Vector4(1, 0, 0, 1))  -- red");
        c += wC("end");
        entries.push_back({"32 - GameObjectTitleComponent", c});
    }

    // ── 33. SpeechBubbleComponent ─────────────────────────────────────────
    {
        Ogre::String c;
        c += wH("SpeechBubbleComponent – Speech Bubbles");
        c += wP("Renders a comic-style speech bubble attached to a GameObject. "
                "Requires a GameObjectTitleComponent. Optionally plays a typewriter "
                "sound using a SimpleSoundComponent.");
        c += wBr();
        c += wSub("Basic usage");
        c += wC("local bubble = npc:getSpeechBubbleComponent()");
        c += wC("bubble:setCaption(\"Hello, traveller!\")");
        c += wC("bubble:setActivated(true)   -- show the bubble");
        c += wBr();
        c += wSub("Typewriter effect");
        c += wC("bubble:setRunSpeech(true)        -- reveal text char by char");
        c += wC("bubble:setRunSpeechSound(true)   -- play typing sound per char");
        c += wC("bubble:setKeepCaption(true)      -- keep text after typewriter ends");
        c += wBr();
        c += wSub("Auto-hide after duration");
        c += wC("bubble:setSpeechDuration(4.0)   -- hide after 4 seconds (0 = forever)");
        c += wBr();
        c += wSub("React when speech finishes");
        c += wC("bubble:reactOnSpeechDone(function()");
        c += wC("    bubble:setActivated(false)");
        c += wC("    aiLua:changeState(PatrolState)");
        c += wC("end)");
        c += wBr();
        c += wSub("Text colour");
        c += wC("bubble:setTextColor(Vector3(1, 0.9, 0))  -- RGB (no alpha on this one)");
        c += wBr();
        c += wTip("Call setCaption BEFORE setActivated(true) so the bubble shows "
                  "the correct text from the first frame.");
        entries.push_back({"33 - SpeechBubbleComponent", c});
    }

    // ── 34. TimeLineComponent ────────────────────────────────────────────
    {
        Ogre::String c;
        c += wH("TimeLineComponent – Timed Event Sequencer");
        c += wP("Activates/deactivates other GameObjects at specified time points. "
                "Each time point can also fire a named Lua function on the same "
                "LuaScriptComponent. Perfect for cutscenes, space-game waves, intros.");
        c += wBr();
        c += wSub("Setup (connect)");
        c += wC("timeLine = mainGameObject:getTimeLineComponent()");
        c += wBr();
        c += wSub("Control");
        c += wC("timeLine:setActivated(true)    -- starts the timeline from 0");
        c += wC("timeLine:setActivated(false)   -- pause/stop");
        c += wBr();
        c += wSub("Jump to a time");
        c += wC("-- Jump to 93 seconds (skips to the boss fight):  ");
        c += wC("timeLine:setCurrentTimeSec(93)");
        c += wC("local now = timeLine:getCurrentTimeSec()");
        c += wC("local max = timeLine:getMaxTimeLineDuration()");
        c += wBr();
        c += wSub("Named callbacks (defined in NOWA-Design per time point)");
        c += wC("-- The function name is set in the editor for each time point.");
        c += wC("-- It is called with the time in seconds as argument.");
        c += wC("MyScript[\"onAsteriodTimePoint\"] = function(timePointSec)");
        c += wC("    asteroidSpawn:setActivated(true)");
        c += wC("end");
        c += wBr();
        c += wC("MyScript[\"onBoss1TimePoint\"] = function(timePointSec)");
        c += wC("    boss1:getPhysicsComponent():setCollidable(true)");
        c += wC("    boss1:setVisible(true)");
        c += wC("    boss1:getAiLuaComponent():setActivated(true)");
        c += wC("end");
        c += wBr();
        c += wTip("Each time point in the editor has both a 'GameObjectId' (activated "
                  "automatically) and a 'FunctionName' field for the Lua callback. "
                  "You can use one or both per time point.");
        entries.push_back({"34 - TimeLineComponent", c});
    }

    // ── 35. Core Singleton ────────────────────────────────────────────────
    {
        Ogre::String c;
        c += wH("Core – Project & Scene Info");
        c += wP("Core is a global singleton providing project metadata, timestamps, "
                "save-game slot management, and scene boundary queries.");
        c += wBr();
        c += wSub("Project / scene identity");
        c += wC("local proj  = Core:getProjectName()   -- e.g. \"MyGame\"");
        c += wC("local scene = Core:getSceneName()     -- e.g. \"Level1\"");
        c += wBr();
        c += wSub("Save game slots");
        c += wC("-- All snapshot saves in the project:");
        c += wC("local snaps = Core:getSceneSnapshotsInProject(Core:getProjectName())");
        c += wC("local first = snaps[0]   -- 0-based C++ vector");
        c += wBr();
        c += wC("-- All .sav files:");
        c += wC("local saves = Core:getSaveNamesInProject(Core:getProjectName())");
        c += wBr();
        c += wSub("Current save game slot");
        c += wC("Core:setCurrentSaveGameName(\"MySave1\")");
        c += wC("local slot = Core:getCurrentSaveGameName()");
        c += wBr();
        c += wSub("Timestamp (for unique save names)");
        c += wC("-- Default format: Year_Month_Day_Hour_Minutes_Seconds");
        c += wC("local ts = Core:getCurrentDateAndTime()");
        c += wC("local saveName = ts .. \"_\" .. Core:getSceneName()");
        c += wBr();
        c += wSub("Scene bounds");
        c += wC("local minBound = Core:getCurrentSceneBoundLeftNear()   -- Vector3");
        c += wC("local maxBound = Core:getCurrentSceneBoundRightFar()   -- Vector3");
        c += wBr();
        c += wSub("Editor / game mode check");
        c += wC("if Core:isGame() == true then ... end");
        c += wTip("Combine getCurrentDateAndTime() with the scene name to create unique "
                  "save slot identifiers: Core:getCurrentDateAndTime() .. \"_Level1\".");
        entries.push_back({"35 - Core Singleton", c});
    }

    // ── 36. Global IDs & Constants ────────────────────────────────────────
    {
        Ogre::String c;
        c += wH("Global IDs & Constants");
        c += wP("NOWA exposes several predefined ID constants and module references "
                "that are available globally in every Lua script.");
        c += wBr();
        c += wSub("Reserved GameObject IDs");
        c += wC("-- The main GameObject (scene root, carries global components):");
        c += wC("local main = AppStateManager:getGameObjectController()");
        c += wC("                :getGameObjectFromId(MAIN_GAMEOBJECT_ID)");
        c += wBr();
        c += wC("-- The main camera GameObject:");
        c += wC("local camGO = AppStateManager:getGameObjectController()");
        c += wC("                :getGameObjectFromId(MAIN_CAMERA_ID)");
        c += wBr();
        c += wC("-- The main directional light:");
        c += wC("local lightGO = AppStateManager:getGameObjectController()");
        c += wC("                :getGameObjectFromId(MAIN_LIGHT_ID)");
        c += wBr();
        c += wSub("Physics category sentinel");
        c += wC("-- Match any physics category in ray-casts:");
        c += wC("local hit = phys:getContactBelow(0, Vector3.ZERO, false, ALL_CATEGORIES_ID)");
        c += wBr();
        c += wSub("Global InputDeviceModule (single-player only)");
        c += wC("-- Available without any component lookup:");
        c += wC("if InputDeviceModule:isActionDown(NOWA_A_UP) then ... end");
        c += wBr();
        c += wSub("All NOWA_A_* action constants");
        c += wC("NOWA_A_UP     NOWA_A_DOWN   NOWA_A_LEFT    NOWA_A_RIGHT");
        c += wC("NOWA_A_JUMP   NOWA_A_RUN    NOWA_A_COWER   NOWA_A_DUCK");
        c += wC("NOWA_A_ACTION NOWA_A_SELECT NOWA_A_START   NOWA_A_MAP");
        c += wC("NOWA_A_ATTACK_1  NOWA_A_ATTACK_2  NOWA_A_ATTACK_3  NOWA_A_ATTACK_4");
        c += wC("NOWA_A_FLASH_LIGHT");
        c += wBr();
        c += wTip("For multi-player input, always use the per-object InputDeviceModule "
                  "(gameObject:getInputDeviceComponent():getInputDeviceModule()). "
                  "The global InputDeviceModule only reads player 1.");
        entries.push_back({"36 - Global IDs & Constants", c});
    }

    // ── 37. InputDeviceModule (Full Reference) ────────────────────────────
    {
        Ogre::String c;
        c += wH("InputDeviceModule – Full Input Reference");
        c += wP("Wraps keyboard and joystick input. Get the per-object instance "
                "from InputDeviceComponent for multi-player safety.");
        c += wBr();
        c += wSub("Getting the module");
        c += wC("-- Per-object (multi-player safe, recommended):");
        c += wC("inputDeviceModule = go:getInputDeviceComponent():getInputDeviceModule()");
        c += wC("-- Global (single-player only):");
        c += wC("-- InputDeviceModule  (always available)");
        c += wBr();
        c += wSub("Digital actions (keyboard or joystick button)");
        c += wC("isActionDown(NOWA_A_UP)              -- held");
        c += wC("isActionDownAmount(NOWA_A_JUMP, dt, 0.2) -- held up to 0.2 s");
        c += wC("isActionPressed(NOWA_A_ACTION, dt, 0.3)  -- repeated press every 0.3 s");
        c += wBr();
        c += wSub("Raw key / button");
        c += wC("isKeyDown(KeyEvent.KC_F)             -- specific keyboard key");
        c += wC("isButtonDown(JoyStickButton.BUTTON_0)");
        c += wBr();
        c += wSub("Analog axes (joystick or keyboard simulation)");
        c += wC("local axis = getSteerAxis()          -- [-1..1] left/right");
        c += wC("local h    = getLeftStickHorizontalMovingStrength()");
        c += wC("local v    = getLeftStickVerticalMovingStrength()");
        c += wBr();
        c += wSub("Configuration");
        c += wC("setAnalogActionThreshold(0.15)       -- dead-zone for stick-as-digital");
        c += wC("setJoyStickDeadZone(0.1)");
        c += wBr();
        c += wSub("Device info");
        c += wC("local name = getDeviceName()         -- e.g. \"Win32InputManager\"");
        c += wC("if isKeyboardDevice() then ... end   -- keyboard vs joystick");
        c += wC("if hasActiveJoyStick() then ... end");
        c += wBr();
        c += wSub("Multi-button combos (joystick)");
        c += wC("areButtonsDown2(JoyStickButton.BUTTON_0, JoyStickButton.BUTTON_1)");
        c += wBr();
        c += wSub("Get what key is mapped to an action");
        c += wC("local keyStr = getStringFromMappedKey(NOWA_A_JUMP)  -- e.g. \"Space\"");
        entries.push_back({"37 - InputDeviceModule (Full Reference)", c});
    }

    // ── 38. Variant increment/decrement helpers ───────────────────────────
    {
        Ogre::String c;
        c += wH("Variant – increment / decrement helpers");
        c += wP("AttributeValue (Variant) objects support arithmetic helpers "
                "so you don't have to read-modify-write manually each time.");
        c += wBr();
        c += wSub("Pattern without helpers (verbose)");
        c += wC("energy:setValueNumber(energy:getValueNumber() - 10)");
        c += wBr();
        c += wSub("Check which helpers exist first");
        c += wC("-- The API exposes these on Variant (check auto-complete in editor):");
        c += wC("energy:incrementValueNumber(1)      -- +1");
        c += wC("energy:decrementValueNumber(10)     -- -10");
        c += wBr();
        c += wSub("Clamp after change");
        c += wC("energy:decrementValueNumber(damage)");
        c += wC("if energy:getValueNumber() < 0 then");
        c += wC("    energy:setValueNumber(0)");
        c += wC("end");
        c += wBr();
        c += wSub("Animated counter (smooth count-up / count-down)");
        c += wC("-- Interpolate display value toward target every frame:");
        c += wC("local target = 2000");
        c += wC("local elapsed = 0");
        c += wC("-- In update(dt):");
        c += wC("elapsed = elapsed + dt");
        c += wC("local t = math.min(elapsed / 1.5, 1.0)  -- 1.5s duration");
        c += wC("local display = math.floor(start + (target - start) * t)");
        c += wC("goldAttribute:setValueNumber(display)");
        c += wC("goldText:setCaption(\"Gold: \" .. display)");
        c += wBr();
        c += wTip("The animated counter pattern is useful for score pop-ups, "
                  "gold doubling, and any case where you want the number "
                  "to visually count up rather than jump instantly.");
        entries.push_back({"38 - Variant Arithmetic & Animated Counters", c});
    }

    // ── 39. Goal System – Full Reference & Townhall Example ───────────────
    {
        Ogre::String c;
        c += wH("Goal System – Full Reference & Townhall Scenario");
        c += wP("The Goal system is for long, multi-step autonomous behaviours. "
                "Goals chain sequentially inside a root composite. Each goal table "
                "defines activate / process / terminate callbacks.");
        c += wBr();

        c += wSub("How the C++ drives goals (important to understand)");
        c += wC("-- 1. Simulation starts → AiLuaGoalComponent finds the Lua table");
        c += wC("--    whose name matches 'RootGoalName' in NOWA-Design.");
        c += wC("-- 2. C++ creates a LuaGoalComposite wrapping it and calls activate().");
        c += wC("-- 3. Every frame: C++ calls process(dt) on the composite,");
        c += wC("--    which calls process(dt) on the FRONT sub-goal.");
        c += wC("-- 4. When a sub-goal sets COMPLETED it is terminated and removed.");
        c += wC("--    The next sub-goal becomes the new front.");
        c += wC("-- 5. When all sub-goals are done the root returns COMPLETED,");
        c += wC("--    reactivateIfFailed() resets it → activate() fires again.");
        c += wBr();

        c += wSub("Callback signatures (exact – must not differ)");
        c += wC("GoalX[\"activate\"]  = function(gameObject, goalResult)   end");
        c += wC("GoalX[\"process\"]   = function(gameObject, dt, goalResult) end  -- 3 args");
        c += wC("GoalX[\"terminate\"] = function(gameObject, goalResult)   end");
        c += wBr();

        c += wSub("Townhall scenario – Player (4 sequential goals)");
        c += wP("Player picks a build spot → walks there → builds Townhall → spawns Worker.");
        c += wBr();
        c += wC("-- In connect(): only setup, no goal calls");
        c += wC("aiGoalComp = gameObject:getAiLuaGoalComponent()");
        c += wC("aiGoalComp:getMovingBehavior():setBehavior(BehaviorType.STOP)");
        c += wBr();
        c += wC("-- Root goal (name matches RootGoalName attribute in editor)");
        c += wC("TownhallRootGoal = {}");
        c += wC("TownhallRootGoal[\"activate\"] = function(gameObject, goalResult)");
        c += wC("    goalResult = AppStateManager:getGameObjectController()");
        c += wC("                     :castGoalResult(goalResult)");
        c += wC("    aiGoalComp:addSubGoal(WaitForPlacementGoal)  -- 1");
        c += wC("    aiGoalComp:addSubGoal(WalkToBuildSiteGoal)   -- 2");
        c += wC("    aiGoalComp:addSubGoal(ConstructBuildingGoal) -- 3");
        c += wC("    aiGoalComp:addSubGoal(SpawnWorkerGoal)       -- 4");
        c += wC("end");
        c += wBr();
        c += wC("-- Goal 1: wait until player presses [E] to mark build position");
        c += wC("WaitForPlacementGoal = {}");
        c += wC("WaitForPlacementGoal[\"activate\"] = function(gameObject, goalResult)");
        c += wC("    goalResult = AppStateManager:getGameObjectController()");
        c += wC("                     :castGoalResult(goalResult)");
        c += wC("    goalResult:setStatus(GoalResult.ACTIVE)");
        c += wC("end");
        c += wC("WaitForPlacementGoal[\"process\"] = function(gameObject, dt, goalResult)");
        c += wC("    goalResult = AppStateManager:getGameObjectController()");
        c += wC("                     :castGoalResult(goalResult)");
        c += wC("    if buildPosition ~= nil then");
        c += wC("        goalResult:setStatus(GoalResult.COMPLETED)");
        c += wC("    else");
        c += wC("        goalResult:setStatus(GoalResult.ACTIVE)");
        c += wC("    end");
        c += wC("end");
        c += wC("WaitForPlacementGoal[\"terminate\"] = function(gameObject, goalResult)");
        c += wC("    goalResult = AppStateManager:getGameObjectController()");
        c += wC("                     :castGoalResult(goalResult)");
        c += wC("end");
        c += wBr();
        c += wC("-- Goal 2: walk to build position using FOLLOW_PATH + reactOnPathGoalReached");
        c += wC("WalkToBuildSiteGoal = {}");
        c += wC("WalkToBuildSiteGoal._arrived = false");
        c += wC("WalkToBuildSiteGoal[\"activate\"] = function(gameObject, goalResult)");
        c += wC("    goalResult = AppStateManager:getGameObjectController()");
        c += wC("                     :castGoalResult(goalResult)");
        c += wC("    WalkToBuildSiteGoal._arrived = false");
        c += wC("    goalResult:setStatus(GoalResult.ACTIVE)");
        c += wC("    aiGoalComp:getMovingBehavior():getPath():addWayPoint(buildPosition)");
        c += wC("    aiGoalComp:getMovingBehavior():setBehavior(BehaviorType.FOLLOW_PATH)");
        c += wC("    aiGoalComp:reactOnPathGoalReached(function()");
        c += wC("        WalkToBuildSiteGoal._arrived = true");
        c += wC("    end)");
        c += wC("end");
        c += wC("WalkToBuildSiteGoal[\"process\"] = function(gameObject, dt, goalResult)");
        c += wC("    goalResult = AppStateManager:getGameObjectController()");
        c += wC("                     :castGoalResult(goalResult)");
        c += wC("    if WalkToBuildSiteGoal._arrived then");
        c += wC("        goalResult:setStatus(GoalResult.COMPLETED)");
        c += wC("    else");
        c += wC("        goalResult:setStatus(GoalResult.ACTIVE)");
        c += wC("    end");
        c += wC("end");
        c += wC("WalkToBuildSiteGoal[\"terminate\"] = function(gameObject, goalResult)");
        c += wC("    goalResult = AppStateManager:getGameObjectController()");
        c += wC("                     :castGoalResult(goalResult)");
        c += wC("    aiGoalComp:getMovingBehavior():setBehavior(BehaviorType.STOP)");
        c += wC("end");
        c += wBr();
        c += wC("-- Goal 3: activate MeshConstructionComponent, wait for done callback");
        c += wC("ConstructBuildingGoal = {}");
        c += wC("ConstructBuildingGoal._done = false");
        c += wC("ConstructBuildingGoal[\"activate\"] = function(gameObject, goalResult)");
        c += wC("    goalResult = AppStateManager:getGameObjectController()");
        c += wC("                     :castGoalResult(goalResult)");
        c += wC("    ConstructBuildingGoal._done = false");
        c += wC("    goalResult:setStatus(GoalResult.ACTIVE)");
        c += wC("    local mc = townhallTemplate:getMeshConstructionComponent()");
        c += wC("    mc:setConstructionTime(5.0)");
        c += wC("    mc:setActivated(true)");
        c += wC("    mc:reactOnConstructionDone(function()");
        c += wC("        ConstructBuildingGoal._done = true");
        c += wC("    end)");
        c += wC("end");
        c += wC("ConstructBuildingGoal[\"process\"] = function(gameObject, dt, goalResult)");
        c += wC("    goalResult = AppStateManager:getGameObjectController()");
        c += wC("                     :castGoalResult(goalResult)");
        c += wC("    if ConstructBuildingGoal._done then");
        c += wC("        goalResult:setStatus(GoalResult.COMPLETED)");
        c += wC("    else");
        c += wC("        goalResult:setStatus(GoalResult.ACTIVE)");
        c += wC("    end");
        c += wC("end");
        c += wC("ConstructBuildingGoal[\"terminate\"] = function(gameObject, goalResult)");
        c += wC("    goalResult = AppStateManager:getGameObjectController()");
        c += wC("                     :castGoalResult(goalResult)");
        c += wC("end");
        c += wBr();

        c += wSub("Worker cycling scenario (wander → rest → repeat)");
        c += wC("WorkerRootGoal = {}");
        c += wC("WorkerRootGoal[\"activate\"] = function(gameObject, goalResult)");
        c += wC("    goalResult = AppStateManager:getGameObjectController()");
        c += wC("                     :castGoalResult(goalResult)");
        c += wC("    aiGoalComp:addSubGoal(WanderGoal)");
        c += wC("    aiGoalComp:addSubGoal(RestGoal)");
        c += wC("end");
        c += wBr();
        c += wC("-- Re-add goals from the last goal's terminate() to create a cycle:");
        c += wC("RestGoal[\"terminate\"] = function(gameObject, goalResult)");
        c += wC("    goalResult = AppStateManager:getGameObjectController()");
        c += wC("                     :castGoalResult(goalResult)");
        c += wC("    aiGoalComp:addSubGoal(WanderGoal)  -- restart cycle");
        c += wC("    aiGoalComp:addSubGoal(RestGoal)");
        c += wC("end");
        c += wBr();
        c += wWarn("Never return GoalResult.ACTIVE from process() — the return value is "
                   "completely ignored by C++. Only goalResult:setStatus() matters.");
        c += wTip("See Townhall_Player_0.lua and Townhall_Worker_0.lua for the complete "
                  "working scenario with all four goals and the worker cycling pattern.");
        entries.push_back({"39 - Goal System Full Reference", c});
    }

    // ── 40. AiPathFollowComponent ─────────────────────────────────────────
    {
        Ogre::String c;
        c += wH("AiPathFollowComponent – Structured Waypoint Paths");
        c += wP("AiPathFollowComponent makes an agent follow a pre-defined sequence of "
                "NodeComponent GameObjects. Unlike AiLuaGoalComponent FOLLOW_PATH "
                "(which targets arbitrary Vector3 positions), this component uses "
                "stable scene objects as waypoints — ideal for patrol routes, "
                "race tracks, and production lines.");
        c += wBr();

        c += wSub("Key difference from AiLuaGoalComponent");
        c += wC("-- AiLuaGoalComponent FOLLOW_PATH: targets any Vector3 (dynamic/runtime)");
        c += wC("-- AiPathFollowComponent:           targets NodeComponent GOs (design-time)");
        c += wBr();

        c += wSub("Setup (connect)");
        c += wC("local pathFollow = gameObject:getAiPathFollowComponent()");
        c += wBr();
        c += wC("-- Set total number of waypoints:");
        c += wC("pathFollow:setWaypointsCount(4)");
        c += wBr();
        c += wC("-- Assign NodeComponent GameObject IDs to each slot:");
        c += wC("pathFollow:setWaypointId(0, \"PatrolNode_0\")   -- index, GO id");
        c += wC("pathFollow:setWaypointId(1, \"PatrolNode_1\")");
        c += wC("pathFollow:setWaypointId(2, \"PatrolNode_2\")");
        c += wC("pathFollow:setWaypointId(3, \"PatrolNode_3\")");
        c += wBr();
        c += wC("-- Or add waypoints one by one (no index needed):");
        c += wC("pathFollow:addWaypointId(\"PatrolNode_0\")");
        c += wC("pathFollow:addWaypointId(\"PatrolNode_1\")");
        c += wBr();
        c += wC("-- Read a waypoint id back:");
        c += wC("local id = pathFollow:getWaypointId(2)");
        c += wBr();

        c += wSub("Path behaviour flags");
        c += wC("pathFollow:setRepeat(true)           -- loop: 0→1→2→3→0→...");
        c += wC("pathFollow:setDirectionChange(true)  -- bounce: 0→1→2→3→2→1→0→...");
        c += wC("pathFollow:setGoalRadius(1.0)        -- arrival tolerance in units");
        c += wBr();

        c += wSub("Start / stop");
        c += wC("pathFollow:setActivated(true)    -- start following the path");
        c += wC("pathFollow:setActivated(false)   -- pause/stop");
        c += wBr();

        c += wSub("Generating many waypoints via the Lua console (^ key)");
        c += wP("If you have dozens of NodeComponent objects and setting them "
                "manually is tedious, run this once in the Lua console. "
                "Adapt the GameObject ID to your agent.");
        c += wC("local go   = AppStateManager:getGameObjectController()");
        c += wC("               :getGameObjectFromId('YOUR_AGENT_ID')");
        c += wC("local pf   = go:getAiPathFollowComponent()");
        c += wC("local nodes = AppStateManager:getGameObjectController()");
        c += wC("               :getGameObjectsFromComponent('NodeComponent')");
        c += wC("pf:setWaypointsCount(#nodes + 1)");
        c += wC("for i = 0, #nodes do");
        c += wC("    pf:setWaypointId(i, nodes[i]:getId())");
        c += wC("end");
        c += wBr();

        c += wSub("Coexistence with AiLuaGoalComponent");
        c += wC("-- Only ONE movement driver should be active at a time.");
        c += wC("-- When switching from goal-driven to path-follow:");
        c += wC("aiGoalComp:getMovingBehavior():setBehavior(BehaviorType.NONE)");
        c += wC("aiGoalComp:setActivated(false)");
        c += wC("pathFollow:setActivated(true)");
        c += wBr();
        c += wC("-- Switching back:");
        c += wC("pathFollow:setActivated(false)");
        c += wC("aiGoalComp:setActivated(true)");
        c += wBr();

        c += wSub("AiPathFollowComponent2D (2-D platformers)");
        c += wC("local pf2d = gameObject:getAiPathFollowComponent2D()");
        c += wC("-- Same API. Uses 2D movement (X/Z only, ignores Y).");
        c += wC("-- Used with BehaviorType.FOLLOW_PATH_2D / PURSUIT_2D etc.");
        c += wBr();

        c += wTip("Place NodeComponent GameObjects along your desired patrol route "
                  "in NOWA-Design, give them descriptive names (PatrolNode_0..N), "
                  "then use setWaypointsCount + setWaypointId in connect(). "
                  "Use setRepeat(true) for infinite loops and setDirectionChange(true) "
                  "for a ping-pong patrol.");
        entries.push_back({"40 - AiPathFollowComponent", c});
    }

    // ── 41. Inventory – Click, Drag & Place Patterns ──────────────────────
    {
        Ogre::String c;
        c += wH("Inventory – Click, Drag-Drop & Item Placement");
        c += wP("MyGUIItemBoxComponent is a full drag-and-drop inventory. "
                "It supports mouse-button callbacks, drop validation, "
                "purchase logic, and item pickup via physics contact.");
        c += wBr();

        c += wSub("Show / hide inventory");
        c += wC("local box = gameObject:getMyGUIItemBoxComponent()");
        c += wC("box:setActivated(true)   -- show");
        c += wC("box:setActivated(false)  -- hide");
        c += wBr();

        c += wSub("React to mouse click on an item");
        c += wC("-- mouseButtonId: Mouse.MB_LEFT = 0, Mouse.MB_RIGHT = 1");
        c += wC("box:reactOnMouseButtonClick(function(resourceName, mouseButtonId)");
        c += wC("    if resourceName == \"TownhallItem\" and mouseButtonId == Mouse.MB_RIGHT then");
        c += wC("        if box:getQuantity(\"TownhallItem\") > 0 then");
        c += wC("            placementMode = true  -- enter build-placement mode");
        c += wC("        end");
        c += wC("    end");
        c += wC("end)");
        c += wBr();

        c += wSub("Drag-drop: validate before accepting");
        c += wC("box:reactOnDropItemRequest(function(dragDropData)");
        c += wC("    -- Allow internal reorder:");
        c += wC("    if dragDropData:getSenderReceiverIsSame() then");
        c += wC("        dragDropData:setCanDrop(true)");
        c += wC("    -- Block coins from being bought:");
        c += wC("    elseif dragDropData:getResourceName() == \"CoinItem\" then");
        c += wC("        dragDropData:setCanDrop(false)");
        c += wC("    else");
        c += wC("        dragDropData:setCanDrop(true)");
        c += wC("    end");
        c += wC("end)");
        c += wBr();

        c += wSub("Drag-drop accepted: execute purchase");
        c += wC("box:reactOnDropItemAccepted(function(dragDropData)");
        c += wC("    local shopId = dragDropData:getSenderInventoryId()");
        c += wC("    local price  = dragDropData:getBuyValue()");
        c += wC("    local coins  = box:getQuantity(\"CoinItem\")");
        c += wC("    local value  = coins * box:getSellValue(\"CoinItem\")");
        c += wBr();
        c += wC("    if value >= price then");
        c += wC("        box:removeQuantity(\"CoinItem\", price)");
        c += wC("        -- Give coins to the shop:");
        c += wC("        local shopBox = AppStateManager:getGameObjectController()");
        c += wC("                            :getGameObjectFromId(shopId)");
        c += wC("                            :getMyGUIItemBoxComponent()");
        c += wC("        shopBox:increaseQuantity(\"CoinItem\", price)");
        c += wC("        shopBox:decreaseQuantity(dragDropData:getResourceName(), 1)");
        c += wC("    else");
        c += wC("        dragDropData:setCanDrop(false)  -- reject");
        c += wC("    end");
        c += wC("end)");
        c += wBr();

        c += wSub("Pick up items via physics contact");
        c += wC("MyScript[\"onContact\"] = function(go0, go1, contact)");
        c += wC("    go0 = AppStateManager:getGameObjectController():castGameObject(go0)");
        c += wC("    go1 = AppStateManager:getGameObjectController():castGameObject(go1)");
        c += wC("    if go1:getCategory() ~= \"Item\" then return end");
        c += wBr();
        c += wC("    local inv = go1:getInventoryItemComponent()");
        c += wC("    if inv ~= nil then");
        c += wC("        -- addQuantityToInventory(receiverId, qty, autoRemoveFromWorld)");
        c += wC("        inv:addQuantityToInventory(player:getId(), 1, true)");
        c += wC("        AppStateManager:getGameObjectController()");
        c += wC("            :deleteGameObject(go1:getId())");
        c += wC("    end");
        c += wC("end");
        c += wBr();

        c += wSub("Quantity helpers");
        c += wC("box:getQuantity(\"SwordItem\")           -- how many in inventory");
        c += wC("box:getSellValue(\"CoinItem\")           -- sell price of one coin");
        c += wC("box:removeQuantity(\"PotionItem\", 1)    -- remove N items");
        c += wC("box:increaseQuantity(\"CoinItem\", 10)   -- add N items");
        c += wC("box:decreaseQuantity(\"ArrowItem\", 5)   -- remove N (clamped to 0)");
        c += wBr();

        c += wSub("Proximity shop (fade-in inventory when player is near)");
        c += wC("-- On the shop GameObject:");
        c += wC("areaOfInterest:reactOnEnter(function(visitorGO)");
        c += wC("    fadeAlphaCtrl:setAlpha(1)");
        c += wC("    fadeAlphaCtrl:setActivated(true)");
        c += wC("    itemBox:setActivated(true)  -- show shop inventory");
        c += wC("end)");
        c += wC("areaOfInterest:reactOnLeave(function(visitorGO)");
        c += wC("    fadeAlphaCtrl:setAlpha(0)");
        c += wC("    fadeAlphaCtrl:setActivated(true)");
        c += wC("    itemBox:setActivated(false) -- hide when player leaves");
        c += wC("end)");
        c += wBr();

        c += wSub("dragDropData fields");
        c += wC("dragDropData:getResourceName()       -- item id string");
        c += wC("dragDropData:getSenderInventoryId()  -- source inventory GO id");
        c += wC("dragDropData:getSenderReceiverIsSame() -- true = internal reorder");
        c += wC("dragDropData:getBuyValue()           -- item price");
        c += wC("dragDropData:getCanDrop()            -- current permission");
        c += wC("dragDropData:setCanDrop(bool)        -- allow or block the drop");
        c += wBr();

        c += wTip("Always check quantity before entering placement or purchase mode. "
                  "Use getSenderReceiverIsSame() in reactOnDropItemRequest to allow "
                  "internal reordering without triggering purchase logic.");
        entries.push_back({"41 - Inventory Click, Drag-Drop & Placement", c});
    }
}
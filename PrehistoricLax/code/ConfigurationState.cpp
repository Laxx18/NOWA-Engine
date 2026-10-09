#include "NOWAPrecompiled.h"
#include "ConfigurationState.h"
#include "MyGUI_InputManager.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>

using namespace NOWA;

// Which InputDeviceModule::Action each row of the controls tab stands for, and its label.
// Attention: Never derive the action from the row index via static_cast<InputDeviceModule::Action>(i).
// The enum has other entries between the rows (e.g. RUN and COWER sit between JUMP and ATTACK_1), which once made
// "Action 2" rebind COWER instead of ATTACK_2. This table is the ONE place that says what a row is: createControlsTab (label),
// populateControlsOptions (read the current bindings) and applyControlsSettings (write them back) all use it, so they cannot drift apart.
// The row count comes from the table itself, so a new action is a single new line here.
// (ConfigurationState::ACTION_COUNT is private to the class and is not used anymore.)
namespace
{
    struct ActionRow
    {
        InputDeviceModule::Action action;
        const char* label;
    };

    const ActionRow actionRows[] = {
        { InputDeviceModule::UP, "Move up:" },
        { InputDeviceModule::DOWN, "Move down:" },
        { InputDeviceModule::LEFT, "Move left:" },
        { InputDeviceModule::RIGHT, "Move right:" },
        { InputDeviceModule::JUMP, "Jump:" },
        { InputDeviceModule::RUN, "Run:" },
        { InputDeviceModule::COWER, "Cower:" },
        { InputDeviceModule::ATTACK_1, "Attack 1:" },
        { InputDeviceModule::ATTACK_2, "Attack 2:" },
        { InputDeviceModule::DUCK, "Duck:" },
        { InputDeviceModule::SNEAK, "Sneak:" },
        { InputDeviceModule::ACTION, "Action:" },
        { InputDeviceModule::RELOAD, "Reload:" },
        { InputDeviceModule::INVENTORY, "Inventory:" },
        { InputDeviceModule::MAP, "Map:" },
        { InputDeviceModule::SELECT, "Select:" },
        { InputDeviceModule::START, "Start:" },
        { InputDeviceModule::PAUSE, "Pause:" },
        { InputDeviceModule::SAVE, "Save:" },
        { InputDeviceModule::LOAD, "Load:" },
        { InputDeviceModule::CAMERA_FORWARD, "Camera forward:" },
        { InputDeviceModule::CAMERA_BACKWARD, "Camera backward:" },
        { InputDeviceModule::CAMERA_LEFT, "Camera left:" },
        { InputDeviceModule::CAMERA_RIGHT, "Camera right:" },
        { InputDeviceModule::CAMERA_UP, "Camera up:" },
        { InputDeviceModule::CAMERA_DOWN, "Camera down:" },
        { InputDeviceModule::CONSOLE, "Console:" },
        { InputDeviceModule::WEAPON_CHANGE_FORWARD, "Next weapon:" },
        { InputDeviceModule::WEAPON_CHANGE_BACKWARD, "Previous weapon:" },
        { InputDeviceModule::FLASH_LIGHT, "Flash light:" },
        { InputDeviceModule::GRID, "Grid:" }
    };

    const unsigned short ACTION_ROW_COUNT = static_cast<unsigned short>(sizeof(actionRows) / sizeof(actionRows[0]));
}

namespace
{
    // Binds the given caption to the currently active (focused) textbox.
    // Used for gamepad buttons and triggers (keyboard keys are handled directly in keyPressed).
    // A gamepad has only a handful of physical buttons, so a button which is already used by another action is TAKEN OVER: that other action becomes "None".
    // Refusing it (as before) would make it impossible to ever move e.g. Select to another action, because no free button would be left to move the old action to.
    template <class TextboxVector, class ActiveVector>
    void bindCaptionToActiveTextbox(TextboxVector& textboxes, ActiveVector& textboxActive, const Ogre::String& caption)
    {
        for (size_t i = 0; i < textboxes.size(); i++)
        {
            if (true == textboxActive[i])
            {
                if (false == caption.empty())
                {
                    for (size_t j = 0; j < textboxes.size(); j++)
                    {
                        if (j != i && textboxes[j]->getCaption() == caption)
                        {
                            textboxes[j]->setCaption("None");
                        }
                    }

                    textboxes[i]->setCaption(caption);
                }

                textboxActive[i] = false;
                textboxes[i]->setTextShadow(false);
                break;
            }
        }
    }
}

namespace
{
    // =============================================================================
    // Gamepad navigation
    //
    // Same idea as MyGUIPadFocusComponent, but for the widgets which are created directly in this state: the mouse pointer is snapped onto
    // the focused widget (MyGUI then draws its own hover state) and a confirm press becomes a real click there.
    // Three things differ from the component, because this screen is not only made of buttons:
    //  - combo boxes and sliders are changed with LEFT / RIGHT (a click in their middle would be useless),
    //  - the rows of the controls tab live in a scroll view, which is scrolled so that the focused row stays visible,
    //  - a rebinding text box is NOT armed by the hover (see notifyKeyEditFocus), the confirm button arms it explicitly.
    // Everything is file local on purpose: no member is added, so ConfigurationState.h does not have to change.
    // =============================================================================

    struct PadNavigation
    {
        // Logic thread only
        float navigationTimer;
        bool confirmWasDown;
        bool engaged;
        bool armedByPad;
        float armedTimer;
        bool soundSliderChanged;

        // Render thread only
        MyGUI::Widget* focused;

        // Written by the logic thread, read by the MyGUI hover events (render thread)
        std::atomic<long long> suppressHoverUntilMs;

        PadNavigation()
        {
            this->reset();
        }

        void reset(void)
        {
            this->navigationTimer = 0.0f;
            this->confirmWasDown = false;
            this->engaged = false;
            this->armedByPad = false;
            this->armedTimer = 0.0f;
            this->soundSliderChanged = false;
            this->focused = nullptr;
            this->suppressHoverUntilMs.store(0);
        }
    };

    PadNavigation padNav;

    struct PadResult
    {
        bool snap = false;
        bool click = false;
        bool armed = false;
        bool soundSliderAdjusted = false;
        int x = 0;
        int y = 0;
        float repeatDelay = 0.25f;
    };

    const float PAD_SLIDER_DELAY = 0.08f;
    const float PAD_ARMED_TIMEOUT = 6.0f;
    const long long PAD_HOVER_SUPPRESS_MS = 1000;

    long long nowMilliseconds(void)
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    // True shortly after the gamepad moved the pointer: such a hover must not arm a rebinding box.
    bool isHoverArmingSuppressed(void)
    {
        return nowMilliseconds() < padNav.suppressHoverUntilMs.load();
    }

    // Only the gamepads, never the keyboard: the arrow keys would otherwise also be bound to an armed textbox.
    bool readPadDirection(int& directionX, int& directionY)
    {
        directionX = 0;
        directionY = 0;

        std::vector<InputDeviceModule*> joystickModules = InputDeviceCore::getSingletonPtr()->getJoystickInputDeviceModules();

        for (size_t i = 0; i < joystickModules.size(); i++)
        {
            InputDeviceModule* joystickModule = joystickModules[i];
            if (nullptr == joystickModule)
            {
                continue;
            }

            if (0 == directionX)
            {
                if (true == joystickModule->isActionDown(InputDeviceModule::LEFT))
                {
                    directionX = -1;
                }
                else if (true == joystickModule->isActionDown(InputDeviceModule::RIGHT))
                {
                    directionX = 1;
                }
            }

            if (0 == directionY)
            {
                if (true == joystickModule->isActionDown(InputDeviceModule::UP))
                {
                    directionY = -1;
                }
                else if (true == joystickModule->isActionDown(InputDeviceModule::DOWN))
                {
                    directionY = 1;
                }
            }
        }

        // A diagonal stick: up / down wins, so a slider is only changed by a clear left / right.
        if (0 != directionY)
        {
            directionX = 0;
        }

        return (0 != directionX || 0 != directionY);
    }

    // JUMP, the same button the virtual keyboard uses to press a key.
    bool readPadConfirm(void)
    {
        std::vector<InputDeviceModule*> joystickModules = InputDeviceCore::getSingletonPtr()->getJoystickInputDeviceModules();

        for (size_t i = 0; i < joystickModules.size(); i++)
        {
            if (nullptr != joystickModules[i] && true == joystickModules[i]->isActionDown(InputDeviceModule::JUMP))
            {
                return true;
            }
        }

        return false;
    }

    // Render thread. Live coordinates, because a scroll view moves its rows.
    MyGUI::IntPoint centerOf(MyGUI::Widget* widget)
    {
        const MyGUI::IntCoord coord = widget->getAbsoluteCoord();
        return MyGUI::IntPoint(coord.left + coord.width / 2, coord.top + coord.height / 2);
    }

    // Render thread. 45 degree cone like in the component. If nothing lies in the cone (e.g. from the last combo box down to the
    // Apply button, which sits far to the left), the nearest widget in that direction is taken instead of getting stuck.
    MyGUI::Widget* findNeighbour(const std::vector<MyGUI::Widget*>& targets, MyGUI::Widget* current, int directionX, int directionY)
    {
        const MyGUI::IntPoint currentCenter = centerOf(current);

        MyGUI::Widget* bestInCone = nullptr;
        int bestInConeScore = 0;
        MyGUI::Widget* bestAny = nullptr;
        int bestAnyScore = 0;

        for (size_t i = 0; i < targets.size(); i++)
        {
            if (targets[i] == current)
            {
                continue;
            }

            const MyGUI::IntPoint center = centerOf(targets[i]);
            const int deltaX = center.left - currentCenter.left;
            const int deltaY = center.top - currentCenter.top;

            const int along = deltaX * directionX + deltaY * directionY;
            const int sideways = std::abs(deltaX * directionY - deltaY * directionX);

            if (0 >= along)
            {
                continue;
            }

            const int score = along + 2 * sideways;

            if (sideways <= along && (nullptr == bestInCone || score < bestInConeScore))
            {
                bestInCone = targets[i];
                bestInConeScore = score;
            }

            if (nullptr == bestAny || score < bestAnyScore)
            {
                bestAny = targets[i];
                bestAnyScore = score;
            }
        }

        return (nullptr != bestInCone) ? bestInCone : bestAny;
    }

    // Render thread. If the widget sits inside a scroll view (the rows of the controls tab), the view is scrolled until the widget is visible.
    void scrollIntoView(MyGUI::Widget* widget)
    {
        MyGUI::ScrollView* scrollView = nullptr;
        for (MyGUI::Widget* parent = widget->getParent(); nullptr != parent; parent = parent->getParent())
        {
            scrollView = parent->castType<MyGUI::ScrollView>(false);
            if (nullptr != scrollView)
            {
                break;
            }
        }

        if (nullptr == scrollView)
        {
            return;
        }

        // > 0: the widget is above the view and the content has to move down, < 0: below it and the content has to move up.
        auto missingShift = [scrollView, widget]() -> int
        {
            const MyGUI::IntCoord view = scrollView->getAbsoluteCoord();
            const int top = widget->getAbsoluteTop();
            const int bottom = top + widget->getHeight();

            if (top < view.top)
            {
                return view.top - top;
            }
            if (bottom > view.top + view.height)
            {
                return (view.top + view.height) - bottom;
            }
            return 0;
        };

        const int shift = missingShift();
        if (0 == shift)
        {
            return;
        }

        const MyGUI::IntPoint offset = scrollView->getViewOffset();

        // The sign convention of the view offset is not relied on: try one direction, and if the widget did not come closer, the other one.
        scrollView->setViewOffset(MyGUI::IntPoint(offset.left, offset.top + shift));
        if (std::abs(missingShift()) >= std::abs(shift))
        {
            scrollView->setViewOffset(MyGUI::IntPoint(offset.left, offset.top - shift));
            if (std::abs(missingShift()) >= std::abs(shift))
            {
                scrollView->setViewOffset(offset);
            }
        }
    }
}

ConfigurationState::ConfigurationState()
    : AppState()
{
    // Do not init anything here
}

void ConfigurationState::enter(void)
{
    this->rootWindow = nullptr;
    this->tabGraphicsButton = nullptr;
    this->tabSoundButton = nullptr;
    this->tabControlsButton = nullptr;
    this->graphicsPanel = nullptr;
    this->soundPanel = nullptr;
    this->controlsPanel = nullptr;
    this->restartRequiredLabel = nullptr;
    this->applyButton = nullptr;
    this->okButton = nullptr;
    this->cancelButton = nullptr;
    this->currentTabIndex = 0;

    this->resolutionCombo = nullptr;
    this->fullscreenCheck = nullptr;
    this->vsyncCheck = nullptr;
    this->vsyncIntervalCombo = nullptr;
    this->fsaaCombo = nullptr;
    this->shadowQualityCombo = nullptr;
    this->graphicsRestartRequired = false;

    this->soundSlider = nullptr;
    this->musicSlider = nullptr;
    this->soundLabel = nullptr;
    this->musicLabel = nullptr;
    this->menuMusic = nullptr;
    this->soundMusic = nullptr;

    this->hasJoystick = false;
    padNav.reset();

    Ogre::LogManager::getSingletonPtr()->logMessage("Entering ConfigurationState...");

    // Attention: currentSceneName is intentionally left empty. AppState::enter() then takes
    // the scene-less branch: it calls this->initializeModules(true, true) directly from the
    // LOGIC thread (which internally issues its own enqueueAndWait calls for scene manager,
    // camera and workspace creation - that is safe, because it runs on the logic thread).
    // It then calls this->start(sceneParameter) synchronously, still on the logic thread.
    //
    // Previously this function created the scene manager/camera/workspace itself INSIDE a
    // RenderCommand that was already running on the render thread via enqueueAndWait, and
    // additionally called this->initializeModules(false, false) from within that same
    // render-thread lambda. initializeModules() issues its OWN enqueueAndWait calls
    // internally, so calling it from the render thread caused a nested enqueueAndWait onto
    // the same thread - the render thread ends up waiting on itself, which either hangs it
    // completely or silently drops the inner commands (workspace creation, render queue
    // setup, MyGUI scene manager assignment). That is why nothing was rendered at all.
    NOWA::AppState::enter();
}

void ConfigurationState::start(const NOWA::SceneParameter& sceneParameter)
{
    // sceneManager and camera were already created by AppState::initializeModules(true, true)
    // and are handed back here via sceneParameter - do not create them again.
    this->sceneManager = sceneParameter.sceneManager;
    this->camera = sceneParameter.mainCamera;

    ProcessManager::getInstance()->attachProcess(ProcessPtr(new FaderProcess(FaderProcess::FadeOperation::FADE_IN, 3.5f)));

    // This is the ONLY render-thread hop needed here, called directly from the logic thread
    // (start() itself runs on the logic thread), so no nesting occurs.
    GraphicsModule::RenderCommand renderCommand = [this]()
    {
        this->setupWidgets();
        this->createScene();
    };
    NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ConfigurationState::start");

    this->createBackgroundMusic();
}

void ConfigurationState::exit(void)
{
    ProcessManager::getInstance()->attachProcess(ProcessPtr(new FaderProcess(FaderProcess::FadeOperation::FADE_OUT, 2.5f)));

    this->canUpdate = false;

    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ConfigurationState] Leaving...");

    GraphicsModule::RenderCommand renderCommand = [this]()
    {
        if (nullptr != MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::ImageBox>("ConfigurationStateBackground"))
        {
            MyGUI::Gui::getInstancePtr()->destroyWidget(MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::ImageBox>("ConfigurationStateBackground"));
        }
        if (nullptr != this->rootWindow)
        {
            MyGUI::Gui::getInstancePtr()->destroyWidget(this->rootWindow);
            this->rootWindow = nullptr;
        }
    };
    NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ConfigurationState::exit");

    this->keyConfigTextboxes.clear();
    this->oldKeyValue.clear();
    this->keyTextboxActive.clear();
    this->buttonConfigTextboxes.clear();
    this->oldButtonValue.clear();
    this->buttonTextboxActive.clear();
    padNav.reset();

    OgreALModule::getInstance()->deleteSound(this->sceneManager, this->soundMusic);

    this->menuMusic = nullptr;
    this->soundMusic = nullptr;

    NOWA::AppState::exit();
}

void ConfigurationState::createBackgroundMusic(void)
{
    OgreALModule::getInstance()->init(this->sceneManager);
    // Use scenemanager from menu state in which the music has been created and setContinue(true) was set to play to manipulate that music with volume here
    AppState* menuState = AppStateManager::getSingletonPtr()->findByName("MenuState");
    Ogre::SceneManager* menuSceneManager = menuState->getSceneManager();
    // this->menuMusic = OgreALModule::getInstance()->getSound(menuSceneManager, "MainGameObject_Menu - Mossgate Sanctuary.ogg");
    this->menuMusic = OgreALModule::getInstance()->getSound(menuSceneManager, "MainGameObject_Menu - The Great Paper Airplane.ogg");
    this->soundMusic = OgreALModule::getInstance()->createSound(this->sceneManager, "Click", "Click.wav");
}

void ConfigurationState::createScene(void)
{
}

// =============================================================================
// Widget setup
// =============================================================================

void ConfigurationState::setupWidgets(void)
{
    Core::getSingletonPtr()->setSceneManagerForMyGuiPlatform(this->sceneManager);

    MyGUI::Gui::getInstancePtr()
        ->createWidget<MyGUI::ImageBox>("RotatingSkin", MyGUI::IntCoord(0, 0, Core::getSingletonPtr()->getOgreRenderWindow()->getWidth(), Core::getSingletonPtr()->getOgreRenderWindow()->getHeight()), MyGUI::Align::Default, "Overlapped",
        "ConfigurationStateBackground")
        ->setImageTexture("BackgroundShadeBlue.png");

    // The layer MUST receive mouse picking (e.g. 'Overlapped'). Layers reserved for
    // passive overlays never route mouse focus to their widgets, so nothing would
    // respond — not even MyGUI's own native widget behaviour (dropdown, checkbox).
    this->rootWindow = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::Window>("WoodWindow", 0.15f, 0.10f, 0.70f, 0.80f, MyGUI::Align::Left | MyGUI::Align::Top, "Overlapped", "ConfigurationStateWindow");
    this->rootWindow->setCaption("Configuration");
    this->rootWindow->setMovable(true);

    this->createTabButtons();
    this->createGraphicsTab();
    this->createSoundTab();
    this->createControlsTab();

    // ── Restart hint + bottom buttons, shared by all tabs ──────────────────
    this->restartRequiredLabel = this->rootWindow->createWidgetReal<MyGUI::EditBox>("TextBox", 0.04f, 0.83f, 0.90f, 0.05f, MyGUI::Align::Left | MyGUI::Align::Top);
    this->restartRequiredLabel->setCaption("Restart required for anti-aliasing");
    this->restartRequiredLabel->setTextColour(MyGUI::Colour::Red);
    this->restartRequiredLabel->setEditReadOnly(true);
    this->restartRequiredLabel->setEditStatic(true);
    this->restartRequiredLabel->setVisible(false);

    this->applyButton = this->rootWindow->createWidgetReal<MyGUI::Button>("WoodButton", 0.04f, 0.90f, 0.26f, 0.07f, MyGUI::Align::Left | MyGUI::Align::Bottom, "configApplyButton");
    this->applyButton->setFontHeight(20);
    MyGUIUtilities::getInstance()->setFontSize(this->applyButton->castType<MyGUI::Button>(false), 20);
    this->applyButton->setCaption("Apply");
    this->applyButton->setTextColour(MyGUI::Colour(0.85f, 0.85f, 0.85f, 1.0f));
    this->applyButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ConfigurationState::buttonHit);

    this->okButton = this->rootWindow->createWidgetReal<MyGUI::Button>("WoodButton", 0.36f, 0.90f, 0.26f, 0.07f, MyGUI::Align::Left | MyGUI::Align::Bottom, "configOkButton");
    MyGUIUtilities::getInstance()->setFontSize(this->okButton->castType<MyGUI::Button>(false), 20);
    this->okButton->setCaptionWithReplacing("Ok");
    this->okButton->setTextColour(MyGUI::Colour(0.85f, 0.85f, 0.85f, 1.0f));
    this->okButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ConfigurationState::buttonHit);

    this->cancelButton = this->rootWindow->createWidgetReal<MyGUI::Button>("WoodButton", 0.68f, 0.90f, 0.26f, 0.07f, MyGUI::Align::Left | MyGUI::Align::Bottom, "configCancelButton");
    MyGUIUtilities::getInstance()->setFontSize(this->cancelButton->castType<MyGUI::Button>(false), 20);
    this->cancelButton->setCaption("Cancel");
    this->cancelButton->setTextColour(MyGUI::Colour(0.85f, 0.85f, 0.85f, 1.0f));
    this->cancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ConfigurationState::buttonHit);

    this->populateGraphicsOptions();
    this->populateSoundOptions();
    this->populateControlsOptions();

    this->showTab(0);
}

void ConfigurationState::createTabButtons(void)
{
    const Ogre::Real tabWidth = 0.30f;
    const Ogre::Real tabHeight = 0.06f;

    this->tabGraphicsButton = this->rootWindow->createWidgetReal<MyGUI::Button>("WoodButton", 0.02f, 0.06f, tabWidth, tabHeight, MyGUI::Align::Left | MyGUI::Align::Top, "tabGraphicsButton");
    this->tabGraphicsButton->setCaption("Graphics");
    this->tabGraphicsButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ConfigurationState::notifyTabButtonClick);

    this->tabSoundButton = this->rootWindow->createWidgetReal<MyGUI::Button>("WoodButton", 0.34f, 0.06f, tabWidth, tabHeight, MyGUI::Align::Left | MyGUI::Align::Top, "tabSoundButton");
    this->tabSoundButton->setCaption("Sound");
    this->tabSoundButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ConfigurationState::notifyTabButtonClick);

    this->tabControlsButton = this->rootWindow->createWidgetReal<MyGUI::Button>("WoodButton", 0.66f, 0.06f, tabWidth, tabHeight, MyGUI::Align::Left | MyGUI::Align::Top, "tabControlsButton");
    this->tabControlsButton->setCaption("Controls");
    this->tabControlsButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ConfigurationState::notifyTabButtonClick);
}

void ConfigurationState::notifyTabButtonClick(MyGUI::Widget* sender)
{
    if (sender == this->tabGraphicsButton)
    {
        this->showTab(0);
    }
    else if (sender == this->tabSoundButton)
    {
        this->showTab(1);
    }
    else if (sender == this->tabControlsButton)
    {
        this->showTab(2);
    }
}

void ConfigurationState::showTab(unsigned short tabIndex)
{
    this->currentTabIndex = tabIndex;

    this->graphicsPanel->setVisible(0 == tabIndex);
    this->soundPanel->setVisible(1 == tabIndex);
    this->controlsPanel->setVisible(2 == tabIndex);

    this->tabGraphicsButton->setStateSelected(0 == tabIndex);
    this->tabSoundButton->setStateSelected(1 == tabIndex);
    this->tabControlsButton->setStateSelected(2 == tabIndex);
}

MyGUI::EditBox* ConfigurationState::createLabel(MyGUI::Widget* parent, const Ogre::String& caption, Ogre::Real posY, Ogre::Real posX, Ogre::Real width)
{
    // Height matches the controls in the same row (0.07f), so that "Left VCenter" really
    // centers the text against its combo box / check box instead of sitting above it.
    MyGUI::EditBox* label = parent->createWidgetReal<MyGUI::EditBox>("TextBox", posX, posY, width, 0.07f, MyGUI::Align::Left | MyGUI::Align::Top);
    label->setCaption(caption);
    label->setEditReadOnly(true);
    label->setEditStatic(true);
    label->setTextAlign(MyGUI::Align::Left | MyGUI::Align::VCenter);

    // The default TextBox skin draws almost black text, which is unreadable on the dark
    // wood panel. Force a light parchment tone plus a dark shadow for contrast.
    label->setTextColour(MyGUI::Colour(0.93f, 0.88f, 0.76f));
    label->setTextShadow(true);
    label->setTextShadowColour(MyGUI::Colour(0.05f, 0.03f, 0.01f));

    return label;
}

// =============================================================================
// Graphics tab
// =============================================================================

void ConfigurationState::createGraphicsTab(void)
{
    this->graphicsPanel = this->rootWindow->createWidgetReal<MyGUI::Widget>("WoodPanel", 0.02f, 0.14f, 0.96f, 0.66f, MyGUI::Align::Left | MyGUI::Align::Top);

    const Ogre::Real rowHeight = 0.10f;
    const Ogre::Real controlX = 0.50f;
    const Ogre::Real controlWidth = 0.46f;
    const Ogre::Real controlHeight = 0.07f;
    Ogre::Real posY = 0.09f;

    this->createLabel(this->graphicsPanel, "Resolution:", posY);
    this->resolutionCombo = this->graphicsPanel->createWidgetReal<MyGUI::ComboBox>("ComboBox", controlX, posY, controlWidth, controlHeight, MyGUI::Align::Left | MyGUI::Align::Top);
    this->resolutionCombo->setComboModeDrop(true);
    this->resolutionCombo->setEditReadOnly(true);
    this->resolutionCombo->eventComboAccept += MyGUI::newDelegate(this, &ConfigurationState::notifyGraphicsComboAccept);
    posY += rowHeight;

    this->createLabel(this->graphicsPanel, "Fullscreen:", posY);
    this->fullscreenCheck = this->graphicsPanel->createWidgetReal<MyGUI::Button>("CheckBox", controlX, posY, controlWidth, controlHeight, MyGUI::Align::Left | MyGUI::Align::Top);
    // MyGUI::Button does not toggle setStateCheck on click by itself — must be done manually
    this->fullscreenCheck->eventMouseButtonClick += MyGUI::newDelegate(this, &ConfigurationState::notifyCheckBoxClick);
    posY += rowHeight;

    this->createLabel(this->graphicsPanel, "VSync:", posY);
    this->vsyncCheck = this->graphicsPanel->createWidgetReal<MyGUI::Button>("CheckBox", controlX, posY, controlWidth, controlHeight, MyGUI::Align::Left | MyGUI::Align::Top);
    this->vsyncCheck->eventMouseButtonClick += MyGUI::newDelegate(this, &ConfigurationState::notifyCheckBoxClick);
    posY += rowHeight;

    this->createLabel(this->graphicsPanel, "VSync interval:", posY);
    this->vsyncIntervalCombo = this->graphicsPanel->createWidgetReal<MyGUI::ComboBox>("ComboBox", controlX, posY, controlWidth, controlHeight, MyGUI::Align::Left | MyGUI::Align::Top);
    this->vsyncIntervalCombo->setComboModeDrop(true);
    this->vsyncIntervalCombo->setEditReadOnly(true);
    this->vsyncIntervalCombo->addItem("1");
    this->vsyncIntervalCombo->addItem("2");
    this->vsyncIntervalCombo->addItem("3");
    this->vsyncIntervalCombo->eventComboAccept += MyGUI::newDelegate(this, &ConfigurationState::notifyGraphicsComboAccept);
    posY += rowHeight;

    this->createLabel(this->graphicsPanel, "Anti-aliasing:", posY);
    this->fsaaCombo = this->graphicsPanel->createWidgetReal<MyGUI::ComboBox>("ComboBox", controlX, posY, controlWidth, controlHeight, MyGUI::Align::Left | MyGUI::Align::Top);
    this->fsaaCombo->setComboModeDrop(true);
    this->fsaaCombo->setEditReadOnly(true);
    this->fsaaCombo->eventComboAccept += MyGUI::newDelegate(this, &ConfigurationState::notifyGraphicsComboAccept);
    posY += rowHeight;

    this->createLabel(this->graphicsPanel, "Shadow quality:", posY);
    this->shadowQualityCombo = this->graphicsPanel->createWidgetReal<MyGUI::ComboBox>("ComboBox", controlX, posY, controlWidth, controlHeight, MyGUI::Align::Left | MyGUI::Align::Top);
    this->shadowQualityCombo->setComboModeDrop(true);
    this->shadowQualityCombo->setEditReadOnly(true);
    // Index 0 maps to -1 (scene default), index 1..4 map to shadow filter 0..3.
    this->shadowQualityCombo->addItem("Scene default");
    this->shadowQualityCombo->addItem("Low");
    this->shadowQualityCombo->addItem("Medium");
    this->shadowQualityCombo->addItem("High");
    this->shadowQualityCombo->addItem("Ultra");
    this->shadowQualityCombo->eventComboAccept += MyGUI::newDelegate(this, &ConfigurationState::notifyGraphicsComboAccept);
    posY += rowHeight;
}

void ConfigurationState::populateGraphicsOptions(void)
{
    Core* core = Core::getSingletonPtr();

    std::pair<unsigned int, unsigned int> resolution = core->getCurrentVideoModeResolution();
    this->initialWidth = resolution.first;
    this->initialHeight = resolution.second;
    this->initialFullscreen = core->getIsFullscreen();
    this->initialVSync = core->getIsVSync();
    this->initialVSyncInterval = 1;
    this->initialFsaa = core->getCurrentFSAA();
    this->initialShadowQuality = core->getShadowQuality();
    this->graphicsRestartRequired = false;

    std::vector<Ogre::String> videoModes = core->getAvailableVideoModes();
    std::vector<Ogre::String> fsaaModes = core->getAvailableFSAAModes();
    Ogre::String currentVideoMode = core->getCurrentVideoMode();

    this->resolutionCombo->removeAllItems();
    size_t selectedResolutionIndex = 0;
    for (size_t i = 0; i < videoModes.size(); i++)
    {
        this->resolutionCombo->addItem(videoModes[i]);
        if (videoModes[i] == currentVideoMode)
        {
            selectedResolutionIndex = i;
        }
    }
    if (this->resolutionCombo->getItemCount() > 0)
    {
        this->resolutionCombo->setIndexSelected(selectedResolutionIndex);
    }

    this->fullscreenCheck->setStateCheck(this->initialFullscreen);
    this->vsyncCheck->setStateCheck(this->initialVSync);
    this->vsyncIntervalCombo->setIndexSelected(0); // "1"

    this->fsaaCombo->removeAllItems();
    size_t selectedFsaaIndex = 0;
    for (size_t i = 0; i < fsaaModes.size(); i++)
    {
        this->fsaaCombo->addItem(fsaaModes[i]);
        if (fsaaModes[i] == this->initialFsaa)
        {
            selectedFsaaIndex = i;
        }
    }
    if (this->fsaaCombo->getItemCount() > 0)
    {
        this->fsaaCombo->setIndexSelected(selectedFsaaIndex);
    }

    // Index 0 is the scene default (-1), index 1..4 map to 0..3
    this->shadowQualityCombo->setIndexSelected(static_cast<size_t>(this->initialShadowQuality + 1));

    this->restartRequiredLabel->setVisible(false);
}

bool ConfigurationState::parseResolution(const Ogre::String& videoMode, unsigned int& outWidth, unsigned int& outHeight) const
{
    // Format is: "1920 x 1080 @ 32-bit colour"
    Ogre::String::size_type separatorPosition = videoMode.find('x');
    if (Ogre::String::npos == separatorPosition)
    {
        return false;
    }

    outWidth = static_cast<unsigned int>(Ogre::StringConverter::parseInt(videoMode.substr(0, separatorPosition)));
    outHeight = static_cast<unsigned int>(Ogre::StringConverter::parseInt(videoMode.substr(separatorPosition + 1)));

    if (0 == outWidth || 0 == outHeight)
    {
        return false;
    }

    return true;
}

void ConfigurationState::notifyGraphicsComboAccept(MyGUI::ComboBox* sender, size_t index)
{
    if (sender == this->fsaaCombo)
    {
        Ogre::String selectedFsaa = this->fsaaCombo->getItemNameAt(this->fsaaCombo->getIndexSelected());
        if (selectedFsaa != this->initialFsaa)
        {
            this->graphicsRestartRequired = true;
            this->restartRequiredLabel->setVisible(true);
        }
        else
        {
            this->graphicsRestartRequired = false;
            this->restartRequiredLabel->setVisible(false);
        }
    }
}

void ConfigurationState::notifyCheckBoxClick(MyGUI::Widget* sender)
{
    // MyGUI::Button (used as CheckBox skin) does not invert its own check state on click,
    // that must be done explicitly, same as in PropertiesPanelComponent::buttonHit.
    MyGUI::Button* button = sender->castType<MyGUI::Button>(false);
    if (nullptr != button)
    {
        button->setStateCheck(!button->getStateCheck());
    }
}

void ConfigurationState::readGraphicsSettingsFromWidgets(void)
{
    // Only reads into local state via applyGraphicsSettings — kept here for symmetry
    // with GraphicsConfigurationComponent's approach; values are read directly there.
}

void ConfigurationState::applyGraphicsSettings(void)
{
    Core* core = Core::getSingletonPtr();

    unsigned int width = this->initialWidth;
    unsigned int height = this->initialHeight;
    if (MyGUI::ITEM_NONE != this->resolutionCombo->getIndexSelected())
    {
        Ogre::String selectedVideoMode = this->resolutionCombo->getItemNameAt(this->resolutionCombo->getIndexSelected());
        this->parseResolution(selectedVideoMode, width, height);
    }

    bool fullscreen = this->fullscreenCheck->getStateCheck();
    bool vsync = this->vsyncCheck->getStateCheck();
    unsigned int vsyncInterval = static_cast<unsigned int>(this->vsyncIntervalCombo->getIndexSelected()) + 1;
    Ogre::String fsaa = this->initialFsaa;
    if (MyGUI::ITEM_NONE != this->fsaaCombo->getIndexSelected())
    {
        fsaa = this->fsaaCombo->getItemNameAt(this->fsaaCombo->getIndexSelected());
    }
    short shadowQuality = this->initialShadowQuality;
    if (MyGUI::ITEM_NONE != this->shadowQualityCombo->getIndexSelected())
    {
        shadowQuality = static_cast<short>(this->shadowQualityCombo->getIndexSelected()) - 1;
    }

    std::pair<unsigned int, unsigned int> currentResolution = core->getCurrentVideoModeResolution();
    if (currentResolution.first != width || currentResolution.second != height)
    {
        core->setVideoMode(width, height);
    }

    if (core->getIsFullscreen() != fullscreen)
    {
        core->setFullscreen(fullscreen, 0);
    }

    core->setVSync(vsync, vsyncInterval);

    if (core->getCurrentFSAA() != fsaa)
    {
        core->setFSAA(fsaa);
    }

    if (core->getShadowQuality() != shadowQuality)
    {
        core->setShadowQuality(shadowQuality);
    }

    core->saveGraphicsConfig();

    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ConfigurationState] Applied graphics settings.");
}

// =============================================================================
// Sound tab
// =============================================================================

void ConfigurationState::createSoundTab(void)
{
    this->soundPanel = this->rootWindow->createWidgetReal<MyGUI::Widget>("WoodPanel", 0.02f, 0.14f, 0.96f, 0.66f, MyGUI::Align::Left | MyGUI::Align::Top);
    this->soundPanel->setVisible(false);

    const Ogre::Real controlX = 0.50f;
    const Ogre::Real controlWidth = 0.44f;
    const Ogre::Real controlHeight = 0.06f;
    Ogre::Real posY = 0.09f;

    this->musicLabel = this->createLabel(this->soundPanel, "Music volume:", posY);
    this->musicSlider = this->soundPanel->createWidgetReal<MyGUI::ScrollBar>("SliderHWood", controlX, posY, controlWidth, controlHeight, MyGUI::Align::Left | MyGUI::Align::Top);
    this->musicSlider->setScrollRange(101);
    this->musicSlider->setScrollPage(5);
    this->musicSlider->eventScrollChangePosition += MyGUI::newDelegate(this, &ConfigurationState::notifySoundSliderChangePosition);
    posY += 0.10f;

    this->soundLabel = this->createLabel(this->soundPanel, "Sound volume:", posY);
    this->soundSlider = this->soundPanel->createWidgetReal<MyGUI::ScrollBar>("SliderHWood", controlX, posY, controlWidth, controlHeight, MyGUI::Align::Left | MyGUI::Align::Top);
    this->soundSlider->setScrollRange(101);
    this->soundSlider->setScrollPage(5);
    this->soundSlider->eventScrollChangePosition += MyGUI::newDelegate(this, &ConfigurationState::notifySoundSliderChangePosition);
    this->soundSlider->eventMouseButtonReleased += MyGUI::newDelegate(this, &ConfigurationState::notifySliderMouseRelease);
}

void ConfigurationState::populateSoundOptions(void)
{
    this->musicSlider->setScrollPosition(static_cast<size_t>(Core::getSingletonPtr()->getOptionMusicVolume()));
    this->soundSlider->setScrollPosition(static_cast<size_t>(Core::getSingletonPtr()->getOptionSoundVolume()));

    OgreALModule::getInstance()->setupVolumes(static_cast<int>(this->soundSlider->getScrollPosition()), static_cast<int>(this->musicSlider->getScrollPosition()));

    this->musicLabel->setCaption("Music volume: (" + Ogre::StringConverter::toString(this->musicSlider->getScrollPosition()) + " %)");
    this->soundLabel->setCaption("Sound volume: (" + Ogre::StringConverter::toString(this->soundSlider->getScrollPosition()) + " %)");
}

void ConfigurationState::notifySoundSliderChangePosition(MyGUI::ScrollBar* sender, size_t position)
{
    if (sender == this->musicSlider)
    {
        if (nullptr != this->menuMusic)
        {
            this->menuMusic->setGain(this->musicSlider->getScrollPosition() / 100.0f);
            NOWA::OgreALModule::getInstance()->setMusicVolume(static_cast<int>(this->musicSlider->getScrollPosition()));
        }
        this->musicLabel->setCaption("Music volume: (" + Ogre::StringConverter::toString(this->musicSlider->getScrollPosition()) + " %)");
    }
    else if (sender == this->soundSlider)
    {
        this->soundMusic->setGain(this->soundSlider->getScrollPosition() / 100.0f);
        this->soundLabel->setCaption("Sound volume: (" + Ogre::StringConverter::toString(this->soundSlider->getScrollPosition()) + " %)");
        NOWA::OgreALModule::getInstance()->setSoundVolume(static_cast<int>(this->soundSlider->getScrollPosition()));
    }
}

void ConfigurationState::notifySliderMouseRelease(MyGUI::Widget* sender, int x, int y, MyGUI::MouseButton button)
{
    if (sender == this->soundSlider)
    {
        if (nullptr != this->soundMusic)
        {
            this->soundMusic->play();
        }
        this->soundLabel->setCaption("Sound volume: (" + Ogre::StringConverter::toString(this->soundSlider->getScrollPosition()) + " %)");
        NOWA::OgreALModule::getInstance()->setSoundVolume(static_cast<int>(this->soundSlider->getScrollPosition()));
    }
}

void ConfigurationState::applySoundSettings(void)
{
    Core::getSingletonPtr()->setOptionMusicVolume(static_cast<int>(this->musicSlider->getScrollPosition()));
    Core::getSingletonPtr()->setOptionSoundVolume(static_cast<int>(this->soundSlider->getScrollPosition()));

    OgreALModule::getInstance()->setupVolumes(static_cast<int>(this->soundSlider->getScrollPosition()), static_cast<int>(this->musicSlider->getScrollPosition()));

    Core::getSingletonPtr()->saveCustomConfiguration();

    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ConfigurationState] Applied sound settings.");
}

// =============================================================================
// Controls tab
// =============================================================================

void ConfigurationState::createControlsTab(void)
{
    this->controlsPanel = this->rootWindow->createWidgetReal<MyGUI::Widget>("WoodPanel", 0.02f, 0.14f, 0.96f, 0.66f, MyGUI::Align::Left | MyGUI::Align::Top);
    this->controlsPanel->setVisible(false);

    // Note: Previously the main keyboard module was asked, which is not a gamepad. Now: Is any gamepad connected?
    this->hasJoystick = InputDeviceCore::getSingletonPtr()->getJoyStickCount() > 0;

    const Ogre::Real keyEditX = 0.42f;
    const Ogre::Real keyEditWidth = 0.24f;
    const Ogre::Real buttonEditX = 0.70f;
    const Ogre::Real buttonEditWidth = 0.24f;

    // Row values are relative to the scroll view below, not to the panel
    const Ogre::Real firstRowY = 0.01f;
    const Ogre::Real rowHeight = 0.10f;
    // Same as the label height in createLabel (0.07f), so that label and textboxes line up
    const Ogre::Real controlHeight = 0.07f;

    this->keyConfigTextboxes.resize(ACTION_ROW_COUNT);
    this->oldKeyValue.resize(ACTION_ROW_COUNT);
    this->keyTextboxActive.resize(ACTION_ROW_COUNT, false);

    if (true == this->hasJoystick)
    {
        this->buttonConfigTextboxes.resize(ACTION_ROW_COUNT);
        this->oldButtonValue.resize(ACTION_ROW_COUNT);
        this->buttonTextboxActive.resize(ACTION_ROW_COUNT, false);

        this->createLabel(this->controlsPanel, "Keyboard", 0.0f, keyEditX, keyEditWidth);
        this->createLabel(this->controlsPanel, "Joystick", 0.0f, buttonEditX, buttonEditWidth);
    }

    // All actions do not fit into the panel, so the rows live in a scroll view below the column headers.
    // Attention: Assumes that a "ScrollView" skin exists (MyGUI core skin). Change the name, if your theme uses another one.
    MyGUI::ScrollView* scrollView = this->controlsPanel->createWidgetReal<MyGUI::ScrollView>("ScrollView", 0.0f, 0.09f, 1.0f, 0.91f, MyGUI::Align::Stretch);
    scrollView->setVisibleHScroll(false);
    scrollView->setVisibleVScroll(true);

    // Rows are placed with real coordinates (relative to the scroll view), the canvas just has to be as high as the last row ends
    const int viewHeight = scrollView->getHeight();
    int canvasHeight = static_cast<int>((firstRowY + rowHeight * static_cast<Ogre::Real>(ACTION_ROW_COUNT)) * static_cast<Ogre::Real>(viewHeight));
    if (canvasHeight < viewHeight)
    {
        canvasHeight = viewHeight;
    }
    scrollView->setCanvasSize(scrollView->getWidth(), canvasHeight);

    Ogre::Real posY = firstRowY;
    for (unsigned short i = 0; i < ACTION_ROW_COUNT; i++)
    {
        this->createLabel(scrollView, actionRows[i].label, posY);

        this->keyConfigTextboxes[i] = scrollView->createWidgetReal<MyGUI::EditBox>("EditBox", keyEditX, posY, keyEditWidth, controlHeight, MyGUI::Align::Left | MyGUI::Align::Top);
        this->keyConfigTextboxes[i]->setEditReadOnly(true);
        this->keyConfigTextboxes[i]->setNeedMouseFocus(true);
        this->keyConfigTextboxes[i]->eventMouseSetFocus += MyGUI::newDelegate(this, &ConfigurationState::notifyKeyEditFocus);

        if (true == this->hasJoystick)
        {
            this->buttonConfigTextboxes[i] = scrollView->createWidgetReal<MyGUI::EditBox>("EditBox", buttonEditX, posY, buttonEditWidth, controlHeight, MyGUI::Align::Left | MyGUI::Align::Top);
            this->buttonConfigTextboxes[i]->setEditReadOnly(true);
            this->buttonConfigTextboxes[i]->setNeedMouseFocus(true);
            this->buttonConfigTextboxes[i]->eventMouseSetFocus += MyGUI::newDelegate(this, &ConfigurationState::notifyButtonEditFocus);
        }

        posY += rowHeight;
    }
}

void ConfigurationState::populateControlsOptions(void)
{
    auto keyboardModule = InputDeviceCore::getSingletonPtr()->getMainKeyboardInputDeviceModule();

    for (unsigned short i = 0; i < ACTION_ROW_COUNT; i++)
    {
        auto keyCode = keyboardModule->getMappedKey(actionRows[i].action);
        Ogre::String strKeyCode = keyboardModule->getStringFromMappedKey(keyCode);
        this->oldKeyValue[i] = strKeyCode;
        this->keyConfigTextboxes[i]->setCaption(strKeyCode);
    }

    if (true == this->hasJoystick)
    {
        // The gamepad profile (shared by all gamepads) is stored in the main keyboard module, so it is also available and saved if no gamepad is connected.
        // Note: Previously getJoystickInputDeviceModule(0) was used, which searches for the module OCCUPIED by game object id 0 (not the first gamepad) and could deliver null.
        for (unsigned short i = 0; i < ACTION_ROW_COUNT; i++)
        {
            auto button = keyboardModule->getMappedButton(actionRows[i].action);
            Ogre::String strButton = keyboardModule->getStringFromMappedButton(button);
            this->oldButtonValue[i] = strButton;
            this->buttonConfigTextboxes[i]->setCaption(strButton);
        }
    }
}

void ConfigurationState::notifyKeyEditFocus(MyGUI::Widget* sender, MyGUI::Widget* old)
{
    // The gamepad navigation snaps the pointer onto the boxes. That hover must not arm the rebinding, otherwise the confirm
    // button press itself would be bound at once. The gamepad arms a box explicitly (see update).
    if (true == isHoverArmingSuppressed())
    {
        return;
    }

    for (unsigned short i = 0; i < this->keyConfigTextboxes.size(); i++)
    {
        this->keyConfigTextboxes[i]->setTextShadow(false);
        this->keyTextboxActive[i] = false;
        if (sender == this->keyConfigTextboxes[i])
        {
            this->keyTextboxActive[i] = true;
            this->keyConfigTextboxes[i]->setTextShadow(true);
        }
    }
}

void ConfigurationState::notifyButtonEditFocus(MyGUI::Widget* sender, MyGUI::Widget* old)
{
    // See notifyKeyEditFocus.
    if (true == isHoverArmingSuppressed())
    {
        return;
    }

    for (unsigned short i = 0; i < this->buttonConfigTextboxes.size(); i++)
    {
        this->buttonConfigTextboxes[i]->setTextShadow(false);
        this->buttonTextboxActive[i] = false;
        if (sender == this->buttonConfigTextboxes[i])
        {
            this->buttonTextboxActive[i] = true;
            this->buttonConfigTextboxes[i]->setTextShadow(true);
        }
    }
}

void ConfigurationState::applyControlsSettings(void)
{
    auto keyboardModule = InputDeviceCore::getSingletonPtr()->getMainKeyboardInputDeviceModule();

    for (unsigned short i = 0; i < ACTION_ROW_COUNT; i++)
    {
        OIS::KeyCode key = keyboardModule->getMappedKeyFromString(this->keyConfigTextboxes[i]->getCaption());
        // Never destroy a binding because a caption could not be parsed
        if (OIS::KC_UNASSIGNED != key)
        {
            keyboardModule->remapKey(actionRows[i].action, key);
        }
    }
    // The main keyboard module is the one the players use, other keyboards get the same mapping
    InputDeviceCore::getSingletonPtr()->applyKeyboardMappingToAllKeyboards();

    if (true == this->hasJoystick)
    {
        for (unsigned short i = 0; i < ACTION_ROW_COUNT; i++)
        {
            const Ogre::String caption = this->buttonConfigTextboxes[i]->getCaption();
            auto button = keyboardModule->getMappedButtonFromString(caption);
            if (InputDeviceModule::BUTTON_NONE != button)
            {
                // Stores the button in the gamepad profile and applies it to all connected gamepads
                InputDeviceCore::getSingletonPtr()->remapGamepadButton(static_cast<unsigned short>(actionRows[i].action), static_cast<unsigned short>(button));
            }
            else if ("None" == caption && "None" != this->oldButtonValue[i])
            {
                // The user took this button over for another action, so this action really becomes unbound.
                // Only done for such an explicit change: a caption which merely could not be parsed must never destroy a binding.
                InputDeviceCore::getSingletonPtr()->remapGamepadButton(static_cast<unsigned short>(actionRows[i].action), static_cast<unsigned short>(InputDeviceModule::BUTTON_NONE));
            }
        }
    }

    Core::getSingletonPtr()->saveCustomConfiguration();

    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ConfigurationState] Applied controls settings.");
}

// =============================================================================
// Bottom buttons
// =============================================================================

void ConfigurationState::buttonHit(MyGUI::Widget* sender)
{
    if ("configApplyButton" == sender->getName())
    {
        this->applyGraphicsSettings();
        this->applySoundSettings();
        this->applyControlsSettings();
        AppStateManager::getSingletonPtr()->reloadCurrentState();
    }
    if ("configOkButton" == sender->getName())
    {
        this->applyGraphicsSettings();
        this->applySoundSettings();
        this->applyControlsSettings();

        AppStateManager::getSingletonPtr()->reloadCurrentStateThenChangeAppState(this->findByName("MenuState"));
    }
    else if ("configCancelButton" == sender->getName())
    {
        // Revert widgets to the values captured when the menu was opened
        this->populateGraphicsOptions();
        this->populateSoundOptions();
        this->populateControlsOptions();

        this->changeAppState(this->findByName("MenuState"));
    }
}

void ConfigurationState::notifyMessageBoxEnd(MyGUI::Message* sender, MyGUI::MessageBoxStyle result)
{
    if (result == MyGUI::MessageBoxStyle::Yes)
    {
        this->shutdown();
    }
}

// =============================================================================
// AppState overrides
// =============================================================================

void ConfigurationState::update(Ogre::Real dt)
{
    if (this->bQuit)
    {
        this->shutdown();
    }

    // ── Gamepad navigation ────────────────────────────────────────────────────
    if (false == this->hasJoystick || nullptr == this->rootWindow)
    {
        return;
    }

    int directionX = 0;
    int directionY = 0;
    const bool directionHeld = readPadDirection(directionX, directionY);

    const bool confirmIsDown = readPadConfirm();
    // Only the flank counts, otherwise a held button would click every frame.
    const bool confirmPressed = (true == confirmIsDown && false == padNav.confirmWasDown);
    padNav.confirmWasDown = confirmIsDown;

    // A rebinding box waits for the next button: every button belongs to the binding now, so nothing is navigated.
    bool anyArmed = false;
    for (size_t i = 0; i < this->keyTextboxActive.size() && false == anyArmed; i++)
    {
        anyArmed = this->keyTextboxActive[i];
    }
    for (size_t i = 0; i < this->buttonTextboxActive.size() && false == anyArmed; i++)
    {
        anyArmed = this->buttonTextboxActive[i];
    }

    if (true == anyArmed)
    {
        padNav.navigationTimer = 0.0f;

        // A gamepad has no Escape, so a box armed by the gamepad disarms itself after a while. One armed by the mouse hover stays as before.
        if (true == padNav.armedByPad)
        {
            padNav.armedTimer -= dt;
            if (padNav.armedTimer <= 0.0f)
            {
                padNav.armedByPad = false;

                GraphicsModule::RenderCommand renderCommand = [this]()
                {
                    for (size_t i = 0; i < this->keyConfigTextboxes.size(); i++)
                    {
                        this->keyTextboxActive[i] = false;
                        this->keyConfigTextboxes[i]->setTextShadow(false);
                    }
                    for (size_t i = 0; i < this->buttonConfigTextboxes.size(); i++)
                    {
                        this->buttonTextboxActive[i] = false;
                        this->buttonConfigTextboxes[i]->setTextShadow(false);
                    }
                };
                GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "ConfigurationState::padDisarm");
            }
        }
        return;
    }
    padNav.armedByPad = false;

    // The sound slider plays its test sound when the mouse button is released. For the gamepad that is: the direction is released.
    if (false == directionHeld && true == padNav.soundSliderChanged)
    {
        padNav.soundSliderChanged = false;

        GraphicsModule::RenderCommand renderCommand = [this]()
        {
            if (nullptr != this->soundSlider)
            {
                this->notifySliderMouseRelease(this->soundSlider, 0, 0, MyGUI::MouseButton::Left);
            }
        };
        GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "ConfigurationState::padSliderRelease");
    }

    bool doStep = false;
    if (true == directionHeld)
    {
        padNav.navigationTimer -= dt;
        if (padNav.navigationTimer <= 0.0f)
        {
            doStep = true;
        }
    }
    else
    {
        // Released: the next press steps immediately instead of waiting for the delay.
        padNav.navigationTimer = 0.0f;
    }

    if (false == doStep && false == confirmPressed)
    {
        return;
    }

    PadResult result;
    const bool engagedBefore = padNav.engaged;

    // Attention: the targets are collected anew for every step, on the render thread, so tab changes and scrolling need no refresh.
    GraphicsModule::RenderCommand renderCommand = [this, &result, directionX, directionY, doStep, confirmPressed, engagedBefore]()
    {
        std::vector<MyGUI::Widget*> targets;

        // A hidden widget (the panel of another tab) must not swallow the focus. getInheritedVisible also covers a hidden parent.
        auto addTarget = [&targets](MyGUI::Widget* widget)
        {
            if (nullptr != widget && true == widget->getInheritedVisible())
            {
                targets.push_back(widget);
            }
        };

        addTarget(this->tabGraphicsButton);
        addTarget(this->tabSoundButton);
        addTarget(this->tabControlsButton);
        addTarget(this->applyButton);
        addTarget(this->okButton);
        addTarget(this->cancelButton);

        addTarget(this->resolutionCombo);
        addTarget(this->fullscreenCheck);
        addTarget(this->vsyncCheck);
        addTarget(this->vsyncIntervalCombo);
        addTarget(this->fsaaCombo);
        addTarget(this->shadowQualityCombo);

        addTarget(this->musicSlider);
        addTarget(this->soundSlider);

        for (size_t i = 0; i < this->keyConfigTextboxes.size(); i++)
        {
            addTarget(this->keyConfigTextboxes[i]);
        }
        for (size_t i = 0; i < this->buttonConfigTextboxes.size(); i++)
        {
            addTarget(this->buttonConfigTextboxes[i]);
        }

        if (true == targets.empty())
        {
            return;
        }

        // The first input only shows where the focus is: on the button of the current tab.
        const bool focusIsValid = (nullptr != padNav.focused && targets.end() != std::find(targets.begin(), targets.end(), padNav.focused));
        if (false == engagedBefore || false == focusIsValid)
        {
            MyGUI::Widget* tabButton = this->tabGraphicsButton;
            if (1 == this->currentTabIndex)
            {
                tabButton = this->tabSoundButton;
            }
            else if (2 == this->currentTabIndex)
            {
                tabButton = this->tabControlsButton;
            }

            padNav.focused = tabButton;

            const MyGUI::IntPoint center = centerOf(padNav.focused);
            result.snap = true;
            result.x = center.left;
            result.y = center.top;
            return;
        }

        MyGUI::Widget* focused = padNav.focused;
        MyGUI::ComboBox* combo = focused->castType<MyGUI::ComboBox>(false);
        MyGUI::ScrollBar* slider = focused->castType<MyGUI::ScrollBar>(false);

        if (true == confirmPressed)
        {
            // A rebinding box: arm it, the next gamepad button (or keyboard key) is bound.
            bool isRebindingBox = false;
            for (size_t i = 0; i < this->keyConfigTextboxes.size(); i++)
            {
                if (focused == this->keyConfigTextboxes[i])
                {
                    isRebindingBox = true;
                }
            }
            for (size_t i = 0; i < this->buttonConfigTextboxes.size(); i++)
            {
                if (focused == this->buttonConfigTextboxes[i])
                {
                    isRebindingBox = true;
                }
            }

            if (true == isRebindingBox)
            {
                for (size_t i = 0; i < this->keyConfigTextboxes.size(); i++)
                {
                    const bool isFocused = (focused == this->keyConfigTextboxes[i]);
                    this->keyTextboxActive[i] = isFocused;
                    this->keyConfigTextboxes[i]->setTextShadow(isFocused);
                }
                for (size_t i = 0; i < this->buttonConfigTextboxes.size(); i++)
                {
                    const bool isFocused = (focused == this->buttonConfigTextboxes[i]);
                    this->buttonTextboxActive[i] = isFocused;
                    this->buttonConfigTextboxes[i]->setTextShadow(isFocused);
                }
                result.armed = true;
            }
            else if (nullptr != combo)
            {
                // Confirm steps to the next entry (wraps around), LEFT / RIGHT steps both ways.
                const size_t count = combo->getItemCount();
                if (count > 0)
                {
                    const size_t selected = combo->getIndexSelected();
                    const size_t next = (MyGUI::ITEM_NONE == selected || selected + 1 >= count) ? 0 : selected + 1;
                    combo->setIndexSelected(next);
                    this->notifyGraphicsComboAccept(combo, next);
                }
            }
            else if (nullptr == slider)
            {
                // A button or a check box: a real click, so the handlers and the pressed look behave exactly as with the mouse.
                const MyGUI::IntPoint center = centerOf(focused);
                result.click = true;
                result.x = center.left;
                result.y = center.top;
            }
        }
        else if (true == doStep)
        {
            if (0 != directionX && nullptr != combo)
            {
                const size_t count = combo->getItemCount();
                if (count > 0)
                {
                    const size_t selected = combo->getIndexSelected();
                    int index = (MyGUI::ITEM_NONE == selected) ? 0 : static_cast<int>(selected) + directionX;
                    index = std::max(0, std::min(index, static_cast<int>(count) - 1));

                    if (MyGUI::ITEM_NONE == selected || static_cast<size_t>(index) != selected)
                    {
                        combo->setIndexSelected(static_cast<size_t>(index));
                        this->notifyGraphicsComboAccept(combo, static_cast<size_t>(index));
                    }
                }
            }
            else if (0 != directionX && nullptr != slider)
            {
                const int maxPosition = static_cast<int>(slider->getScrollRange()) - 1;
                const int current = static_cast<int>(slider->getScrollPosition());
                const int position = std::max(0, std::min(current + directionX * 5, maxPosition));

                if (position != current)
                {
                    slider->setScrollPosition(static_cast<size_t>(position));
                    this->notifySoundSliderChangePosition(slider, static_cast<size_t>(position));

                    if (slider == this->soundSlider)
                    {
                        result.soundSliderAdjusted = true;
                    }
                }
                result.repeatDelay = PAD_SLIDER_DELAY;
            }
            else
            {
                MyGUI::Widget* neighbour = findNeighbour(targets, focused, directionX, directionY);
                if (nullptr != neighbour)
                {
                    padNav.focused = neighbour;
                    scrollIntoView(neighbour);

                    const MyGUI::IntPoint center = centerOf(neighbour);
                    result.snap = true;
                    result.x = center.left;
                    result.y = center.top;
                }
            }
        }
    };
    GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ConfigurationState::padNavigation");

    padNav.engaged = true;
    padNav.navigationTimer = result.repeatDelay;

    if (true == result.soundSliderAdjusted)
    {
        padNav.soundSliderChanged = true;
    }

    if (true == result.armed)
    {
        padNav.armedByPad = true;
        padNav.armedTimer = PAD_ARMED_TIMEOUT;
    }

    if (true == result.snap || true == result.click)
    {
        // Before the pointer is moved, because the hover event arrives right after.
        padNav.suppressHoverUntilMs.store(nowMilliseconds() + PAD_HOVER_SUPPRESS_MS);
    }

    if (true == result.snap)
    {
        InputDeviceCore::getSingletonPtr()->setMousePosition(result.x, result.y);
    }

    if (true == result.click)
    {
        const int positionX = result.x;
        const int positionY = result.y;

        // Fire and forget, only plain values are captured. The move is repeated, so MyGUI surely has its mouse focus on the widget.
        GraphicsModule::RenderCommand clickCommand = [positionX, positionY]()
        {
            if (nullptr == MyGUI::InputManager::getInstancePtr())
            {
                return;
            }

            MyGUI::InputManager::getInstancePtr()->injectMouseMove(positionX, positionY, 0);
            MyGUI::InputManager::getInstancePtr()->injectMousePress(positionX, positionY, MyGUI::MouseButton::Left);
            MyGUI::InputManager::getInstancePtr()->injectMouseRelease(positionX, positionY, MyGUI::MouseButton::Left);
        };
        GraphicsModule::getInstance()->enqueue(std::move(clickCommand), "ConfigurationState::padClick");
    }
}

bool ConfigurationState::keyPressed(const OIS::KeyEvent& keyEventRef)
{
    NOWA::Core::getSingletonPtr()->keyPressed(keyEventRef);

    if (OIS::KC_ESCAPE == keyEventRef.key)
    {
        // Escape while waiting for a key/button just cancels the rebinding, instead of quitting
        bool wasRebinding = false;
        for (unsigned short i = 0; i < this->keyConfigTextboxes.size(); i++)
        {
            if (true == this->keyTextboxActive[i])
            {
                this->keyTextboxActive[i] = false;
                this->keyConfigTextboxes[i]->setTextShadow(false);
                wasRebinding = true;
            }
        }
        for (unsigned short i = 0; i < this->buttonConfigTextboxes.size(); i++)
        {
            if (true == this->buttonTextboxActive[i])
            {
                this->buttonTextboxActive[i] = false;
                this->buttonConfigTextboxes[i]->setTextShadow(false);
                wasRebinding = true;
            }
        }

        if (false == wasRebinding)
        {
            this->bQuit = true;
        }
        return true;
    }

#if defined(_WIN32)
    if (keyEventRef.key == OIS::KC_TAB)
    {
        if (GetAsyncKeyState(KF_ALTDOWN))
        {
            NOWA::Core::getSingletonPtr()->moveWindowToTaskbar();
            return true;
        }
    }
#endif

    auto keyboardModule = InputDeviceCore::getSingletonPtr()->getMainKeyboardInputDeviceModule();

    for (unsigned short i = 0; i < this->keyConfigTextboxes.size(); i++)
    {
        if (true == this->keyTextboxActive[i])
        {
            Ogre::String strKeyCode = keyboardModule->getStringFromMappedKey(keyEventRef.key);

            // Refuse a key already used by another action
            bool alreadyExisting = false;
            for (unsigned short j = 0; j < this->keyConfigTextboxes.size(); j++)
            {
                if (j != i && this->keyConfigTextboxes[j]->getCaption() == strKeyCode)
                {
                    alreadyExisting = true;
                    break;
                }
            }

            if (false == alreadyExisting && false == strKeyCode.empty())
            {
                this->keyConfigTextboxes[i]->setCaption(strKeyCode);
            }

            this->keyTextboxActive[i] = false;
            this->keyConfigTextboxes[i]->setTextShadow(false);
            break;
        }
    }

    return true;
}

bool ConfigurationState::keyReleased(const OIS::KeyEvent& keyEventRef)
{
    NOWA::Core::getSingletonPtr()->keyReleased(keyEventRef);
    return true;
}

bool ConfigurationState::mouseMoved(const OIS::MouseEvent& evt)
{
    NOWA::Core::getSingletonPtr()->mouseMoved(evt);
    return true;
}

bool ConfigurationState::mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
{
    NOWA::Core::getSingletonPtr()->mousePressed(evt, id);
    return true;
}

bool ConfigurationState::mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
{
    NOWA::Core::getSingletonPtr()->mouseReleased(evt, id);
    return true;
}

bool ConfigurationState::axisMoved(const OIS::JoyStickEvent& evt, int axis)
{
    if (false == this->hasJoystick || axis < 0 || axis >= static_cast<int>(evt.state.mAxes.size()))
    {
        return true;
    }

    // XInput / Linux deliver the triggers as axes, so LT/RT can only be bound here
    InputDeviceModule* joystickModule = InputDeviceCore::getSingletonPtr()->getInputDeviceModuleFromDeviceObject(evt.device);
    if (nullptr == joystickModule)
    {
        return true;
    }

    InputDeviceModule::JoyStickButton button = joystickModule->translateRawAxis(axis, evt.state.mAxes[axis].abs);
    if (InputDeviceModule::BUTTON_LT == button || InputDeviceModule::BUTTON_RT == button)
    {
        bindCaptionToActiveTextbox(this->buttonConfigTextboxes, this->buttonTextboxActive, joystickModule->getStringFromMappedButton(button));
    }
    return true;
}

bool ConfigurationState::buttonPressed(const OIS::JoyStickEvent& evt, int button)
{
    if (false == this->hasJoystick)
    {
        return true;
    }

    // The event carries the device, so the raw button index can be translated with the layout of THAT gamepad
    // (previously the raw index was casted directly, which is wrong e.g. for Xbox pads / Steam Deck: raw 0 is Start there, not X).
    InputDeviceModule* joystickModule = InputDeviceCore::getSingletonPtr()->getInputDeviceModuleFromDeviceObject(evt.device);
    if (nullptr == joystickModule)
    {
        return true;
    }

    InputDeviceModule::JoyStickButton logicalButton = joystickModule->translateRawButton(button);
    if (InputDeviceModule::BUTTON_NONE == logicalButton)
    {
        return true;
    }

    bindCaptionToActiveTextbox(this->buttonConfigTextboxes, this->buttonTextboxActive, joystickModule->getStringFromMappedButton(logicalButton));

    return true;
}

bool ConfigurationState::buttonReleased(const OIS::JoyStickEvent& evt, int button)
{
    return true;
}
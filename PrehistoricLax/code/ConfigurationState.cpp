#include "NOWAPrecompiled.h"
#include "ConfigurationState.h"

using namespace NOWA;

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
    this->menuMusic = OgreALModule::getInstance()->getSound(menuSceneManager, "MainGameObject_Menu - Mossgate Sanctuary.ogg");
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
    this->applyButton->setCaption("Apply");
    this->applyButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ConfigurationState::buttonHit);

    this->cancelButton = this->rootWindow->createWidgetReal<MyGUI::Button>("WoodButton", 0.68f, 0.90f, 0.26f, 0.07f, MyGUI::Align::Left | MyGUI::Align::Bottom, "configCancelButton");
    this->cancelButton->setCaption("Cancel");
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

    this->hasJoystick = InputDeviceCore::getSingletonPtr()->getMainKeyboardInputDeviceModule()->hasActiveJoyStick();

    static const char* actionLabels[ACTION_COUNT] = { "Move up:", "Move down:", "Move left:", "Move right:", "Jump:", "Action 1:", "Action 2:" };

    const Ogre::Real rowHeight = 0.09f;
    const Ogre::Real keyEditX = 0.42f;
    const Ogre::Real keyEditWidth = 0.24f;
    const Ogre::Real buttonEditX = 0.70f;
    const Ogre::Real buttonEditWidth = 0.24f;
    const Ogre::Real controlHeight = 0.06f;

    this->keyConfigTextboxes.resize(ACTION_COUNT);
    this->oldKeyValue.resize(ACTION_COUNT);
    this->keyTextboxActive.resize(ACTION_COUNT, false);

    if (true == this->hasJoystick)
    {
        this->buttonConfigTextboxes.resize(ACTION_COUNT);
        this->oldButtonValue.resize(ACTION_COUNT);
        this->buttonTextboxActive.resize(ACTION_COUNT, false);

        this->createLabel(this->controlsPanel, "Keyboard", 0.0f, keyEditX, keyEditWidth);
        this->createLabel(this->controlsPanel, "Joystick", 0.0f, buttonEditX, buttonEditWidth);
    }

    Ogre::Real posY = 0.13f;
    for (unsigned short i = 0; i < ACTION_COUNT; i++)
    {
        this->createLabel(this->controlsPanel, actionLabels[i], posY);

        this->keyConfigTextboxes[i] = this->controlsPanel->createWidgetReal<MyGUI::EditBox>("EditBox", keyEditX, posY, keyEditWidth, controlHeight, MyGUI::Align::Left | MyGUI::Align::Top);
        this->keyConfigTextboxes[i]->setEditReadOnly(true);
        this->keyConfigTextboxes[i]->setNeedMouseFocus(true);
        this->keyConfigTextboxes[i]->eventMouseSetFocus += MyGUI::newDelegate(this, &ConfigurationState::notifyKeyEditFocus);

        if (true == this->hasJoystick)
        {
            this->buttonConfigTextboxes[i] = this->controlsPanel->createWidgetReal<MyGUI::EditBox>("EditBox", buttonEditX, posY, buttonEditWidth, controlHeight, MyGUI::Align::Left | MyGUI::Align::Top);
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

    for (unsigned short i = 0; i < ACTION_COUNT; i++)
    {
        auto keyCode = keyboardModule->getMappedKey(static_cast<InputDeviceModule::Action>(i));
        Ogre::String strKeyCode = keyboardModule->getStringFromMappedKey(keyCode);
        this->oldKeyValue[i] = strKeyCode;
        this->keyConfigTextboxes[i]->setCaption(strKeyCode);
    }

    if (true == this->hasJoystick)
    {
        // NOTE: assumes InputDeviceCore exposes a joystick counterpart to
        // getMainKeyboardInputDeviceModule() not tied to a game object id.
        // If your InputDeviceCore names it differently, adjust this one call.
        auto joystickModule = InputDeviceCore::getSingletonPtr()->getJoystickInputDeviceModule(0);

        for (unsigned short i = 0; i < ACTION_COUNT; i++)
        {
            auto button = joystickModule->getMappedButton(static_cast<InputDeviceModule::Action>(i));
            Ogre::String strButton = joystickModule->getStringFromMappedButton(button);
            this->oldButtonValue[i] = strButton;
            this->buttonConfigTextboxes[i]->setCaption(strButton);
        }
    }
}

void ConfigurationState::notifyKeyEditFocus(MyGUI::Widget* sender, MyGUI::Widget* old)
{
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

    for (unsigned short i = 0; i < ACTION_COUNT; i++)
    {
        OIS::KeyCode key = keyboardModule->getMappedKeyFromString(this->keyConfigTextboxes[i]->getCaption());
        keyboardModule->remapKey(static_cast<InputDeviceModule::Action>(i), key);
    }

    if (true == this->hasJoystick)
    {
        auto joystickModule = InputDeviceCore::getSingletonPtr()->getJoystickInputDeviceModule(0);

        for (unsigned short i = 0; i < ACTION_COUNT; i++)
        {
            auto button = joystickModule->getMappedButtonFromString(this->buttonConfigTextboxes[i]->getCaption());
            joystickModule->remapButton(static_cast<InputDeviceModule::Action>(i), button);
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
}

bool ConfigurationState::keyPressed(const OIS::KeyEvent& keyEventRef)
{
    NOWA::Core::getSingletonPtr()->keyPressed(keyEventRef);

    if (InputDeviceCore::getSingletonPtr()->getKeyboard()->isKeyDown(OIS::KC_ESCAPE))
    {
        this->bQuit = true;
        return true;
    }

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
    return true;
}

bool ConfigurationState::buttonPressed(const OIS::JoyStickEvent& evt, int button)
{
    if (false == this->hasJoystick)
    {
        return true;
    }

    // NOTE: same assumption as populateControlsOptions/applyControlsSettings —
    // adjust the accessor name if your InputDeviceCore differs.
    auto joystickModule = InputDeviceCore::getSingletonPtr()->getJoystickInputDeviceModule(0);

    for (unsigned short i = 0; i < this->buttonConfigTextboxes.size(); i++)
    {
        if (true == this->buttonTextboxActive[i])
        {
            Ogre::String strButton = joystickModule->getStringFromMappedButton(static_cast<InputDeviceModule::JoyStickButton>(button));

            bool alreadyExisting = false;
            for (unsigned short j = 0; j < this->buttonConfigTextboxes.size(); j++)
            {
                if (j != i && this->buttonConfigTextboxes[j]->getCaption() == strButton)
                {
                    alreadyExisting = true;
                    break;
                }
            }

            if (false == alreadyExisting && false == strButton.empty())
            {
                this->buttonConfigTextboxes[i]->setCaption(strButton);
            }

            this->buttonTextboxActive[i] = false;
            this->buttonConfigTextboxes[i]->setTextShadow(false);
            break;
        }
    }

    return true;
}

bool ConfigurationState::buttonReleased(const OIS::JoyStickEvent& evt, int button)
{
    return true;
}
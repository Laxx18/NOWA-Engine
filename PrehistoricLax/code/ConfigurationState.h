#ifndef CONFIGURATION_STATE_H
#define CONFIGURATION_STATE_H

#include "NOWA.h"

/**
    * @brief	Full screen in-game configuration menu with three tabs: Graphics, Sound, Controls.
    *			Pure MyGUI, no layout file, no GameObjectComponent — everything is created programmatically.
    *			Graphics tab mirrors GraphicsConfigurationComponent's option set (resolution, fullscreen,
    *			vsync, vsync interval, anti aliasing, shadow quality). Sound tab controls music/sound volume
    *			via OgreALModule. Controls tab remaps keyboard keys and, if a joystick is active, joystick
    *			buttons — directly via InputDeviceCore's main input device modules (not tied to a game object).
    */
class EXPORTED ConfigurationState : public NOWA::AppState
{
public:
    DECLARE_APPSTATE_CLASS(ConfigurationState)

    ConfigurationState();

    virtual ~ConfigurationState()
    {
    }

    virtual void enter(void) override;
    virtual void start(const NOWA::SceneParameter& sceneParameter);
    virtual void exit(void) override;

    virtual void update(Ogre::Real dt) override;

    virtual bool keyPressed(const OIS::KeyEvent& keyEventRef) override;
    virtual bool keyReleased(const OIS::KeyEvent& keyEventRef) override;

    virtual bool mouseMoved(const OIS::MouseEvent& evt) override;
    virtual bool mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id) override;
    virtual bool mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id) override;

    virtual bool axisMoved(const OIS::JoyStickEvent& evt, int axis) override;
    virtual bool buttonPressed(const OIS::JoyStickEvent& evt, int button) override;
    virtual bool buttonReleased(const OIS::JoyStickEvent& evt, int button) override;

private:
    // ── Setup ────────────────────────────────────────────────────────────
    void createScene(void);
    void setupWidgets(void);
    void createTabButtons(void);
    void createGraphicsTab(void);
    void createSoundTab(void);
    void createControlsTab(void);
    MyGUI::EditBox* createLabel(MyGUI::Widget* parent, const Ogre::String& caption, Ogre::Real posY, Ogre::Real posX = 0.04f, Ogre::Real width = 0.42f);

    // ── Tab switching ───────────────────────────────────────────────────
    void showTab(unsigned short tabIndex);
    void notifyTabButtonClick(MyGUI::Widget* sender);

    // ── Graphics tab logic ──────────────────────────────────────────────
    void populateGraphicsOptions(void);
    void readGraphicsSettingsFromWidgets(void);
    void applyGraphicsSettings(void);
    bool parseResolution(const Ogre::String& videoMode, unsigned int& outWidth, unsigned int& outHeight) const;
    void notifyGraphicsComboAccept(MyGUI::ComboBox* sender, size_t index);
    void notifyCheckBoxClick(MyGUI::Widget* sender);

    // ── Sound tab logic ─────────────────────────────────────────────────
    void populateSoundOptions(void);
    void applySoundSettings(void);
    void notifySoundSliderChangePosition(MyGUI::ScrollBar* sender, size_t position);
    void notifySliderMouseRelease(MyGUI::Widget* sender, int x, int y, MyGUI::MouseButton button);
    void createBackgroundMusic(void);

    // ── Controls tab logic ──────────────────────────────────────────────
    void populateControlsOptions(void);
    void applyControlsSettings(void);
    void notifyKeyEditFocus(MyGUI::Widget* sender, MyGUI::Widget* old);
    void notifyButtonEditFocus(MyGUI::Widget* sender, MyGUI::Widget* old);

    // ── Buttons ─────────────────────────────────────────────────────────
    void buttonHit(MyGUI::Widget* sender);
    void notifyMessageBoxEnd(MyGUI::Message* sender, MyGUI::MessageBoxStyle result);

private:
    static const unsigned short ACTION_COUNT = 7;

    // ── Root widgets ────────────────────────────────────────────────────
    MyGUI::Window* rootWindow;
    MyGUI::Button* tabGraphicsButton;
    MyGUI::Button* tabSoundButton;
    MyGUI::Button* tabControlsButton;
    MyGUI::Widget* graphicsPanel;
    MyGUI::Widget* soundPanel;
    MyGUI::Widget* controlsPanel;
    MyGUI::EditBox* restartRequiredLabel;
    MyGUI::Button* applyButton;
    MyGUI::Button* okButton;
    MyGUI::Button* cancelButton;
    unsigned short currentTabIndex;

    // ── Graphics tab widgets ────────────────────────────────────────────
    MyGUI::ComboBox* resolutionCombo;
    MyGUI::Button* fullscreenCheck;
    MyGUI::Button* vsyncCheck;
    MyGUI::ComboBox* vsyncIntervalCombo;
    MyGUI::ComboBox* fsaaCombo;
    MyGUI::ComboBox* shadowQualityCombo;

    unsigned int initialWidth;
    unsigned int initialHeight;
    bool initialFullscreen;
    bool initialVSync;
    unsigned int initialVSyncInterval;
    Ogre::String initialFsaa;
    short initialShadowQuality;
    bool graphicsRestartRequired;

    // ── Sound tab widgets ───────────────────────────────────────────────
    MyGUI::ScrollBar* soundSlider;
    MyGUI::ScrollBar* musicSlider;
    MyGUI::EditBox* soundLabel;
    MyGUI::EditBox* musicLabel;
    OgreAL::Sound* menuMusic;
    OgreAL::Sound* soundMusic;

    // ── Controls tab widgets ────────────────────────────────────────────
    std::vector<MyGUI::EditBox*> keyConfigTextboxes;
    std::vector<Ogre::String> oldKeyValue;
    std::vector<bool> keyTextboxActive;

    bool hasJoystick;
    std::vector<MyGUI::EditBox*> buttonConfigTextboxes;
    std::vector<Ogre::String> oldButtonValue;
    std::vector<bool> buttonTextboxActive;
};

#endif
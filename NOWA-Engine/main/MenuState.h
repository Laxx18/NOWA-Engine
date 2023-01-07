#ifndef MENU_STATE_H
#define MENU_STATE_H

#include "AppState.h"
#include "modules/OgreALModule.h"

namespace NOWA
{

	//Der Menuezustand ist eine Klasse, welche die AppState Klasse implementiert
	//Durch die Ableitung von AppState wird sichergestellt, dass alle Zustaende gemeinsame Methoden wie enter(), pause(), resume() oder update() haben
	class EXPORTED MenuState : public AppState
	{
	public:
		//Dieser Makroaufruf macht die Klasse zu einem validen Andwendungszustand
		DECLARE_APPSTATE_CLASS(MenuState)

		MenuState();

		void enter(void);
		void createScene(void);
		void exit(void);

		bool keyPressed(const OIS::KeyEvent &keyEventRef);
		bool keyReleased(const OIS::KeyEvent &keyEventRef);

		bool mouseMoved(const OIS::MouseEvent &evt);
		bool mousePressed(const OIS::MouseEvent &evt, OIS::MouseButtonID id);
		bool mouseReleased(const OIS::MouseEvent &evt, OIS::MouseButtonID id);

		void update(Ogre::Real dt);
	public:
		static void showCameraSettings(bool bShow);

		static bool bShowCameraSettings;
	private:
		void buttonHit(OgreBites::Button *pButton);
		void itemSelected(OgreBites::SelectMenu *pMenu);
		void sliderMoved(OgreBites::Slider *pSlider);
		void yesNoDialogClosed(const Ogre::DisplayString &question, bool yesHit);
		void setupWidgets(void);
		void populateOptions(void);
		void switchConfiguration(int state);
		void applyOptions(void);
		void createBackgroundMusic(void);
	private:
		OgreBites::Button* enterButton;
		OgreBites::Button* optionsButton;
		OgreBites::Button* exitButton;
		// render system selectionsmenue
		OgreBites::SelectMenu* rendererMenu;
		OgreBites::Button* configurationButton;
		OgreBites::Button* wiiConfigurationButton;
		OgreBites::Button* audioConfigurationButton;
		OgreBites::Button* keyConfigurationButton;
		std::vector<OgreBites::TextBox*> keyConfigTextboxes;
		std::vector<bool> textboxActive;
		OgreBites::Separator* configSeparator;
		OgreBites::SelectMenu* fSAAMenu;
		OgreBites::SelectMenu* fullscreenMenu;
		OgreBites::SelectMenu* vSyncMenu;
		OgreBites::SelectMenu* videoModeMenu;
		OgreBites::Slider* viewRangeSlider;
		OgreBites::Slider* lODBiasSlider;
		OgreBites::SelectMenu* textureFilteringMenu;
		OgreBites::Slider* anisotropySlider;
		OgreBites::Label* infoLabel;
		OgreBites::Button* optionsApplyButton;
		OgreBites::Button* optionsAbordButton;
		Ogre::DisplayString	strOldRenderSystem;
		Ogre::DisplayString	strOldFSAA;
		Ogre::DisplayString	strOldFullscreen;
		Ogre::DisplayString	strOldVSync;
		Ogre::DisplayString	strOldVideoMode;
		// Wii
		OgreBites::Label* wiiInfoLabel;
		OgreBites::CheckBox* wiiUseCheckBox;
		OgreBites::Label* wiiBatteryLevelLabel;
		OgreBites::SelectMenu* wiiIRPosMenu;
		OgreBites::SelectMenu* wiiAspectRatioMenu;
		// Audio
		OgreBites::Slider* soundVolumeSlider;
		OgreBites::Slider* musicVolumeSlider;

		OgreAL::Sound* menuMusic;
	};

}; //naemspace end

#endif
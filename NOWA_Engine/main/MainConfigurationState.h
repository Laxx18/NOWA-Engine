#ifndef MAIN_CONFIGURATION_STATE_H
#define MAIN_CONFIGURATION_STATE_H

#include "AppState.h"
#include "modules/OgreALModule.h"

namespace NOWA
{

	//Der NetzwerkMenuezustand ist eine Klasse, welche die AppState Klasse implementiert
	//Durch die Ableitung von AppState wird sichergestellt, dass alle Zustaende gemeinsame Methoden wie enter(), pause(), resume() oder update() haben
	class EXPORTED MainConfigurationState : public AppState
	{
	public:
		//Dieser Makroaufruf macht die Klasse zu einem validen Andwendungszustand
		DECLARE_APPSTATE_CLASS(MainConfigurationState)

		MainConfigurationState();

		bool keyPressed(const OIS::KeyEvent &keyEventRef);
		bool keyReleased(const OIS::KeyEvent &keyEventRef);

		bool mouseMoved(const OIS::MouseEvent &evt);
		bool mousePressed(const OIS::MouseEvent &evt, OIS::MouseButtonID id);
		bool mouseReleased(const OIS::MouseEvent &evt, OIS::MouseButtonID id);

		void enter(void);
		void createScene(void);
		void exit(void);
		void update(Ogre::Real dt);

	protected:
		void initState(const Ogre::String& strWindowTitle);
		void buttonHit(OgreBites::Button *pButton);
		void itemSelected(OgreBites::SelectMenu *pMenu);
		void sliderMoved(OgreBites::Slider *pSlider);
		void yesNoDialogClosed(const Ogre::DisplayString &question, bool yesHit);
		void setupWidgets(void);
		void populateOptions(void);
		void switchConfiguration(int state);
		void applyOptions(void);
		void createBackgroundMusic(void);
		void checkWiiAvailable(void);
	protected:
		OgreAL::Sound* menuMusic;
		//Wird als public benoetigt damit die Wii verbunden werden kann
		Ogre::Camera* camera;

		Ogre::SceneManager* sceneManager;
	private:
		OgreBites::Button* clientEnterButton;
		OgreBites::Button* serverEnterButton;
		OgreBites::Button* optionsButton;
		OgreBites::Button* exitButton;
		// render system selektionsmenue
		OgreBites::SelectMenu		*rendererMenu;
		OgreBites::Button			*configurationButton;
		OgreBites::Button			*wiiConfigurationButton;
		OgreBites::Button			*audioConfigurationButton;
		OgreBites::Separator		*configSeparator;
		OgreBites::SelectMenu		*fSAAMenu;
		OgreBites::SelectMenu		*fullscreenMenu;
		OgreBites::SelectMenu		*vSyncMenu;
		OgreBites::SelectMenu		*videoModeMenu;
		OgreBites::Slider			*viewRangeSlider;
		OgreBites::Slider			*lODBiasSlider;
		OgreBites::SelectMenu		*textureFilteringMenu;
		OgreBites::Slider			*anisotropySlider;
		OgreBites::Label 			*infoLabel;
		OgreBites::Button			*optionsApplyButton;
		OgreBites::Button			*optionsAbordButton;
		Ogre::DisplayString			strOldRenderSystem;
		Ogre::DisplayString			strOldFSAA;
		Ogre::DisplayString			strOldFullscreen;
		Ogre::DisplayString			strOldVSync;
		Ogre::DisplayString			strOldVideoMode;
		//Wii
		OgreBites::Label			*wiiInfoLabel;
		OgreBites::CheckBox			*wiiUseCheckBox;
		OgreBites::Label			*wiiBatteryLevelLabel;
		OgreBites::SelectMenu		*wiiIRPosMenu;
		OgreBites::SelectMenu		*wiiAspectRatioMenu;
		//Audio
		OgreBites::Slider			*soundVolumeSlider;
		OgreBites::Slider			*musicVolumeSlider;

	};

}; // namespace end

#endif
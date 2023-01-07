#ifndef CLIENT_CONFIGURATION_STATE_H
#define CLIENT_CONFIGURATION_STATE_H

#include "AppState.h"
#include "defines.h"
#include "modules/OgreALModule.h"

namespace NOWA
{

	class EXPORTED ClientConfigurationState : public AppState
	{
	public:
		DECLARE_APPSTATE_CLASS(ClientConfigurationState)

		ClientConfigurationState();

		void enter(void);

		void update(Ogre::Real dt);

		void createScene(void);

		void exit(void);

		bool keyPressed(const OIS::KeyEvent& keyEventRef);
		bool keyReleased(const OIS::KeyEvent& keyEventRef);

		bool mouseMoved(const OIS::MouseEvent& evt);
		bool mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id);
		bool mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id);
	private:
		void buttonHit(OgreBites::Button* button);

		void itemSelected(OgreBites::SelectMenu* menu);

		void sliderMoved(OgreBites::Slider* slider);

		void yesNoDialogClosed(const Ogre::DisplayString& question, bool yesHit);

		void okDialogClosed(const Ogre::DisplayString& message);

		void setupWidgets(void);

		void populateMenus(void);

		void createBackgroundMusic(void);

		void searchServer(void);

		void disconnect(void);
	private:
		OgreBites::Button* startGameButton;
		OgreBites::Button* connectToServerButton;
		OgreBites::Button* backButton;
		OgreBites::Button* searchServerButton;
		OgreBites::Label* serverIPLabel;
		OgreBites::TextBox* setServerIPTextBox;
		OgreBites::Button* setServerIPButton;
		OgreBites::SelectMenu* selectPlayerMenu;
		OgreBites::Label* foundServerLabel;
		OgreBites::TextBox* nameTextBox;
		OgreBites::Button* nameOkButton;
		OgreBites::SelectMenu* packetSendRateMenu;
		OgreBites::SelectMenu* interpolationTimeMenu;
		bool interpolationFirstTime;
		bool nameTextBoxActive;
		bool setServerIPTextBoxActive;
		OgreAL::Sound* menuMusic;
	};

}; // namespace end


#endif

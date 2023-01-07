#pragma once

#ifndef SERVER_CONFIGURATION_STATE_H
#define SERVER_CONFIGURATION_STATE_H

#include "AppState.h"

namespace NOWA
{

class EXPORTED ServerConfigurationState : public AppState
{
public:
	DECLARE_APPSTATE_CLASS(ServerConfigurationState)

	ServerConfigurationState();

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

	void setupWidgets(void);
	
	void populateVirtualEnvironments(void);
private:
	OgreBites::Button* serverSimulationButton;
	OgreBites::Button* serverConsoleButton;
	OgreBites::TextBox* serverNameTextBox;
	OgreBites::Button* serverNameButton;
	OgreBites::SelectMenu* mapMenu;
	OgreBites::Label* maxPlayerLabel;
	OgreBites::Button* backButton;
	OgreBites::Separator* configSeparator;
	OgreBites::Slider* areaOfInterestSlider;
	OgreBites::Slider* packetSendRateSlider;
	bool serverNameTextBoxActive;
	unsigned short clientNumber;
};

}; // namespace end

#endif


#ifndef SERVER_CONFIGURATION_STATE_MY_GUI_H
#define SERVER_CONFIGURATION_STATE_MY_GUI_H

#include "AppState.h"

namespace NOWA
{

	class EXPORTED ServerConfigurationStateMyGui : public AppState
	{
	public:
		DECLARE_APPSTATE_CLASS(ServerConfigurationStateMyGui)

		ServerConfigurationStateMyGui();

		virtual ~ServerConfigurationStateMyGui() { }

		virtual void enter(void) override;

		virtual void exit(void) override;

		virtual void update(Ogre::Real dt) override;

		virtual bool keyPressed(const OIS::KeyEvent& keyEventRef) override;
		virtual bool keyReleased(const OIS::KeyEvent& keyEventRef) override;

		virtual bool mouseMoved(const OIS::MouseEvent& evt);
		virtual bool mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id) override;
		virtual bool mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id) override;

		virtual bool axisMoved(const OIS::JoyStickEvent& evt, int axis) override;
		virtual bool buttonPressed(const OIS::JoyStickEvent& evt, int button) override;
		virtual bool buttonReleased(const OIS::JoyStickEvent& evt, int button) override;

	private:
		void createScene(void);
		void itemSelected(MyGUI::ComboBox* _sender, size_t _index);
		void buttonHit(MyGUI::Widget* _sender);
		void editChange(MyGUI::EditBox* _sender);
		void sliderMoved(MyGUI::ScrollBar* _sender, size_t _position);

		void setupWidgets(void);

		void populateOptions(void);

		void applyOptions(void);
	private:
		unsigned short clientNumber;

		MyGUI::VectorWidgetPtr widgets;

		MyGUI::ComboBox* mapsCombo;
		MyGUI::ComboBox* packetSendRateCombo;
		MyGUI::ScrollBar* areaOfInterestSlider;
	};

}; // namespace end

#endif


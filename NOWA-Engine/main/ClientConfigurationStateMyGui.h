#ifndef CLIENT_CONFIGURATION_STATE_MY_GUI_H
#define CLIENT_CONFIGURATION_STATE_MY_GUI_H

#include "AppState.h"
#include "modules/OgreALModule.h"

namespace NOWA
{

	class EXPORTED ClientConfigurationStateMyGui : public AppState
	{
	public:
		DECLARE_APPSTATE_CLASS(ClientConfigurationStateMyGui)

		ClientConfigurationStateMyGui();

		virtual ~ClientConfigurationStateMyGui() { }

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

		void setupWidgets(void);

		void populateOptions(void);

		void createBackgroundMusic(void);

		void applyOptions(void);

		void searchServer(void);

		void disconnect(void);
	private:
		MyGUI::VectorWidgetPtr widgets;

		MyGUI::ComboBox* playerColorCombo;
		MyGUI::ComboBox* packetsPerSecondCombo;
		MyGUI::ComboBox* interpolationRateCombo;

		bool interpolationFirstTime;
		OgreAL::Sound* menuMusic;
	};

}; // namespace end


#endif

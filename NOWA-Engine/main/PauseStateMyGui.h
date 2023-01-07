#ifndef PAUSE_STATE_MY_GUI_H
#define PAUSE_STATE_MY_GUI_H

#include "AppState.h"
#include "MyGUI.h"
#include "MyGUI_Ogre2Platform.h"
#include "MessageBox/MessageBox.h"

namespace NOWA
{

	class EXPORTED PauseStateMyGui : public AppState
	{
	public:
		DECLARE_APPSTATE_CLASS(PauseStateMyGui)

		PauseStateMyGui();

		virtual ~PauseStateMyGui() { }

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
		void buttonHit(MyGUI::WidgetPtr _sender);
		void notifyMessageBoxEnd(MyGUI::Message* _sender, MyGUI::MessageBoxStyle _result);
	private:
		MyGUI::VectorWidgetPtr widgets;
	};

}; // namespace end

#endif

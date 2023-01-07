#ifndef NOWA_INTRO_STATE_H
#define NOWA_INTRO_STATE_H

#include "main/AppState.h"
#include "MyGUI.h"
#include "MyGUI_Ogre2Platform.h"
#include "MessageBox/MessageBox.h"

namespace NOWA
{

	class EXPORTED IntroState : public AppState
	{
	public:
		IntroState();

		virtual ~IntroState() { }

		DECLARE_APPSTATE_CLASS(IntroState)

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
	private:
		MyGUI::VectorWidgetPtr widgets;
		Ogre::Real introTimeDt;
	};

}; // namespace end

#endif

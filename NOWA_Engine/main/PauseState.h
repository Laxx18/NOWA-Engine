#ifndef PAUSE_STATE_H
#define PAUSE_STATE_H

#include "AppState.h"

namespace NOWA
{

	class EXPORTED PauseState : public AppState
	{
	public:
		PauseState();

		DECLARE_APPSTATE_CLASS(PauseState)

		void enter(void);
		void createScene(void);
		void exit(void);

		bool keyPressed(const OIS::KeyEvent &keyEventRef);
		bool keyReleased(const OIS::KeyEvent &keyEventRef);

		bool mouseMoved(const OIS::MouseEvent &evt);
		bool mousePressed(const OIS::MouseEvent &evt, OIS::MouseButtonID id);
		bool mouseReleased(const OIS::MouseEvent &evt, OIS::MouseButtonID id);

		void update(Ogre::Real dt);

	private:
		void buttonHit(OgreBites::Button *pButton);
		void yesNoDialogClosed(const Ogre::DisplayString& question, bool yesHit);

	};

}; // namespace end

#endif

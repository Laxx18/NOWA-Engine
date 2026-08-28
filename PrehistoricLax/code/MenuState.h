#ifndef MENU_STATE_H
#define MENU_STATE_H

#include "NOWA.h"

class MenuState : public NOWA::AppState
{
public:
	DECLARE_APPSTATE_CLASS(MenuState)

	MenuState();

	virtual ~MenuState() { }

	/**
	 * @see AppState::enter()
	 */
	virtual void enter(void) override;

	/**
	 * @see AppState::start()
	 */
	virtual void start(const NOWA::SceneParameter& sceneParameter) override;

	/**
	 * @see AppState::exit()
	 */
	virtual void exit(void) override;

	/**
	 * @see AppState::update()
	 */
	virtual void update(Ogre::Real dt) override;

	/**
	 * @see AppState::renderUpdate()
	 */
	virtual void renderUpdate(Ogre::Real dt) override;

	/**
	 * @see AppState::keyPressed()
	 */
	virtual bool keyPressed(const OIS::KeyEvent& keyEventRef) override;

	/**
	 * @see AppState::keyReleased()
	 */
	virtual bool keyReleased(const OIS::KeyEvent& keyEventRef) override;

	/**
	 * @see AppState::mousePressed()
	 */
	virtual bool mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id) override;

	/**
	 * @see AppState::mouseReleased()
	 */
	virtual bool mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id) override;

	/**
	 * @see AppState::mouseMoved()
	 */
	virtual bool mouseMoved(const OIS::MouseEvent& evt) override;

	/**
	 * @see AppState::axisMoved()
	 */
	virtual bool axisMoved(const OIS::JoyStickEvent& evt, int axis) override;

	/**
	 * @see AppState::buttonPressed()
	 */
	virtual bool buttonPressed(const OIS::JoyStickEvent& evt, int button) override;

	/**
	 * @see AppState::buttonReleased()
	 */
	virtual bool buttonReleased(const OIS::JoyStickEvent& evt, int button) override;

private:
	void notifyMessageBoxEnd(MyGUI::Message* sender, MyGUI::MessageBoxStyle result);

	void processUnbufferedKeyInput(Ogre::Real dt);

	void processUnbufferedMouseInput(Ogre::Real dt);
private:
	OgreAL::Sound* menuMusic;
};

#endif

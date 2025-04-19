#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "NOWA.h"

class GameState : public NOWA::AppState
{
public:
	DECLARE_APPSTATE_CLASS(GameState)

	GameState();

	virtual ~GameState() { }

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
	 * @brief		Actions on key pressed event.
	 * @see			OIS::KeyListener::keyPressed()
	 * @param[in]	keyEventRef		The key event
	 * @return		true			if other key listener successors shall have the chance to react or not.
	 */
	virtual bool keyPressed(const OIS::KeyEvent& keyEventRef) override;

	/**
	 * @brief		Actions on key released event.
	 * @see			OIS::KeyListener::keyReleased()
	 * @param[in]	keyEventRef		The key event
	 * @return		true			if other key listener successors shall have the chance to react or not.
	 */
	virtual bool keyReleased(const OIS::KeyEvent& keyEventRef) override;

	/**
	 * @brief		Actions on mouse moved event.
	 * @see			OIS::MouseListener::mouseMoved()
	 * @param[in]	evt		The mouse event
	 * @return		true			if other key listener successors shall have the chance to react or not.
	 */
	virtual bool mouseMoved(const OIS::MouseEvent& evt) override;

	/**
	 * @brief		Actions on mouse pressed event.
	 * @see			OIS::MouseListener::mousePressed()
	 * @param[in]	evt		The mouse event
	 * @return		true			if other key listener successors shall have the chance to react or not.
	 */
	virtual bool mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id) override;

	/**
	 * @brief		Actions on mouse released event.
	 * @see			OIS::MouseListener::mouseReleased()
	 * @param[in]	evt		The mouse event
	 * @return		true			if other key listener successors shall have the chance to react or not.
	 */
	virtual bool mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id) override;

	/**
	 * @brief		Actions on joyStick axis moved event.
	 * @see			OIS::JoyStickListener::axisMoved()
	 * @param[in]	evt		The joyStick event
	 * @return		true			if other key listener successors shall have the chance to react or not.
	 */
	virtual bool axisMoved(const OIS::JoyStickEvent& evt, int axis) override;

	/**
	 * @brief		Actions on joyStick button pressed event.
	 * @see			OIS::JoyStickListener::buttonPressed()
	 * @param[in]	evt		The joyStick event
	 * @return		true			if other key listener successors shall have the chance to react or not.
	 */
	virtual bool buttonPressed(const OIS::JoyStickEvent& evt, int button) override;

	/**
	 * @brief		Actions on joyStick button released event.
	 * @see			OIS::JoyStickListener::buttonReleased()
	 * @param[in]	evt		The joyStick event
	 * @return		true			if other key listener successors shall have the chance to react or not.
	 */
	virtual bool buttonReleased(const OIS::JoyStickEvent& evt, int button) override;
	
private:
	void notifyMessageBoxEnd(MyGUI::Message* sender, MyGUI::MessageBoxStyle result);
	
	void processUnbufferedKeyInput(Ogre::Real dt);

	void processUnbufferedMouseInput(Ogre::Real dt);
};

#endif

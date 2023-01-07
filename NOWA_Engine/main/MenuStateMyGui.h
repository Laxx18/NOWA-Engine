#ifndef MENU_STATE_MY_GUI_H
#define MENU_STATE_MY_GUI_H

#include "AppState.h"
#include "modules/OgreALModule.h"

namespace NOWA
{
	class EXPORTED MenuStateMyGui : public AppState
	{
	public:
		DECLARE_APPSTATE_CLASS(MenuStateMyGui)

		MenuStateMyGui();

		virtual ~MenuStateMyGui() { }

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

		
	public:
		static void showCameraSettings(bool bShow);

		static bool bShowCameraSettings;
	private:
		void createScene(void);
		void itemSelected(MyGUI::ComboBox* _sender, size_t _index);
		void sliderMoved(MyGUI::ScrollBar* _sender, size_t _position);
		void buttonHit(MyGUI::Widget* _sender);
		void notifyMessageBoxEnd(MyGUI::Message* _sender, MyGUI::MessageBoxStyle _result);
		void notifyMouseSetFocus(MyGUI::Widget* _sender, MyGUI::Widget* _old);
		void setupWidgets(void);
		void populateOptions(void);
		void applyOptions(void);
		void createBackgroundMusic(void);
	private:
		MyGUI::VectorWidgetPtr widgets;
		std::vector<bool> textboxActive;
		OgreAL::Sound* menuMusic;
		MyGUI::ComboBox* renderSystemCombo;
		MyGUI::ComboBox* antiAliasingCombo;
		MyGUI::ComboBox* fullscreenCombo;
		MyGUI::ComboBox* vSyncCombo;
		MyGUI::ComboBox* resolutionCombo;
		MyGUI::ComboBox* graphicsQualityCombo;
		MyGUI::ComboBox* languageCombo;
		MyGUI::ScrollBar* soundSlider;
		MyGUI::ScrollBar* musicSlider;
		std::vector<MyGUI::EditBox*> keyConfigTextboxes;

		Ogre::String strOldRenderSystem;
		Ogre::String strOldFSAA;
		Ogre::String strOldFullscreen;
		Ogre::String strOldVSync;
		Ogre::String strOldResolution;
		Ogre::String strOldQuality;
		Ogre::String strOldLanguage;
		std::vector<Ogre::String> oldKeyValue;
	};

}; // namespace end

#endif
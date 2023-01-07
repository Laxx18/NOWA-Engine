#include "NOWAPrecompiled.h"
#include "MainConfigurationStateMyGui.h"
#include "Core.h"
#include "InputDeviceCore.h"
#include "utilities/FaderProcess.h"
#include "modules/InputDeviceModule.h"
#include "main/AppStateManager.h"

namespace NOWA
{

	bool MainConfigurationStateMyGui::bShowCameraSettings = true;

	MainConfigurationStateMyGui::MainConfigurationStateMyGui()
		: AppState()
	{
		// Do not init anything here
	}

	void MainConfigurationStateMyGui::enter(void)
	{
		NOWA::AppState::enter();

		this->menuMusic = nullptr;
		this->renderSystemCombo = nullptr;
		this->antiAliasingCombo = nullptr;
		this->fullscreenCombo = nullptr;
		this->vSyncCombo = nullptr;
		this->resolutionCombo = nullptr;
		this->graphicsQualityCombo = nullptr;
		this->languageCombo = nullptr;
		this->soundSlider = nullptr;
		this->musicSlider = nullptr;

		ProcessManager::getInstance()->attachProcess(ProcessPtr(new FaderProcess(FaderProcess::FadeOperation::FADE_IN, 3.5f)));
		Ogre::LogManager::getSingletonPtr()->logMessage("Entering MainConfigurationStateMyGui...");

		this->sceneManager = Core::getSingletonPtr()->getOgreRoot()->createSceneManager(Ogre::ST_GENERIC, 1, "MainConfigurationStateMyGui");
		// this->sceneManager->setAmbientLight(Ogre::ColourValue(0.7f, 0.7f, 0.7f));
		this->sceneManager->addRenderQueueListener(Core::getSingletonPtr()->getOverlaySystem());
		this->sceneManager->getRenderQueue()->setSortRenderQueue(Ogre::v1::OverlayManager::getSingleton().mDefaultRenderQueueId, Ogre::RenderQueue::StableSort);

		this->camera = this->sceneManager->createCamera("MenuCamera");
		this->camera->setPosition(Ogre::Vector3(0, 25, -50));
		this->camera->lookAt(Ogre::Vector3(0, 0, 0));
		this->camera->setNearClipDistance(1);
		this->camera->setAutoAspectRatio(true);

		WorkspaceModule::getInstance()->setPrimaryWorkspace(this->sceneManager, this->camera, nullptr);

		this->initializeModules(false, false);

		this->setupWidgets();

		this->createScene();
		this->createBackgroundMusic();
	}

	void MainConfigurationStateMyGui::exit()
	{
		NOWA::AppState::exit();

		ProcessManager::getInstance()->attachProcess(ProcessPtr(new FaderProcess(FaderProcess::FadeOperation::FADE_OUT, 2.5f)));

		this->canUpdate = false;

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MainConfigurationStateMyGui] Leaving...");

		MyGUI::Gui::getInstancePtr()->destroyWidget(MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::ImageBox>("Background"));
		MyGUI::LayoutManager::getInstancePtr()->unloadLayout(this->widgets);
		this->menuMusic = nullptr;
		this->renderSystemCombo = nullptr;
		this->antiAliasingCombo = nullptr;
		this->fullscreenCombo = nullptr;
		this->vSyncCombo = nullptr;
		this->resolutionCombo = nullptr;
		this->graphicsQualityCombo = nullptr;
		this->languageCombo = nullptr;
		this->soundSlider = nullptr;
		this->musicSlider = nullptr;

		this->destroyModules();
	}

	void MainConfigurationStateMyGui::showCameraSettings(bool bShow)
	{
		MainConfigurationStateMyGui::bShowCameraSettings = bShow;
	}

	void MainConfigurationStateMyGui::createBackgroundMusic(void)
	{
		// create sound
		OgreALModule::getInstance()->init(this->sceneManager);
		this->menuMusic = OgreALModule::getInstance()->createSound(this->sceneManager, "MenuMusic1", "Lines Of Code.ogg", true, true);
		this->menuMusic->play();
	}

	void MainConfigurationStateMyGui::setupWidgets(void)
	{
		// Create Gui
		Core::getSingletonPtr()->setSceneManagerForMyGuiPlatform(this->sceneManager);

		MyGUI::Gui::getInstancePtr()->createWidget<MyGUI::ImageBox>("RotatingSkin",
			MyGUI::IntCoord(0, 0, Core::getSingletonPtr()->getOgreRenderWindow()->getWidth(), Core::getSingletonPtr()->getOgreRenderWindow()->getHeight()),
			MyGUI::Align::Default, "Overlapped", "Background")->setImageTexture("BackgroundShadeBlue.png");

		this->widgets = MyGUI::LayoutManager::getInstancePtr()->loadLayout("MainConfigurationState.layout");
		// http://www.ogre3d.org/addonforums/viewtopic.php?f=17&t=12934&p=72481&hilit=inputmanager+unloadLayout#p72481
		MyGUI::FloatPoint windowPosition;
		windowPosition.left = 0.1f;
		windowPosition.top = 0.1f;
		MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Window>("menueStatePanel")->setRealPosition(windowPosition);
		MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Window>("configurationPanel")->setRealPosition(windowPosition);

		MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("clientEnterButton")->eventMouseButtonClick += MyGUI::newDelegate(this, &MainConfigurationStateMyGui::buttonHit);
		MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("serverEnterButton")->eventMouseButtonClick += MyGUI::newDelegate(this, &MainConfigurationStateMyGui::buttonHit);
		MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("optionsButton")->eventMouseButtonClick += MyGUI::newDelegate(this, &MainConfigurationStateMyGui::buttonHit);
		MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("quitButton")->eventMouseButtonClick += MyGUI::newDelegate(this, &MainConfigurationStateMyGui::buttonHit);

		MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("applyButton")->eventMouseButtonClick += MyGUI::newDelegate(this, &MainConfigurationStateMyGui::buttonHit);
		MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("abordButton")->eventMouseButtonClick += MyGUI::newDelegate(this, &MainConfigurationStateMyGui::buttonHit);

		this->renderSystemCombo = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::ComboBox>("renderSystemCombo");
		this->antiAliasingCombo = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::ComboBox>("antiAliasingCombo");
		this->fullscreenCombo = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::ComboBox>("fullscreenCombo");
		this->vSyncCombo = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::ComboBox>("vSyncCombo");
		this->resolutionCombo = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::ComboBox>("resolutionCombo");
		this->graphicsQualityCombo = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::ComboBox>("graphicsQualityCombo");
		this->languageCombo = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::ComboBox>("languageCombo");

		this->renderSystemCombo->eventComboChangePosition += MyGUI::newDelegate(this, &MainConfigurationStateMyGui::itemSelected);
		this->antiAliasingCombo->eventComboChangePosition += MyGUI::newDelegate(this, &MainConfigurationStateMyGui::itemSelected);
		this->fullscreenCombo->eventComboChangePosition += MyGUI::newDelegate(this, &MainConfigurationStateMyGui::itemSelected);
		this->vSyncCombo->eventComboChangePosition += MyGUI::newDelegate(this, &MainConfigurationStateMyGui::itemSelected);
		this->resolutionCombo->eventComboChangePosition += MyGUI::newDelegate(this, &MainConfigurationStateMyGui::itemSelected);
		this->graphicsQualityCombo->eventComboChangePosition += MyGUI::newDelegate(this, &MainConfigurationStateMyGui::itemSelected);
		this->languageCombo->eventComboChangePosition += MyGUI::newDelegate(this, &MainConfigurationStateMyGui::itemSelected);

		this->soundSlider = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::ScrollBar>("soundSlider");
		this->musicSlider = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::ScrollBar>("musicSlider");

		this->soundSlider->eventScrollChangePosition += MyGUI::newDelegate(this, &MainConfigurationStateMyGui::sliderMoved);
		this->musicSlider->eventScrollChangePosition += MyGUI::newDelegate(this, &MainConfigurationStateMyGui::sliderMoved);

		if (true == NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->hasActiveJoyStick())
			this->textboxActive.resize(7 + 7, false);
		else
			this->textboxActive.resize(7, false);
		
		// Key-configuration widgets
		this->keyConfigTextboxes.resize(7);
		
		this->oldKeyValue.resize(7);
		this->keyConfigTextboxes[0] = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("moveUpEdit");
		this->keyConfigTextboxes[1] = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("moveDownEdit");
		this->keyConfigTextboxes[2] = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("moveLeftEdit");
		this->keyConfigTextboxes[3] = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("moveRightEdit");
		this->keyConfigTextboxes[4] = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("jumpEdit");
		this->keyConfigTextboxes[5] = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("action1Edit");
		this->keyConfigTextboxes[6] = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("action2Edit");

		for (unsigned short i = 0; i < keyConfigTextboxes.size(); i++)
		{
			auto keyCode = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedKey(static_cast<InputDeviceModule::Action>(i));
			Ogre::String strKeyCode = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getStringFromMappedKey(keyCode);
			this->oldKeyValue[i] = strKeyCode;
			// Ogre::String strKeyCode = NOWA::Core::getSingletonPtr()->getKeyboard()->getAsString(keyCode);
			this->keyConfigTextboxes[i]->setNeedMouseFocus(true);
			this->keyConfigTextboxes[i]->setCaptionWithReplacing(strKeyCode);
			this->keyConfigTextboxes[i]->eventMouseSetFocus += MyGUI::newDelegate(this, &MainConfigurationStateMyGui::notifyMouseSetFocus);
		}

		if (true == NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->hasActiveJoyStick())
		{
			// Button-configuration widgets
			this->buttonConfigTextboxes.resize(7);
			this->textboxActive.resize(7, false);
			this->oldButtonValue.resize(7);
			this->buttonConfigTextboxes[0] = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("moveUpEdit");
			this->buttonConfigTextboxes[1] = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("moveDownEdit");
			this->buttonConfigTextboxes[2] = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("moveLeftEdit");
			this->buttonConfigTextboxes[3] = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("moveRightEdit");
			this->buttonConfigTextboxes[4] = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("jumpEdit");
			this->buttonConfigTextboxes[5] = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("action1Edit");
			this->buttonConfigTextboxes[6] = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("action2Edit");

			for (unsigned short i = 0; i < buttonConfigTextboxes.size(); i++)
			{
				auto button = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedButton(static_cast<InputDeviceModule::Action>(i));
				Ogre::String strButton = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getStringFromMappedButton(button);
				this->oldButtonValue[i] = strButton;
				// Ogre::String strKeyCode = NOWA::Core::getSingletonPtr()->getKeyboard()->getAsString(keyCode);
				this->buttonConfigTextboxes[i]->setNeedMouseFocus(true);
				this->buttonConfigTextboxes[i]->setCaptionWithReplacing(strButton);
				this->buttonConfigTextboxes[i]->eventMouseSetFocus += MyGUI::newDelegate(this, &MainConfigurationStateMyGui::notifyMouseSetFocusJoyStick);
			}
		}

		// fill with data
		this->populateOptions();
	}

	void MainConfigurationStateMyGui::populateOptions(void)
	{
		// load some options and set the GUI states
		// Ogre::TextureManager::getSingleton().setDefaultNumMipmaps(5);
		// Ogre::MaterialManager::getSingleton().setDefaultTextureFiltering(static_cast<Ogre::TextureFilterOptions>(Core::getSingletonPtr()->getOptionTextureFiltering()));
		// Ogre::MaterialManager::getSingleton().setDefaultAnisotropy(Core::getSingletonPtr()->getOptionAnisotropyLevel());

		this->soundSlider->setScrollPosition(Core::getSingletonPtr()->getOptionSoundVolume());
		this->musicSlider->setScrollPosition(Core::getSingletonPtr()->getOptionMusicVolume());

		OgreALModule::getInstance()->setupVolumes(static_cast<int>(this->soundSlider->getScrollPosition()), static_cast<int>(this->musicSlider->getScrollPosition()));
		MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("musicLabel")->setCaptionWithReplacing("#{Music_Volume}: ("
			+ Ogre::StringConverter::toString(musicSlider->getScrollPosition()) + " %)");

		MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("soundLabel")->setCaptionWithReplacing("#{Sound_Volume}: ("
			+ Ogre::StringConverter::toString(soundSlider->getScrollPosition()) + " %)");

		// Render systems
		const Ogre::RenderSystemList& rsList = Core::getSingletonPtr()->getOgreRoot()->getAvailableRenderers();
		for (unsigned int i = 0; i < rsList.size(); i++)
		{
			this->renderSystemCombo->addItem(rsList[i]->getName());
		}
		this->renderSystemCombo->setCaption(Core::getSingletonPtr()->getOgreRoot()->getRenderSystem()->getName());
		size_t index = this->renderSystemCombo->findItemIndexWith(Core::getSingletonPtr()->getOgreRoot()->getRenderSystem()->getName());
		this->renderSystemCombo->setIndexSelected(index);
		this->strOldRenderSystem = renderSystemCombo->getItemNameAt(index);
		Ogre::ConfigOptionMap& configOptions = Core::getSingletonPtr()->getOgreRoot()->getRenderSystemByName(renderSystemCombo->getItemNameAt(index))->getConfigOptions();
		Ogre::ConfigOption configOption;
		Ogre::StringVector possibleOptions;

		// Anti-Aliasing
		configOption = configOptions.find("FSAA")->second;
		possibleOptions = configOption.possibleValues;
		for (unsigned int i = 0; i < possibleOptions.size(); i++)
		{
			this->antiAliasingCombo->addItem(possibleOptions.at(i));
		}
		this->antiAliasingCombo->setCaption(configOption.currentValue);
		index = this->antiAliasingCombo->findItemIndexWith(configOption.currentValue);
		this->antiAliasingCombo->setIndexSelected(index);
		this->strOldFSAA = this->antiAliasingCombo->getItemNameAt(index);

		// Fullscreen
		this->fullscreenCombo->addItem(MyGUI::LanguageManager::getInstancePtr()->replaceTags("#{MessageBox_Yes}"));
		this->fullscreenCombo->addItem(MyGUI::LanguageManager::getInstancePtr()->replaceTags("#{MessageBox_No}"));
		configOption = configOptions.find("Full Screen")->second;
		if ("Yes" == configOption.currentValue)
		{
			index = 0;
		}
		else
		{
			index = 1;
		}
		this->fullscreenCombo->setIndexSelected(index);
		this->strOldFullscreen = this->fullscreenCombo->getItemNameAt(index);

		// VSync
		configOption = configOptions.find("VSync")->second;
		this->vSyncCombo->addItem(MyGUI::LanguageManager::getInstancePtr()->replaceTags("#{MessageBox_Yes}"));
		this->vSyncCombo->addItem(MyGUI::LanguageManager::getInstancePtr()->replaceTags("#{MessageBox_No}"));
		if ("Yes" == configOption.currentValue || "1" == configOption.currentValue)
		{
			index = 0;
		}
		else
		{
			index = 1;
		}

		this->vSyncCombo->setIndexSelected(index);
		this->strOldVSync = this->vSyncCombo->getItemNameAt(index);

		// Resolution
		configOption = configOptions.find("Video Mode")->second;
		possibleOptions = configOption.possibleValues;
		for (unsigned int i = 0; i < possibleOptions.size(); i++)
		{
			this->resolutionCombo->addItem(possibleOptions.at(i));
		}
		this->resolutionCombo->setCaption(configOption.currentValue);
		index = this->resolutionCombo->findItemIndexWith(configOption.currentValue);
		this->resolutionCombo->setIndexSelected(index);
		this->strOldResolution = this->resolutionCombo->getItemNameAt(index);

		// Quality
		graphicsQualityCombo->addItem(MyGUI::LanguageManager::getInstancePtr()->replaceTags("#{Low}"));
		graphicsQualityCombo->addItem(MyGUI::LanguageManager::getInstancePtr()->replaceTags("#{Middle}"));
		graphicsQualityCombo->addItem(MyGUI::LanguageManager::getInstancePtr()->replaceTags("#{High}"));

		index = Core::getSingletonPtr()->getOptionQualityLevel();
		this->graphicsQualityCombo->setIndexSelected(index);
		this->strOldQuality = this->graphicsQualityCombo->getItemNameAt(index);

		// Language
		this->languageCombo->addItem(MyGUI::LanguageManager::getInstancePtr()->replaceTags("#{English}"));
		this->languageCombo->addItem(MyGUI::LanguageManager::getInstancePtr()->replaceTags("#{German}"));

		index = Core::getSingletonPtr()->getOptionLanguage();
		this->languageCombo->setIndexSelected(index);
		this->strOldLanguage = this->languageCombo->getItemNameAt(index);
	}

	void MainConfigurationStateMyGui::itemSelected(MyGUI::ComboBox* _sender, size_t _index)
	{
		// Check if an item selection has been changed, that required a new start of the application
		bool restart = false;
		MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("messageLabel")->setCaption("");

		if (_sender == this->renderSystemCombo)
		{
			if (this->strOldRenderSystem != Ogre::String(this->renderSystemCombo->getItemNameAt(this->renderSystemCombo->getIndexSelected())))
			{
				restart = true;
			}
		}
		else if (_sender == this->antiAliasingCombo)
		{
			if (this->strOldFSAA != Ogre::String(this->antiAliasingCombo->getItemNameAt(this->antiAliasingCombo->getIndexSelected())))
			{
				restart = true;
			}
		}
		else if (_sender == this->fullscreenCombo)
		{
			if (this->strOldFullscreen != Ogre::String(this->fullscreenCombo->getItemNameAt(this->fullscreenCombo->getIndexSelected())))
			{
				restart = true;
			}
		}
		else if (_sender == this->resolutionCombo)
		{
			if (this->strOldResolution != Ogre::String(this->resolutionCombo->getItemNameAt(this->resolutionCombo->getIndexSelected())))
			{
				restart = true;
			}
		}
		else if (_sender == this->languageCombo)
		{
			if (this->strOldLanguage != Ogre::String(this->languageCombo->getItemNameAt(this->languageCombo->getIndexSelected())))
			{
				restart = true;
			}
		}

		if (true == restart)
		{
			MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("messageLabel")->setCaptionWithReplacing("#{Restart}");
		}
	}

	void MainConfigurationStateMyGui::buttonHit(MyGUI::Widget* _sender)
	{
		if ("clientEnterButton" == _sender->getName())
		{
			if (!Core::getSingletonPtr()->isStartedAsServer())
			{
				this->menuMusic->pause();
			}
			this->changeAppState(this->findByName("ClientConfigurationStateMyGui"));
			// this->changeAppState(this->findByName(this->nextAppStateName));
		}
		else if ("serverEnterButton" == _sender->getName())
		{
			this->changeAppState(this->findByName("ServerConfigurationStateMyGui"));
		}
		else if ("optionsButton" == _sender->getName())
		{
			MyGUI::Gui::getInstance().findWidget<MyGUI::Window>("menueStatePanel")->setVisible(false);
			MyGUI::Gui::getInstance().findWidget<MyGUI::Window>("configurationPanel")->setVisible(true);
		}
		else if ("quitButton" == _sender->getName())
		{
			MyGUI::Message* messageBox = MyGUI::Message::createMessageBox("Menue", MyGUI::LanguageManager::getInstancePtr()->replaceTags("#{Quit_Application}"),
				MyGUI::MessageBoxStyle::IconWarning | MyGUI::MessageBoxStyle::Yes | MyGUI::MessageBoxStyle::No, "Popup", true);

			messageBox->eventMessageBoxResult += MyGUI::newDelegate(this, &MainConfigurationStateMyGui::notifyMessageBoxEnd);
		}
		else if ("applyButton" == _sender->getName())
		{
			this->applyOptions();
			MyGUI::Gui::getInstance().findWidget<MyGUI::Window>("menueStatePanel")->setVisible(true);
			MyGUI::Gui::getInstance().findWidget<MyGUI::Window>("configurationPanel")->setVisible(false);
		}
		else if ("abordButton" == _sender->getName())
		{
			MyGUI::Gui::getInstance().findWidget<MyGUI::Window>("menueStatePanel")->setVisible(true);
			MyGUI::Gui::getInstance().findWidget<MyGUI::Window>("configurationPanel")->setVisible(false);

			// Rest unsaved values to old ones
			this->renderSystemCombo->setCaptionWithReplacing(this->strOldRenderSystem);
			this->antiAliasingCombo->setCaptionWithReplacing(this->strOldFSAA);
			this->fullscreenCombo->setCaptionWithReplacing(this->strOldFullscreen);
			this->vSyncCombo->setCaptionWithReplacing(this->strOldVSync);
			this->resolutionCombo->setCaptionWithReplacing(this->strOldResolution);
			this->graphicsQualityCombo->setCaptionWithReplacing(this->strOldQuality);
			this->languageCombo->setCaptionWithReplacing(this->strOldLanguage);
			for (unsigned short i = 0; i < this->keyConfigTextboxes.size(); i++)
			{
				this->keyConfigTextboxes[i]->setCaptionWithReplacing(this->oldKeyValue[i]);
				this->textboxActive[i] = false;
			}
			for (unsigned short i = 0; i < this->buttonConfigTextboxes.size(); i++)
			{
				this->buttonConfigTextboxes[i]->setCaptionWithReplacing(this->oldButtonValue[i]);
				this->textboxActive[i] = false;
			}
		}
	}

	void  MainConfigurationStateMyGui::notifyMouseSetFocus(MyGUI::Widget* _sender, MyGUI::Widget* _old)
	{
		// Does not work with this combination
		// if (NOWA::Core::getSingletonPtr()->getMouse()->getMouseState().buttonDown(OIS::MB_Left))
		{
			// Highlight current edit box for key-mapping
			for (unsigned short i = 0; i < this->keyConfigTextboxes.size(); i++)
			{
				this->keyConfigTextboxes[i]->setTextShadow(false);
				this->textboxActive[i] = false;
				if (_sender == this->keyConfigTextboxes[i])
				{
					this->textboxActive[i] = true;
					this->keyConfigTextboxes[i]->setTextShadow(true);
				}
			}
		}
		/*
		// does not work if cursor is editcursor in edit box, only works on border of editbox, silly shit
		MyGUI::Widget* focus = MyGUI::InputManager::getInstance().getMouseFocusWidget();
		MyGUI::Widget* widget = MyGUI::LayerManager::getInstance().getWidgetFromPoint(positionX, positionY);
		*/
	}

	void  MainConfigurationStateMyGui::notifyMouseSetFocusJoyStick(MyGUI::Widget* _sender, MyGUI::Widget* _old)
	{
		// Does not work with this combination
		// if (NOWA::Core::getSingletonPtr()->getMouse()->getMouseState().buttonDown(OIS::MB_Left))
		{
			// Highlight current edit box for key-mapping
			for (unsigned short i = 0; i < this->buttonConfigTextboxes.size(); i++)
			{
				this->buttonConfigTextboxes[i]->setTextShadow(false);
				this->textboxActive[i + this->buttonConfigTextboxes.size()] = false;
				if (_sender == this->buttonConfigTextboxes[i])
				{
					this->textboxActive[i + this->buttonConfigTextboxes.size()] = true;
					this->buttonConfigTextboxes[i]->setTextShadow(true);
				}
			}
		}
		/*
		// does not work if cursor is editcursor in edit box, only works on border of editbox, silly shit
		MyGUI::Widget* focus = MyGUI::InputManager::getInstance().getMouseFocusWidget();
		MyGUI::Widget* widget = MyGUI::LayerManager::getInstance().getWidgetFromPoint(positionX, positionY);
		*/
	}

	void MainConfigurationStateMyGui::sliderMoved(MyGUI::ScrollBar* _sender, size_t _position)
	{
		if (this->menuMusic && _sender == this->musicSlider)
		{
			this->menuMusic->setGain(musicSlider->getScrollPosition() / 100.0f);
			MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("musicLabel")->setCaptionWithReplacing("#{Music_Volume}: ("
				+ Ogre::StringConverter::toString(musicSlider->getScrollPosition()) + " %)");
		}
		else if (_sender == this->soundSlider)
		{
			MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("soundLabel")->setCaptionWithReplacing("#{Sound_Volume}: ("
				+ Ogre::StringConverter::toString(soundSlider->getScrollPosition()) + " %)");
		}
	}

	void MainConfigurationStateMyGui::createScene()
	{

	}

	bool MainConfigurationStateMyGui::keyPressed(const OIS::KeyEvent &keyEventRef)
	{
		NOWA::Core::getSingletonPtr()->keyPressed(keyEventRef);
		if (InputDeviceCore::getSingletonPtr()->getKeyboard()->isKeyDown(OIS::KC_ESCAPE))
		{
			this->bQuit = true;
			return true;
		}

		// Check if an editbox is active and set the pressed key to the edit box for key-mapping
		bool keepMappingActive = false;
		bool alreadyExisting = false;
		short index = -1;
		Ogre::String strKeyCode;
		for (unsigned short i = 0; i < this->keyConfigTextboxes.size(); i++)
		{
			if (true == this->textboxActive[i])
			{
				index = i;
				// this->oldKeyValue[i] = this->keyConfigTextboxes[i]->getCaption();
				// get key string and set the text
				strKeyCode = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getStringFromMappedKey(keyEventRef.key);
				this->textboxActive[i] = false;
				this->keyConfigTextboxes[i]->setTextShadow(false);
				keepMappingActive = true;
			}
		}

		index = -1;
		Ogre::String strButton;
		for (unsigned short i = 0; i < this->buttonConfigTextboxes.size(); i++)
		{
			if (true == this->textboxActive[i + this->buttonConfigTextboxes.size()])
			{
				index = i;
				// this->oldKeyValue[i] = this->buttonConfigTextboxes[i]->getCaption();
				// get key string and set the text
				strButton = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getStringFromMappedButton(NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getPressedButton());
				this->textboxActive[i + this->buttonConfigTextboxes.size()] = false;
				this->buttonConfigTextboxes[i]->setTextShadow(false);
				keepMappingActive = true;
			}
		}

		if (InputDeviceCore::getSingletonPtr()->getKeyboard()->isKeyDown(OIS::KC_RETURN) && false == keepMappingActive)
		{
			// start next state
			this->changeAppState(this->findByName(this->nextAppStateName));
			return true;
		}

		// Check if the key does not exist already
		for (unsigned short i = 0; i < this->keyConfigTextboxes.size(); i++)
		{
			if (this->keyConfigTextboxes[i]->getCaption() == strKeyCode)
			{
				alreadyExisting = true;
				break;
			}
		}

		for (unsigned short i = 0; i < this->buttonConfigTextboxes.size(); i++)
		{
			if (this->buttonConfigTextboxes[i]->getCaption() == strButton)
			{
				alreadyExisting = true;
				break;
			}
		}

		// Set the new button
		if (-1 != index && !alreadyExisting && !strButton.empty())
		{
			this->buttonConfigTextboxes[index]->setCaptionWithReplacing(strButton);
			NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->remapButton(static_cast<InputDeviceModule::Action>(index), NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getPressedButton());
		}
		return true;
	}

	bool MainConfigurationStateMyGui::keyReleased(const OIS::KeyEvent &keyEventRef)
	{
		NOWA::Core::getSingletonPtr()->keyReleased(keyEventRef);
		return true;
	}

	bool MainConfigurationStateMyGui::mouseMoved(const OIS::MouseEvent &evt)
	{
		NOWA::Core::getSingletonPtr()->mouseMoved(evt);
		return true;
	}

	bool MainConfigurationStateMyGui::mousePressed(const OIS::MouseEvent &evt, OIS::MouseButtonID id)
	{
		NOWA::Core::getSingletonPtr()->mousePressed(evt, id);
		return true;
	}

	bool MainConfigurationStateMyGui::mouseReleased(const OIS::MouseEvent &evt, OIS::MouseButtonID id)
	{
		NOWA::Core::getSingletonPtr()->mouseReleased(evt, id);
		return true;
	}

	bool MainConfigurationStateMyGui::axisMoved(const OIS::JoyStickEvent& evt, int axis)
	{
		return true;
	}

	bool MainConfigurationStateMyGui::buttonPressed(const OIS::JoyStickEvent& evt, int button)
	{
		return true;
	}

	bool MainConfigurationStateMyGui::buttonReleased(const OIS::JoyStickEvent& evt, int button)
	{
		return true;
	}

	void MainConfigurationStateMyGui::update(Ogre::Real dt)
	{
		if (this->bQuit)
		{
			this->shutdown();
		}
	}

	void MainConfigurationStateMyGui::notifyMessageBoxEnd(MyGUI::Message* _sender, MyGUI::MessageBoxStyle _result)
	{
		if (_result == MyGUI::MessageBoxStyle::Yes)
		{
			shutdown();
		}
	}

	void MainConfigurationStateMyGui::applyOptions(void)
	{
		Ogre::String renderSystemSelected = Ogre::String(this->renderSystemCombo->getItemNameAt(this->renderSystemCombo->getIndexSelected()));
		Ogre::String antiAliasingSelected = Ogre::String(this->antiAliasingCombo->getItemNameAt(this->antiAliasingCombo->getIndexSelected()));
		Ogre::String fullScreenSelected = Ogre::String(this->fullscreenCombo->getItemNameAt(this->fullscreenCombo->getIndexSelected()));
		Ogre::String vSyncSelected = Ogre::String(this->vSyncCombo->getItemNameAt(this->vSyncCombo->getIndexSelected()));
		Ogre::String resolutionSelected = Ogre::String(this->resolutionCombo->getItemNameAt(this->resolutionCombo->getIndexSelected()));
		Ogre::String qualitySelected = Ogre::String(this->graphicsQualityCombo->getItemNameAt(this->graphicsQualityCombo->getIndexSelected()));
		Ogre::String languageSelected = Ogre::String(this->languageCombo->getItemNameAt(this->languageCombo->getIndexSelected()));

		if (this->strOldRenderSystem != renderSystemSelected)
		{
			this->bQuit = true;
		}
		if (this->strOldFSAA != antiAliasingSelected)
		{
			this->bQuit = true;
		}
		if (this->strOldFullscreen != fullScreenSelected)
		{
			this->bQuit = true;
		}
		if (this->strOldResolution != resolutionSelected)
		{
			this->bQuit = true;
		}
		if (this->strOldLanguage != languageSelected)
		{
			Core::getSingletonPtr()->setOptionLanguage(static_cast<unsigned int>(this->languageCombo->getIndexSelected()));
			this->bQuit = true;
		}

		if (this->strOldVSync != vSyncSelected)
		{
			bool vSync = false;
			if (1 == this->vSyncCombo->getIndexSelected())
			{
				vSync = true;
			}
			// Not supported anymore
			// Core::getSingletonPtr()->getOgreRoot()->getRenderSystem()->setWaitForVerticalBlank(vSync);
			Ogre::ConfigOptionMap& cfgOpts = Ogre::Root::getSingletonPtr()->getRenderSystem()->getConfigOptions();
			Core::getSingletonPtr()->getOgreRenderWindow()->setVSync(vSync, Ogre::StringConverter::parseUnsignedInt(cfgOpts["VSync"].currentValue));
		}

		if (this->strOldQuality != Ogre::String(this->graphicsQualityCombo->getItemNameAt(this->graphicsQualityCombo->getIndexSelected())))
		{
			if (0 == this->graphicsQualityCombo->getIndexSelected())
			{
				Core::getSingletonPtr()->setOptionLODBias(0.5f);
				Core::getSingletonPtr()->setOptionTextureFiltering(1);
				Core::getSingletonPtr()->setOptionAnisotropyLevel(4);
			}
			else if (1 == this->graphicsQualityCombo->getIndexSelected())
			{
				Core::getSingletonPtr()->setOptionLODBias(1);
				Core::getSingletonPtr()->setOptionTextureFiltering(2);
				Core::getSingletonPtr()->setOptionAnisotropyLevel(8);
				// Here rts shadows quality if available
			}
			else if (2 == this->graphicsQualityCombo->getIndexSelected())
			{
				Core::getSingletonPtr()->setOptionLODBias(1.5f);
				Core::getSingletonPtr()->setOptionTextureFiltering(3);
				Core::getSingletonPtr()->setOptionAnisotropyLevel(16);
			}
			Core::getSingletonPtr()->setOptionQualityLevel(static_cast<int>(this->graphicsQualityCombo->getIndexSelected()));
		}

		// Ogre::TextureManager::getSingleton().setDefaultNumMipmaps(5);
		// Ogre::MaterialManager::getSingleton().setDefaultTextureFiltering(static_cast<Ogre::TextureFilterOptions>(Core::getSingletonPtr()->getOptionTextureFiltering()));
		// Ogre::MaterialManager::getSingleton().setDefaultAnisotropy(Core::getSingletonPtr()->getOptionAnisotropyLevel());

		OgreALModule::getInstance()->setupVolumes(static_cast<int>(this->soundSlider->getScrollPosition()), static_cast<int>(this->musicSlider->getScrollPosition()));

		Core::getSingletonPtr()->setOptionSoundVolume(static_cast<int>(this->soundSlider->getScrollPosition()));
		Core::getSingletonPtr()->setOptionMusicVolume(static_cast<int>(this->musicSlider->getScrollPosition()));

		this->strOldRenderSystem = renderSystemSelected;
		this->strOldFSAA = antiAliasingSelected;
		this->strOldFullscreen = fullScreenSelected;
		this->strOldVSync = vSyncSelected;
		this->strOldResolution = resolutionSelected;
		this->strOldQuality = qualitySelected;
		this->strOldLanguage = languageSelected;

		for (unsigned short i = 0; i < this->keyConfigTextboxes.size(); i++)
		{
			this->oldKeyValue[i] = this->keyConfigTextboxes[i]->getCaption();
			this->textboxActive[i] = false;
		}
		for (unsigned short i = 0; i < this->buttonConfigTextboxes.size(); i++)
		{
			this->oldButtonValue[i] = this->buttonConfigTextboxes[i]->getCaption();
			this->textboxActive[i] = false;
		}

		Ogre::RenderSystem* renderSystem = Core::getSingletonPtr()->getOgreRoot()->getRenderSystemByName(renderSystemSelected);
		renderSystem->setConfigOption("FSAA", antiAliasingSelected);
		renderSystem->setConfigOption("Full Screen", fullScreenSelected);
		renderSystem->setConfigOption("VSync", vSyncSelected);
		renderSystem->setConfigOption("Video Mode", resolutionSelected);

		Core::getSingletonPtr()->getOgreRoot()->saveConfig();
		/*if (true == MainConfigurationStateMyGui::bShowCameraSettings)
		{
		Core::getSingletonPtr()->setOptionViewRange(this->viewRangeSlider->getValue());
		}*/

		// save options in XML
		Core::getSingletonPtr()->saveCustomConfiguration();
	}

}; // Namespace end
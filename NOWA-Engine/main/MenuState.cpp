#include "main/MenuState.h"
#include "main/Core.h"
#include "main/WiiManager.h"
#include "utilities/Fader.h"
#include "main/KeyConfigurator.h"

namespace NOWA
{

	bool MenuState::bShowCameraSettings = true;

	MenuState::MenuState()
		: AppState()
	{
		//Achtung: Der Konstruktor wird nur beim ersten Mal aufgerufen!!
		//Hier nichts initialisieren!
	}

	void MenuState::enter(void)
	{
		this->menuMusic = nullptr;
		//this->bQuit = false;
		FaderPlugin::getSingletonPtr()->startFadeIn(3.5f);
		Ogre::LogManager::getSingletonPtr()->logMessage("Entering MenuState...");

		//SceneManager erstellen
		this->sceneManager = Core::getSingletonPtr()->getOgreRoot()->createSceneManager(Ogre::ST_GENERIC, "MenuSceneManager");
		this->sceneManager->setAmbientLight(Ogre::ColourValue(0.7f, 0.7f, 0.7f));
		this->sceneManager->addRenderQueueListener(Core::getSingletonPtr()->getOverlaySystem());

		//Kamera erstellen
		this->camera = this->sceneManager->createCamera("MenuCamera");
		this->camera->setPosition(Ogre::Vector3(0, 25, -50));
		this->camera->lookAt(Ogre::Vector3(0, 0, 0));
		this->camera->setNearClipDistance(1);

		//Bildschirmverhaeltnis angeben
		//this->camera->setAspectRatio(Ogre::Real(Core::getSingletonPtr()->getOgreViewport()->getActualWidth()) /
			//Ogre::Real(Core::getSingletonPtr()->getOgreViewport()->getActualHeight()));
		this->camera->setAutoAspectRatio(true);

		Core::getSingletonPtr()->getOgreViewport()->setCamera(this->camera);

		this->setupWidgets();

		if (Core::getSingletonPtr()->isWiiAvailable())
		{
			Core::getSingletonPtr()->getWiiManager()->setConfiguration(this->wiiIRPosMenu->getSelectionIndex(), this->wiiAspectRatioMenu->getSelectionIndex());
			//Nutzung des Wii-Controllers?
			this->wiiUseCheckBox->setChecked(Core::getSingletonPtr()->getOptionWiiUse());
			//Die Infrarotkamera der Wii-Soll direkt (ohne das Festhalten einer Taste) benutzbar sein
			Core::getSingletonPtr()->getWiiManager()->useInfrateCameraDirectly(true);
		}

		this->createScene();
		this->createBackgroundMusic();
	}

	//Kamera, GUI und den SceneManager zerstoeren
	void MenuState::exit()
	{
		FaderPlugin::getSingletonPtr()->startFadeOut();
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MenuState] Leaving...");

		Core::getSingletonPtr()->setOptionSoundVolume(static_cast<int>(this->soundVolumeSlider->getValue()));
		Core::getSingletonPtr()->setOptionMusicVolume(static_cast<int>(this->musicVolumeSlider->getValue()));

		this->sceneManager->removeRenderQueueListener(Core::getSingletonPtr()->getOverlaySystem());
		Core::getSingletonPtr()->deleteGui();

		OgreALModule::getInstance()->deleteSound(this->menuMusic);
		//Wii Infrarotkamera soll nicht mehr direkt benutzbar sein
		Core::getSingletonPtr()->getWiiManager()->useInfrateCameraDirectly(false);

		Core::getSingletonPtr()->destroyScene(this->sceneManager);
	}

	void MenuState::showCameraSettings(bool bShow)
	{
		MenuState::bShowCameraSettings = bShow;
	}

	void MenuState::createBackgroundMusic(void)
	{
		this->menuMusic = OgreALModule::getInstance()->createMusic("MenuMusic1", "Lines Of Code.ogg", true, true);
		this->menuMusic->play();
	}

	void MenuState::setupWidgets(void)
	{
		// create GUI
		Core::getSingletonPtr()->getTrayManager()->destroyAllWidgets();
		Core::getSingletonPtr()->getTrayManager()->showCursor();
		Core::getSingletonPtr()->getTrayManager()->createLabel(OgreBites::TL_TOP, "MenuLabel", "Menue", 250);

		// main buttons
		this->enterButton = Core::getSingletonPtr()->getTrayManager()->createButton(OgreBites::TL_CENTER, "EnterButton", "Start Simulation", 250);
		this->optionsButton = Core::getSingletonPtr()->getTrayManager()->createButton(OgreBites::TL_CENTER, "OptionsButton", "Options", 250);
		this->exitButton = Core::getSingletonPtr()->getTrayManager()->createButton(OgreBites::TL_CENTER, "ExitButton", "Exit Application", 250);

		// options widgets (childs of options button)
		this->configurationButton = Core::getSingletonPtr()->getTrayManager()->createButton(OgreBites::TL_NONE, "ConfigurationButton", "Main-Configuration", 200);
		this->configurationButton->hide();
		this->wiiConfigurationButton = Core::getSingletonPtr()->getTrayManager()->createButton(OgreBites::TL_NONE, "WiiConfigurationButton", "Wii-Configuration", 200);
		this->wiiConfigurationButton->hide();
		this->audioConfigurationButton = Core::getSingletonPtr()->getTrayManager()->createButton(OgreBites::TL_NONE, "AudioConfigurationButton", "Audio-Configuration", 200);
		this->audioConfigurationButton->hide();
		this->keyConfigurationButton = Core::getSingletonPtr()->getTrayManager()->createButton(OgreBites::TL_NONE, "KeyConfigurationButton", "Key-Configuration", 200);
		this->keyConfigurationButton->hide();

		// main configuration widgets
		this->rendererMenu = Core::getSingletonPtr()->getTrayManager()->createLongSelectMenu(OgreBites::TL_NONE, "RendererMenue", "Rendersystem", 450, 240, 10);
		this->rendererMenu->hide();
		this->configSeparator = Core::getSingletonPtr()->getTrayManager()->createSeparator(OgreBites::TL_NONE, "ConfigSeparator");
		this->configSeparator->hide();
		this->fSAAMenu = Core::getSingletonPtr()->getTrayManager()->createLongSelectMenu(OgreBites::TL_NONE, "FSAAMenu", "Anti-Aliasing", 450, 240, 10);
		this->fSAAMenu->hide();
		this->fullscreenMenu = Core::getSingletonPtr()->getTrayManager()->createLongSelectMenu(OgreBites::TL_NONE, "FullscreenMenu", "Fullscreen", 450, 240, 10);
		this->fullscreenMenu->hide();
		this->vSyncMenu = Core::getSingletonPtr()->getTrayManager()->createLongSelectMenu(OgreBites::TL_NONE, "VsyncMenu", "Vsync", 450, 240, 10);
		this->vSyncMenu->hide();
		this->videoModeMenu = Core::getSingletonPtr()->getTrayManager()->createLongSelectMenu(OgreBites::TL_NONE, "VideoModeMenu", "Resolution", 450, 240, 10);
		this->videoModeMenu->hide();
		if (true == MenuState::bShowCameraSettings)
		{
			this->viewRangeSlider = Core::getSingletonPtr()->getTrayManager()->createThickSlider(OgreBites::TL_NONE, "ViewRangeSlider", "Viewrange (meters)", 450, 70, 0, 0, 0);
			this->viewRangeSlider->setRange(10, 5000, 500);
			this->viewRangeSlider->setValue(Core::getSingletonPtr()->getOptionViewRange());
			//Event muss bei Slidern 1x aufgerufen werden, sonst gibt es einen Absturz wenn Sliderfunktion verwendet wird
			///this->sliderMoved(this->viewRangeSlider); 
			this->viewRangeSlider->hide();
		}
		this->lODBiasSlider = Core::getSingletonPtr()->getTrayManager()->createThickSlider(OgreBites::TL_NONE, "LODBiasSlider", "LODBias", 450, 70, 0, 0, 0);
		this->lODBiasSlider->setRange(1, 2, 3);
		this->lODBiasSlider->setValue(Core::getSingletonPtr()->getOptionLODBias());
		this->lODBiasSlider->hide();
		this->textureFilteringMenu = Core::getSingletonPtr()->getTrayManager()->createLongSelectMenu(OgreBites::TL_NONE, "TextureFilteringMenu", "Texture-Filtering", 450, 240, 10);
		this->textureFilteringMenu->hide();
		this->anisotropySlider = Core::getSingletonPtr()->getTrayManager()->createThickSlider(OgreBites::TL_NONE, "AnisotropySlider", "Anisotropy-Level", 450, 70, 0, 0, 0);
		this->anisotropySlider->setRange(1, 16, 16);
		this->anisotropySlider->setValue(Core::getSingletonPtr()->getOptionAnisotropyLevel());
		this->anisotropySlider->hide();
		this->infoLabel = Core::getSingletonPtr()->getTrayManager()->createLabel(OgreBites::TL_NONE, "InfoLabel", "", 450);
		this->infoLabel->hide();
		this->optionsApplyButton = Core::getSingletonPtr()->getTrayManager()->createButton(OgreBites::TL_NONE, "OptionsApplyButton", "Apply", 200);
		this->optionsApplyButton->hide();
		this->optionsAbordButton = Core::getSingletonPtr()->getTrayManager()->createButton(OgreBites::TL_NONE, "OptionsAbordButton", "Abord", 200);
		this->optionsAbordButton->hide();

		// Wii-configuration widgets
		this->wiiInfoLabel = Core::getSingletonPtr()->getTrayManager()->createLabel(OgreBites::TL_NONE, "WiiInfoLabel", "", 450);
		this->wiiInfoLabel->hide();
		this->wiiUseCheckBox = Core::getSingletonPtr()->getTrayManager()->createCheckBox(OgreBites::TL_NONE, "wiiUseCheckBox", "Use Wii-Controller?", 450);
		this->wiiUseCheckBox->hide();
		this->wiiBatteryLevelLabel = Core::getSingletonPtr()->getTrayManager()->createLabel(OgreBites::TL_NONE, "WiiBatteryLevelLabel", "", 450);
		this->wiiBatteryLevelLabel->hide();
		this->wiiIRPosMenu = Core::getSingletonPtr()->getTrayManager()->createLongSelectMenu(OgreBites::TL_NONE, "WiiIRPos", "Position Ultrared Bar", 450, 240, 10);
		this->wiiIRPosMenu->hide();
		this->wiiAspectRatioMenu = Core::getSingletonPtr()->getTrayManager()->createLongSelectMenu(OgreBites::TL_NONE, "WiiAspectRatio", "Aspect Ratio", 450, 240, 10);
		this->wiiAspectRatioMenu->hide();

		// Audio-configuration widgets
		this->soundVolumeSlider = Core::getSingletonPtr()->getTrayManager()->createThickSlider(OgreBites::TL_NONE, "SoundVolumeSlider", "Sound Volume", 450, 70, 0, 0, 0);
		this->soundVolumeSlider->setRange(1, 100, 100);
		this->soundVolumeSlider->setValue((Ogre::Real)Core::getSingletonPtr()->getOptionSoundVolume());
		this->soundVolumeSlider->hide();
		this->musicVolumeSlider = Core::getSingletonPtr()->getTrayManager()->createThickSlider(OgreBites::TL_NONE, "MusicVolumeSlider", "Music Volume", 450, 70, 0, 0, 0);
		this->musicVolumeSlider->setRange(1, 100, 100);
		this->musicVolumeSlider->setValue((Ogre::Real)Core::getSingletonPtr()->getOptionMusicVolume());
		this->musicVolumeSlider->hide();
		//Event muss bei Slidern 1x aufgerufen werden, sonst gibt es einen Absturz wenn Sliderfunktion verwendet wird
		this->sliderMoved(this->musicVolumeSlider);

		// key-configuration widgets
		this->keyConfigTextboxes.resize(7);
		this->textboxActive.resize(7, false);

		this->keyConfigTextboxes[0] = Core::getSingletonPtr()->getTrayManager()->createTextBox(OgreBites::TL_NONE, "KeyUp", "Move up", 450, 80);
		this->keyConfigTextboxes[1] = Core::getSingletonPtr()->getTrayManager()->createTextBox(OgreBites::TL_NONE, "KeyDown", "Move down (crouch)", 450, 80);
		this->keyConfigTextboxes[2] = Core::getSingletonPtr()->getTrayManager()->createTextBox(OgreBites::TL_NONE, "KeyLeft", "Move left", 450, 80);
		this->keyConfigTextboxes[3] = Core::getSingletonPtr()->getTrayManager()->createTextBox(OgreBites::TL_NONE, "KeyRight", "Move right", 450, 80);
		this->keyConfigTextboxes[4] = Core::getSingletonPtr()->getTrayManager()->createTextBox(OgreBites::TL_NONE, "KeySpace", "Jump", 450, 80);
		this->keyConfigTextboxes[5] = Core::getSingletonPtr()->getTrayManager()->createTextBox(OgreBites::TL_NONE, "KeyAction1", "Action 1", 450, 80);
		this->keyConfigTextboxes[6] = Core::getSingletonPtr()->getTrayManager()->createTextBox(OgreBites::TL_NONE, "KeyAction2", "Action 2", 450, 80);
		
		for (unsigned short i = 0; i < keyConfigTextboxes.size(); i++)
		{
			auto keyCode = KeyConfigurator::getInstance()->getKey(static_cast<KeyboardAction>(i));
			Ogre::String strKeyCode = KeyConfigurator::getInstance()->getKeyString(keyCode);
			// Ogre::String strKeyCode = NOWA::Core::getSingletonPtr()->getKeyboard()->getAsString(keyCode);

			this->keyConfigTextboxes[i]->setText(strKeyCode);
			this->keyConfigTextboxes[i]->hide();

			/*this->keyConfigTextboxes[i] = Core::getSingletonPtr()->getTrayManager()->createTextBox(OgreBites::TL_NONE,
				"KeyConfigTextbox" + Ogre::StringConverter::toString(i), strKeyCode, 380, 80);*/
			
		}
		
		// fill with data
		this->populateOptions();
	}

	void MenuState::populateOptions(void)
	{
		// load rendersystems to menue
		Ogre::StringVector rsNames;
		Ogre::RenderSystemList rsList = Core::getSingletonPtr()->getOgreRoot()->getAvailableRenderers();
		for (unsigned int i = 0; i < rsList.size(); i++)
		{
			rsNames.push_back(rsList[i]->getName());
		}
		this->rendererMenu->setItems(rsNames);
		this->strOldRenderSystem = this->rendererMenu->getSelectedItem();

		Ogre::ConfigOptionMap& configOptions = Core::getSingletonPtr()->getOgreRoot()->getRenderSystemByName(this->rendererMenu->getSelectedItem())->getConfigOptions();
		Ogre::ConfigOption configOption;
		Ogre::StringVector strOptionNames;
		Ogre::StringVector possibleOptions;

		//Antialiasingoptionen
		configOption = configOptions.find("FSAA")->second;
		possibleOptions = configOption.possibleValues;
		for (unsigned int i = 0; i < possibleOptions.size(); i++)
		{
			strOptionNames.push_back(possibleOptions.at(i));
		}
		this->fSAAMenu->setItems(strOptionNames);
		this->fSAAMenu->selectItem(configOption.currentValue);
		this->strOldFSAA = this->fSAAMenu->getSelectedItem();
		strOptionNames.clear();

		//Vollbildoptionen
		configOption = configOptions.find("Full Screen")->second;
		possibleOptions = configOption.possibleValues;
		for (unsigned int i = 0; i < possibleOptions.size(); i++)
		{
			strOptionNames.push_back(possibleOptions.at(i));
		}
		this->fullscreenMenu->setItems(strOptionNames);
		this->fullscreenMenu->selectItem(configOption.currentValue);
		this->strOldFullscreen = this->fullscreenMenu->getSelectedItem();
		strOptionNames.clear();

		//Vertikale Synchronisation Optionen
		configOption = configOptions.find("VSync")->second;
		possibleOptions = configOption.possibleValues;
		for (unsigned int i = 0; i < possibleOptions.size(); i++)
		{
			strOptionNames.push_back(possibleOptions.at(i));
		}
		this->vSyncMenu->setItems(strOptionNames);
		this->vSyncMenu->selectItem(configOption.currentValue);
		this->strOldVSync = this->vSyncMenu->getSelectedItem();
		strOptionNames.clear();

		//Aufloesungsoptionen
		configOption = configOptions.find("Video Mode")->second;
		possibleOptions = configOption.possibleValues;
		for (unsigned int i = 0; i < possibleOptions.size(); i++)
		{
			strOptionNames.push_back(possibleOptions.at(i));
		}
		this->videoModeMenu->setItems(strOptionNames);
		this->videoModeMenu->selectItem(configOption.currentValue);
		this->strOldVideoMode = this->videoModeMenu->getSelectedItem();
		strOptionNames.clear();

		//Textur-Filterung Optionen
		strOptionNames.push_back("None");
		strOptionNames.push_back("Bilinear");
		strOptionNames.push_back("Trilinear");
		strOptionNames.push_back("Anisotropic");
		this->textureFilteringMenu->setItems(strOptionNames);
		this->textureFilteringMenu->selectItem(Core::getSingletonPtr()->getOptionTextureFiltering());
		strOptionNames.clear();

		//Position Wii-Infrarotleiste
		strOptionNames.push_back("Above the Screen");
		strOptionNames.push_back("Below the Screen");
		this->wiiIRPosMenu->setItems(strOptionNames);
		this->wiiIRPosMenu->selectItem(Core::getSingletonPtr()->getOptionWiiIRPos());
		strOptionNames.clear();

		//Bildschirmverhaeltnis Wii
		strOptionNames.push_back("4:3");
		strOptionNames.push_back("16:9");
		this->wiiAspectRatioMenu->setItems(strOptionNames);
		this->wiiAspectRatioMenu->selectItem(Core::getSingletonPtr()->getOptionWiiAspectRatio());
		strOptionNames.clear();

		//Nutzung des Wii-Controllers?
		//Option wird nur geladen, wenn das System sich erfolgreich mit dem Wii-Controller verbinden konnte
		if (Core::getSingletonPtr()->isWiiAvailable())
		{
			this->wiiUseCheckBox->setChecked(Core::getSingletonPtr()->getOptionWiiUse());
		}
		else
		{
			Core::getSingletonPtr()->setOptionWiiUse(false);
			this->wiiUseCheckBox->setChecked(false);
		}
	}

	void MenuState::itemSelected(OgreBites::SelectMenu *pMenu)
	{
		this->infoLabel->setCaption("");
		if (pMenu == this->rendererMenu)
		{
			//Wenn ein anderes Item ausgewaehlt wurde, dann kommt eine Meldung, das die Anwendung neugestartet wird
			if (this->strOldRenderSystem != this->rendererMenu->getSelectedItem())
			{
				this->infoLabel->setCaption("Attention: Application must be restarted!");
			}
		}
		if (pMenu == this->fSAAMenu)
		{
			//Wenn ein anderes Item ausgewaehlt wurde, dann kommt eine Meldung, das die Anwendung neugestartet wird
			if (this->strOldFSAA != this->fSAAMenu->getSelectedItem())
			{
				this->infoLabel->setCaption("Attention: Application must be restarted!");
			}
		}
		else if (pMenu == this->fullscreenMenu)
		{
			if (this->strOldFullscreen != this->fullscreenMenu->getSelectedItem())
			{
				this->infoLabel->setCaption("Attention: Application must be restarted!");
			}
		}
		else if (pMenu == this->vSyncMenu)
		{
			if (this->strOldVSync != this->vSyncMenu->getSelectedItem())
			{
				this->infoLabel->setCaption("Attention: Application must be restarted!");
			}
		}
		else if (pMenu == this->videoModeMenu)
		{
			if (this->strOldVideoMode != this->videoModeMenu->getSelectedItem())
			{
				this->infoLabel->setCaption("Attention: Application must be restarted!");
			}
		}
	}

	void MenuState::buttonHit(OgreBites::Button *pButton)
	{
		if (pButton->getName() == "ExitButton")
			//Core::getSingletonPtr()->getTrayManager()->showYesNoDialog("Simulation", "Zurueck zu Windows?");
			this->bQuit = true;
		else if (pButton->getName() == "EnterButton")
		{
			//Spielzustand beginnen
			this->changeAppState(this->findByName(this->nextAppStateName));
		}
		else if (pButton->getName() == "OptionsButton")
		{
			this->switchConfiguration(1);
		}
		else if (pButton->getName() == "OptionsApplyButton")
		{
			this->applyOptions();
			this->switchConfiguration(0);
		}
		else if (pButton->getName() == "OptionsAbordButton")
		{
			this->switchConfiguration(0);
		}
		else if (pButton->getName() == "ConfigurationButton")
		{
			this->switchConfiguration(1);
		}
		else if (pButton->getName() == "WiiConfigurationButton")
		{
			this->switchConfiguration(2);
		}
		else if (pButton->getName() == "AudioConfigurationButton")
		{
			this->switchConfiguration(3);
		}
		else if (pButton->getName() == "KeyConfigurationButton")
		{
			this->switchConfiguration(4);
		}
	}

	void MenuState::applyOptions(void)
	{
		if (this->strOldRenderSystem != this->rendererMenu->getSelectedItem())
			this->bQuit = true;
		if (this->strOldFSAA != this->fSAAMenu->getSelectedItem())
			this->bQuit = true;
		if (this->strOldFullscreen != this->fullscreenMenu->getSelectedItem())
			this->bQuit = true;
		if (this->strOldVSync != this->vSyncMenu->getSelectedItem())
			this->bQuit = true;
		if (this->strOldVideoMode != this->videoModeMenu->getSelectedItem())
			this->bQuit = true;

		this->strOldRenderSystem = this->rendererMenu->getSelectedItem();
		this->strOldFSAA = this->fSAAMenu->getSelectedItem();
		this->strOldFullscreen = this->fullscreenMenu->getSelectedItem();
		this->strOldVSync = this->vSyncMenu->getSelectedItem();
		this->strOldVideoMode = this->videoModeMenu->getSelectedItem();

		Ogre::RenderSystem *pRenderSystem = Core::getSingletonPtr()->getOgreRoot()->getRenderSystemByName(this->rendererMenu->getSelectedItem());
		pRenderSystem->setConfigOption("FSAA", this->fSAAMenu->getSelectedItem());
		pRenderSystem->setConfigOption("Full Screen", this->fullscreenMenu->getSelectedItem());
		pRenderSystem->setConfigOption("VSync", this->vSyncMenu->getSelectedItem());
		pRenderSystem->setConfigOption("Video Mode", this->videoModeMenu->getSelectedItem());

		if (Core::getSingletonPtr()->isWiiAvailable())
		{
			Core::getSingletonPtr()->getWiiManager()->setConfiguration(this->wiiIRPosMenu->getSelectionIndex(), this->wiiAspectRatioMenu->getSelectionIndex());
		}

		Core::getSingletonPtr()->getOgreRoot()->saveConfig();
		if (true == MenuState::bShowCameraSettings)
		{
			Core::getSingletonPtr()->setOptionViewRange(this->viewRangeSlider->getValue());
		}
		Core::getSingletonPtr()->setOptionLODBias(this->lODBiasSlider->getValue());
		Core::getSingletonPtr()->setOptionTextureFiltering(this->textureFilteringMenu->getSelectionIndex());
		Core::getSingletonPtr()->setOptionAnisotropyLevel(this->anisotropySlider->getValue());
		Core::getSingletonPtr()->setOptionWiiIRPos(this->wiiIRPosMenu->getSelectionIndex());
		Core::getSingletonPtr()->setOptionWiiAspectRatio(this->wiiAspectRatioMenu->getSelectionIndex());
		Core::getSingletonPtr()->setOptionWiiUse(this->wiiUseCheckBox->isChecked());

		OgreALModule::getInstance()->setupVolumes(this->soundVolumeSlider->getValue(), this->musicVolumeSlider->getValue());

		Core::getSingletonPtr()->setOptionMusicVolume(this->musicVolumeSlider->getValue());
		Core::getSingletonPtr()->setOptionSoundVolume(this->soundVolumeSlider->getValue());

		// save options in XML
		Core::getSingletonPtr()->saveCustomConfiguration();
	}

	// show and hide options
	void MenuState::switchConfiguration(int state)
	{
		// main menue
		if (state == 0)
		{
			// hide all configurations
			while (Core::getSingletonPtr()->getTrayManager()->getTrayContainer(OgreBites::TL_CENTER)->isVisible())
			{
				Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(OgreBites::TL_CENTER, 0, OgreBites::TL_NONE);
			}
			this->configurationButton->hide();
			this->wiiConfigurationButton->hide();
			this->audioConfigurationButton->hide();
			this->keyConfigurationButton->hide();
			this->configSeparator->hide();
			this->rendererMenu->hide();
			this->fSAAMenu->hide();
			this->fullscreenMenu->hide();
			this->vSyncMenu->hide();
			this->videoModeMenu->hide();
			if (true == MenuState::bShowCameraSettings)
			{
				this->viewRangeSlider->hide();
			}
			this->lODBiasSlider->hide();
			this->textureFilteringMenu->hide();
			this->anisotropySlider->hide();
			this->infoLabel->hide();
			this->optionsApplyButton->hide();
			this->optionsAbordButton->hide();
			this->wiiInfoLabel->hide();
			this->wiiUseCheckBox->hide();
			this->wiiBatteryLevelLabel->hide();
			this->wiiIRPosMenu->hide();
			this->wiiAspectRatioMenu->hide();
			this->soundVolumeSlider->hide();
			this->musicVolumeSlider->hide();
			for (unsigned short i = 0; i < this->keyConfigTextboxes.size(); i++)
			{
				this->keyConfigTextboxes[i]->hide();
			}

			this->enterButton->show();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->enterButton, OgreBites::TL_CENTER);
			this->optionsButton->show();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->optionsButton, OgreBites::TL_CENTER);
			this->exitButton->show();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->exitButton, OgreBites::TL_CENTER);
			this->infoLabel->setCaption("");
		}
		// configuration menue
		else if (state == 1)
		{
			// hide main menue
			Core::getSingletonPtr()->getTrayManager()->removeWidgetFromTray(this->enterButton);
			Core::getSingletonPtr()->getTrayManager()->removeWidgetFromTray(this->optionsButton);
			Core::getSingletonPtr()->getTrayManager()->removeWidgetFromTray(this->exitButton);
			this->enterButton->hide();
			this->optionsButton->hide();
			this->exitButton->hide();

			//Konfigurationsmenue zeigen
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->configurationButton, OgreBites::TL_CENTER);
			this->configurationButton->show();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->wiiConfigurationButton, OgreBites::TL_CENTER);
			this->wiiConfigurationButton->show();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->audioConfigurationButton, OgreBites::TL_CENTER);
			this->audioConfigurationButton->show();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->keyConfigurationButton, OgreBites::TL_CENTER);
			this->keyConfigurationButton->show();

			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->configSeparator, OgreBites::TL_CENTER);
			this->configSeparator->show();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->rendererMenu, OgreBites::TL_CENTER);
			this->rendererMenu->selectItem(Core::getSingletonPtr()->getOgreRoot()->getRenderSystem()->getName());
			this->rendererMenu->show();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->fSAAMenu, OgreBites::TL_CENTER);
			this->fSAAMenu->show();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->fullscreenMenu, OgreBites::TL_CENTER);
			this->fullscreenMenu->show();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->vSyncMenu, OgreBites::TL_CENTER);
			this->vSyncMenu->show();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->videoModeMenu, OgreBites::TL_CENTER);
			this->videoModeMenu->show();
			if (true == MenuState::bShowCameraSettings)
			{
				Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->viewRangeSlider, OgreBites::TL_CENTER);
				this->viewRangeSlider->show();
			}
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->lODBiasSlider, OgreBites::TL_CENTER);
			this->lODBiasSlider->show();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->textureFilteringMenu, OgreBites::TL_CENTER);
			this->textureFilteringMenu->show();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->anisotropySlider, OgreBites::TL_CENTER);
			this->anisotropySlider->show();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->infoLabel, OgreBites::TL_CENTER);
			this->infoLabel->show();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->optionsApplyButton, OgreBites::TL_CENTER);
			this->optionsApplyButton->show();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->optionsAbordButton, OgreBites::TL_CENTER);
			this->optionsAbordButton->show();

			// hide Wii-configuration
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->wiiInfoLabel, OgreBites::TL_NONE);
			this->wiiInfoLabel->hide();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->wiiUseCheckBox, OgreBites::TL_NONE);
			this->wiiUseCheckBox->hide();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->wiiBatteryLevelLabel, OgreBites::TL_NONE);
			this->wiiBatteryLevelLabel->hide();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->wiiIRPosMenu, OgreBites::TL_NONE);
			this->wiiIRPosMenu->hide();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->wiiAspectRatioMenu, OgreBites::TL_NONE);
			this->wiiAspectRatioMenu->hide();

			// hide Audio-configuration
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->soundVolumeSlider, OgreBites::TL_NONE);
			this->soundVolumeSlider->hide();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->musicVolumeSlider, OgreBites::TL_NONE);
			this->musicVolumeSlider->hide();

			// hide key-configuration
			for (unsigned short i = 0; i < this->keyConfigTextboxes.size(); i++)
			{
				this->keyConfigTextboxes[i]->hide();
				Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->keyConfigTextboxes[i], OgreBites::TL_NONE);
			}
		}
		// Wii-configurations menue
		else if (state == 2)
		{
			// hide configurations menue
			this->configSeparator->hide();
			this->rendererMenu->hide();
			this->fSAAMenu->hide();
			this->fullscreenMenu->hide();
			this->vSyncMenu->hide();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->vSyncMenu, OgreBites::TL_NONE);
			this->videoModeMenu->hide();
			if (true == MenuState::bShowCameraSettings)
			{
				Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->viewRangeSlider, OgreBites::TL_NONE);
				this->viewRangeSlider->hide();
			}
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->lODBiasSlider, OgreBites::TL_NONE);
			this->lODBiasSlider->hide();

			// hide audio configuration
			this->soundVolumeSlider->hide();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->soundVolumeSlider, OgreBites::TL_NONE);
			this->musicVolumeSlider->hide();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->musicVolumeSlider, OgreBites::TL_NONE);

			// hide key-configuration
			for (unsigned short i = 0; i < this->keyConfigTextboxes.size(); i++)
			{
				this->keyConfigTextboxes[i]->hide();
				Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->keyConfigTextboxes[i], OgreBites::TL_NONE);
			}

			// last two objects will be replaced with the new ones, hence moveWidgetToTray
			this->textureFilteringMenu->hide();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->anisotropySlider, OgreBites::TL_NONE);
			this->anisotropySlider->hide();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->infoLabel, OgreBites::TL_NONE);
			this->infoLabel->hide();

			// show Wii-configuration
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->wiiInfoLabel, OgreBites::TL_CENTER);
			this->wiiInfoLabel->show();
			if (Core::getSingletonPtr()->isWiiAvailable())
			{
				this->wiiInfoLabel->setCaption("The Wii-controller is ready!");
			}
			else
			{
				this->wiiInfoLabel->setCaption("The Wii-controller is not ready!");
			}
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->wiiUseCheckBox, OgreBites::TL_CENTER);
			this->wiiUseCheckBox->show();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->wiiBatteryLevelLabel, OgreBites::TL_CENTER);
			this->wiiBatteryLevelLabel->show();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->wiiIRPosMenu, OgreBites::TL_CENTER);
			this->wiiIRPosMenu->show();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->wiiAspectRatioMenu, OgreBites::TL_CENTER);
			this->wiiAspectRatioMenu->show();

			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->optionsApplyButton, OgreBites::TL_CENTER);
			this->optionsApplyButton->show();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->optionsAbordButton, OgreBites::TL_CENTER);
			this->optionsAbordButton->show();
		}
		// Audio-configurations menue
		else if (state == 3)
		{
			// hide configurations menue
			this->configSeparator->hide();
			this->rendererMenu->hide();
			this->fSAAMenu->hide();
			this->fullscreenMenu->hide();
			this->vSyncMenu->hide();
			//Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->vSyncMenu, OgreBites::TL_NONE);
			this->videoModeMenu->hide();
			if (true == MenuState::bShowCameraSettings)
			{
				Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->viewRangeSlider, OgreBites::TL_NONE);
				this->viewRangeSlider->hide();
			}
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->lODBiasSlider, OgreBites::TL_NONE);
			this->lODBiasSlider->hide();

			// hide wii configuration
			this->wiiInfoLabel->hide();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->wiiInfoLabel, OgreBites::TL_NONE);
			this->wiiUseCheckBox->hide();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->wiiUseCheckBox, OgreBites::TL_NONE);
			this->wiiBatteryLevelLabel->hide();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->wiiBatteryLevelLabel, OgreBites::TL_NONE);
			this->wiiIRPosMenu->hide();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->wiiIRPosMenu, OgreBites::TL_NONE);
			this->wiiAspectRatioMenu->hide();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->wiiAspectRatioMenu, OgreBites::TL_NONE);

			// hide key-configuration
			for (unsigned short i = 0; i < this->keyConfigTextboxes.size(); i++)
			{
				this->keyConfigTextboxes[i]->hide();
				Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->keyConfigTextboxes[i], OgreBites::TL_NONE);
			}

			// last two objects will be replaced with the new ones, hence moveWidgetToTray
			this->textureFilteringMenu->hide();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->anisotropySlider, OgreBites::TL_NONE);
			this->anisotropySlider->hide();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->infoLabel, OgreBites::TL_NONE);
			this->infoLabel->hide();

			// show Audio-configurations menue
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->soundVolumeSlider, OgreBites::TL_CENTER);
			this->soundVolumeSlider->show();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->musicVolumeSlider, OgreBites::TL_CENTER);
			this->musicVolumeSlider->show();

			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->optionsApplyButton, OgreBites::TL_CENTER);
			this->optionsApplyButton->show();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->optionsAbordButton, OgreBites::TL_CENTER);
			this->optionsAbordButton->show();
		}
		// Key-configurations menue
		else if (state == 4)
		{
			// hide configurations menue
			this->configSeparator->hide();
			this->rendererMenu->hide();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->rendererMenu, OgreBites::TL_NONE);
			this->fSAAMenu->hide();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->fSAAMenu, OgreBites::TL_NONE);
			this->fullscreenMenu->hide();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->fullscreenMenu, OgreBites::TL_NONE);
			this->vSyncMenu->hide();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->vSyncMenu, OgreBites::TL_NONE);
			this->videoModeMenu->hide();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->videoModeMenu, OgreBites::TL_NONE);
			// last two objects will be replaced with the new ones, hence moveWidgetToTray
			this->textureFilteringMenu->hide();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->textureFilteringMenu, OgreBites::TL_NONE);
			this->anisotropySlider->hide();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->anisotropySlider, OgreBites::TL_NONE);
			this->infoLabel->hide();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->infoLabel, OgreBites::TL_NONE);
			this->infoLabel->hide();
			if (true == MenuState::bShowCameraSettings)
			{
				Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->viewRangeSlider, OgreBites::TL_NONE);
				this->viewRangeSlider->hide();
			}
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->lODBiasSlider, OgreBites::TL_NONE);
			this->lODBiasSlider->hide();

			// hide wii configuration
			this->wiiInfoLabel->hide();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->wiiInfoLabel, OgreBites::TL_NONE);
			this->wiiUseCheckBox->hide();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->wiiUseCheckBox, OgreBites::TL_NONE);
			this->wiiBatteryLevelLabel->hide();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->wiiBatteryLevelLabel, OgreBites::TL_NONE);
			this->wiiIRPosMenu->hide();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->wiiIRPosMenu, OgreBites::TL_NONE);
			this->wiiAspectRatioMenu->hide();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->wiiAspectRatioMenu, OgreBites::TL_NONE);

			// hide audio configuration
			this->soundVolumeSlider->hide();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->soundVolumeSlider, OgreBites::TL_NONE);
			this->musicVolumeSlider->hide();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->musicVolumeSlider, OgreBites::TL_NONE);

			// show key-configuration
			for (unsigned short i = 0; i < this->keyConfigTextboxes.size(); i++)
			{
				this->keyConfigTextboxes[i]->show();
				Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->keyConfigTextboxes[i], OgreBites::TL_CENTER);
			}

			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->optionsApplyButton, OgreBites::TL_CENTER);
			this->optionsApplyButton->show();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->optionsAbordButton, OgreBites::TL_CENTER);
			this->optionsAbordButton->show();
		}
	}

	void MenuState::sliderMoved(OgreBites::Slider *pSlider)
	{
		if (this->menuMusic && pSlider->getName() == "MusicVolumeSlider")
		{
			this->menuMusic->setGain(pSlider->getValue() / 100.0f);
		}
	}

	void MenuState::createScene()
	{
	}

	bool MenuState::keyPressed(const OIS::KeyEvent &keyEventRef)
	{
		if (Core::getSingletonPtr()->getKeyboard()->isKeyDown(OIS::KC_ESCAPE))
		{
			this->bQuit = true;
			return true;
		}
		if (Core::getSingletonPtr()->getKeyboard()->isKeyDown(OIS::KC_RETURN))
		{
			// start next state
			this->changeAppState(this->findByName(this->nextAppStateName));
			return true;
		}

		if (Core::getSingletonPtr()->getKeyboard()->isKeyDown(OIS::KC_F1))
		{
			this->wiiUseCheckBox->setChecked(false);
			Core::getSingletonPtr()->setOptionWiiUse(false);
			this->applyOptions();
			return true;
		}

		bool alreadyExisting = false;
		short index = -1;
		Ogre::String strKeyCode;
		for (unsigned short i = 0; i < this->keyConfigTextboxes.size(); i++)
		{
			if (true == this->textboxActive[i])
			{
				index = i;
				// get key string and set the text
				strKeyCode = KeyConfigurator::getInstance()->getKeyString(keyEventRef.key);
				// this->keyConfigTextboxes[i]->setText(strKeyCode);
				// remap the key
				// KeyConfigurator::getInstance()->remapKey(static_cast<KeyConfigurator::KeyboardAction>(i), keyEventRef.key);
				this->textboxActive[i] = false;
			}

		}
		for (unsigned short i = 0; i < this->keyConfigTextboxes.size(); i++)
		{
			if (this->keyConfigTextboxes[i]->getText() == strKeyCode)
			{
				alreadyExisting = true;
				break;
			}
		}

		if (-1 != index && !alreadyExisting)
		{
			this->keyConfigTextboxes[index]->setText(strKeyCode);
			KeyConfigurator::getInstance()->remapKey(static_cast<KeyboardAction>(index), keyEventRef.key);
		}

		Core::getSingletonPtr()->keyPressed(keyEventRef);
		return true;
	}

	bool MenuState::keyReleased(const OIS::KeyEvent &keyEventRef)
	{
		Core::getSingletonPtr()->keyReleased(keyEventRef);
		return true;
	}

	bool MenuState::mouseMoved(const OIS::MouseEvent &evt)
	{
		if (Core::getSingletonPtr()->getTrayManager()->injectMouseMove(evt))
			return true;
		return true;
	}

	bool MenuState::mousePressed(const OIS::MouseEvent &evt, OIS::MouseButtonID id)
	{
		for (unsigned short i = 0; i < this->keyConfigTextboxes.size(); i++)
		{
			// first reset everything
			this->textboxActive[i] = false;
			this->keyConfigTextboxes[i]->getOverlayElement()->setMaterialName("SdkTrays/TextBox");

			// then highlight
			if (this->keyConfigTextboxes[i]->isCursorOver(this->keyConfigTextboxes[i]->getOverlayElement(), Ogre::Vector2(evt.state.X.abs, evt.state.Y.abs)))
			{
				this->keyConfigTextboxes[i]->getOverlayElement()->setMaterialName("SdkTrays/Button/Over");
				this->textboxActive[i] = true;
			}
		}

		if (Core::getSingletonPtr()->getTrayManager()->injectMouseDown(evt, id))
			return true;
		return true;
	}

	bool MenuState::mouseReleased(const OIS::MouseEvent &evt, OIS::MouseButtonID id)
	{
		if (Core::getSingletonPtr()->getTrayManager()->injectMouseUp(evt, id))
			return true;
		return true;
	}

	void MenuState::update(Ogre::Real dt)
	{
		if (Core::getSingletonPtr()->isWiiAvailable() && Core::getSingletonPtr()->getOptionWiiUse())
		{
			Ogre::Real wiiX = Core::getSingletonPtr()->getWiiManager()->wiiX;
			Ogre::Real wiiY = Core::getSingletonPtr()->getWiiManager()->wiiY;

			//Maus aus der Gui erhalten
			Ogre::OverlayContainer *pCursor = Core::getSingletonPtr()->getTrayManager()->getCursorContainer();
			//Mauszeiger an Wiikoordinater der Infrarotkamera setzen
			pCursor->setPosition(wiiX, wiiY);

			this->wiiBatteryLevelLabel->setCaption("Batterieanzeige: " + Ogre::StringConverter::toString(Core::getSingletonPtr()->getWiiManager()->getBatteryLevel() * 100) + " %");

			//Hier while schleife, die durch alle widgets laeuft!
			//Abfragen ueber welchem Objekt sich der Wii-Controller befindet und behandlung des Events
			if (Core::getSingletonPtr()->getWiiManager()->buttonA)
			{
				this->enterButton->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
				this->optionsButton->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
				this->exitButton->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
				this->rendererMenu->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
				this->configurationButton->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
				this->wiiConfigurationButton->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
				this->audioConfigurationButton->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
				this->fSAAMenu->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
				this->vSyncMenu->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
				this->videoModeMenu->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
				if (true == MenuState::bShowCameraSettings)
				{
					this->viewRangeSlider->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
				}
				this->lODBiasSlider->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
				this->textureFilteringMenu->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
				this->exitButton->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
				this->anisotropySlider->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
				this->optionsApplyButton->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
				this->optionsAbordButton->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
				this->wiiUseCheckBox->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
				this->wiiIRPosMenu->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
				this->wiiAspectRatioMenu->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
				this->soundVolumeSlider->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
				this->musicVolumeSlider->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
				//Core::getSingletonPtr()->getTrayManager()->getWidget("AOFTrayMgr/YesButton")->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
				//Core::getSingletonPtr()->getTrayManager()->getWidget("AOFTrayMgr/NoButton")->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
			}
			else
			{
				this->enterButton->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
				this->optionsButton->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
				this->exitButton->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
				this->rendererMenu->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
				this->configurationButton->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
				this->wiiConfigurationButton->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
				this->audioConfigurationButton->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
				this->fSAAMenu->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
				this->vSyncMenu->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
				this->videoModeMenu->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
				if (true == MenuState::bShowCameraSettings)
				{
					this->viewRangeSlider->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
				}
				this->lODBiasSlider->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
				this->textureFilteringMenu->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
				this->exitButton->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
				this->anisotropySlider->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
				this->optionsApplyButton->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
				this->optionsAbordButton->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
				this->wiiUseCheckBox->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
				this->wiiIRPosMenu->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
				this->wiiAspectRatioMenu->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
				this->soundVolumeSlider->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
				this->musicVolumeSlider->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
				//Core::getSingletonPtr()->getTrayManager()->getWidget("AOFTrayMgr/YesButton")->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
			   // Core::getSingletonPtr()->getTrayManager()->getWidget("AOFTrayMgr/NoButton")->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
			}
		}

		if (this->bQuit)
			this->shutdown();
	}

	void MenuState::yesNoDialogClosed(const Ogre::DisplayString &question, bool yesHit)
	{
		//Falls beenden gedrueckt wurde, wird der Zustand beendet
		if (yesHit == true)
			this->bQuit = true;
		else
			Core::getSingletonPtr()->getTrayManager()->closeDialog();
	}

}; // Namespace end
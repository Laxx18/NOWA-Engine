#include "main/MainConfigurationState.h"
#include "main/Core.h"
#include "main/WiiManager.h"
#include "utilities/Fader.h"

namespace NOWA
{

	MainConfigurationState::MainConfigurationState()
		:
		AppState()
	{
		// Attention: Konstruktor is only called once, hence do not init here, but static data
	}

	void MainConfigurationState::enter(void)
	{
		this->menuMusic = nullptr;
		Ogre::String title = "NOWA";
		NOWA::Core::getSingletonPtr()->changeWindowTitle(title); // hier von aussen eingeben koennen wieder WindowTitle sein soll
		//this->bQuit = false;
		FaderPlugin::getSingletonPtr()->startFadeIn(3.5f);
		Ogre::LogManager::getSingletonPtr()->logMessage("Entering MainConfigurationState...");

		//SceneManager erstellen
		this->sceneManager = Core::getSingletonPtr()->getOgreRoot()->createSceneManager(Ogre::ST_GENERIC, "MainConfigurationState");
		this->sceneManager->setAmbientLight(Ogre::ColourValue(0.7f, 0.7f, 0.7f));
		this->sceneManager->addRenderQueueListener(Core::getSingletonPtr()->getOverlaySystem());

		//Kamera erstellen
		this->camera = this->sceneManager->createCamera("MainConfigurationState");
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
		if (!Core::getSingletonPtr()->isStartedAsServer())
		{
			this->createBackgroundMusic();
		}
	}

	//Kamera, GUI und den SceneManager zerstoeren
	void MainConfigurationState::exit()
	{
		FaderPlugin::getSingletonPtr()->startFadeOut();

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MainConfigurationState] Leaving...");

		this->sceneManager->removeRenderQueueListener(Core::getSingletonPtr()->getOverlaySystem());
		Core::getSingletonPtr()->deleteGui();

		if (!Core::getSingletonPtr()->isStartedAsServer())
		{
			//Wii Infrarotkamera soll nicht mehr direkt benutzbar sein
			Core::getSingletonPtr()->getWiiManager()->useInfrateCameraDirectly(false);
			OgreALModule::getInstance()->deleteSound(this->menuMusic);
		}

		Core::getSingletonPtr()->destroyScene(this->sceneManager);
	}

	//Hier momentan kein Inhalt zum rendern
	void MainConfigurationState::createScene()
	{
	}

	void MainConfigurationState::createBackgroundMusic(void)
	{
		this->menuMusic = OgreALModule::getInstance()->createMusic("MenuMusic1", "kickstarter.ogg", true, true);

		//Hintergrundmusik kann beginnen, falls sie erfolgreich erstellt wurde
		this->menuMusic->play();
	}

	void MainConfigurationState::checkWiiAvailable(void)
	{
		if (Core::getSingletonPtr()->isWiiAvailable())
		{
			Core::getSingletonPtr()->getWiiManager()->setConfiguration(this->wiiIRPosMenu->getSelectionIndex(), this->wiiAspectRatioMenu->getSelectionIndex());
			//Nutzung des Wii-Controllers?
			this->wiiUseCheckBox->setChecked(Core::getSingletonPtr()->getOptionWiiUse());
			//Die Infrarotkamera der Wii-Soll direkt (ohne das Festhalten einer Taste) benutzbar sein
			Core::getSingletonPtr()->getWiiManager()->useInfrateCameraDirectly(true);
		}
	}

	void MainConfigurationState::setupWidgets(void)
	{
		//GUI erstellen
		Core::getSingletonPtr()->getTrayManager()->destroyAllWidgets();
		Core::getSingletonPtr()->getTrayManager()->showCursor();
		Core::getSingletonPtr()->getTrayManager()->createLabel(OgreBites::TL_TOP, "MainConfigurationState", "Menue", 250);

		//this->pGameEnterButton = Core::getSingletonPtr()->getTrayManager()->createButton(OgreBites::TL_CENTER, "GameEnterButton", "Spiel starten", 250);
		this->clientEnterButton = Core::getSingletonPtr()->getTrayManager()->createButton(OgreBites::TL_CENTER, "ClientEnterButton", "Start as Client", 250);
		this->serverEnterButton = Core::getSingletonPtr()->getTrayManager()->createButton(OgreBites::TL_CENTER, "ServerEnterButton", "Start as Server", 250);
		this->optionsButton = Core::getSingletonPtr()->getTrayManager()->createButton(OgreBites::TL_CENTER, "OptionsButton", "Options", 250);
		this->exitButton = Core::getSingletonPtr()->getTrayManager()->createButton(OgreBites::TL_CENTER, "ExitButton", "Exit Application", 250);

		this->configurationButton = Core::getSingletonPtr()->getTrayManager()->createButton(OgreBites::TL_NONE, "ConfigurationButton", "Main-Configuration", 200);
		this->configurationButton->hide();
		this->wiiConfigurationButton = Core::getSingletonPtr()->getTrayManager()->createButton(OgreBites::TL_NONE, "WiiConfigurationButton", "Wii-Configuration", 200);
		this->wiiConfigurationButton->hide();
		this->audioConfigurationButton = Core::getSingletonPtr()->getTrayManager()->createButton(OgreBites::TL_NONE, "AudioConfigurationButton", "Audio-Configuration", 200);
		this->audioConfigurationButton->hide();

		this->rendererMenu = Core::getSingletonPtr()->getTrayManager()->createLongSelectMenu(OgreBites::TL_NONE, "RendererMenu", "Rendersystem", 450, 240, 10);
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
		this->viewRangeSlider = Core::getSingletonPtr()->getTrayManager()->createThickSlider(OgreBites::TL_NONE, "ViewRangeSlider", "Viewrange (meters)", 450, 70, 0, 0, 0);
		this->viewRangeSlider->setRange(50, 5000, 100);
		this->viewRangeSlider->setValue(Core::getSingletonPtr()->getOptionViewRange());
		//Event muss bei Slidern 1x aufgerufen werden, sonst gibt es einen Absturz wenn Sliderfunktion verwendet wird
		///this->sliderMoved(this->viewRangeSlider); 
		this->viewRangeSlider->hide();
		this->lODBiasSlider = Core::getSingletonPtr()->getTrayManager()->createThickSlider(OgreBites::TL_NONE, "LODBiasSlider", "LODBias", 450, 70, 0, 0, 0);
		this->lODBiasSlider->setRange(1, 2, 3);
		this->lODBiasSlider->setValue(Core::getSingletonPtr()->getOptionLODBias());
		this->lODBiasSlider->hide();
		this->textureFilteringMenu = Core::getSingletonPtr()->getTrayManager()->createLongSelectMenu(OgreBites::TL_NONE, "TextureFilteringMenu", "Textur-Filtering", 450, 240, 10);
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

		//Wii-Konfiguration erstellen
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

		//Audio-Konfiguration erstellen
		this->soundVolumeSlider = Core::getSingletonPtr()->getTrayManager()->createThickSlider(OgreBites::TL_NONE, "SoundVolumeSlider", "Sound Volume", 450, 70, 0, 0, 0);
		this->soundVolumeSlider->setRange(0, 100, 101);
		this->soundVolumeSlider->setValue(static_cast<Ogre::Real>(OgreALModule::getInstance()->getSoundVolume()));
		this->soundVolumeSlider->hide();
		this->musicVolumeSlider = Core::getSingletonPtr()->getTrayManager()->createThickSlider(OgreBites::TL_NONE, "MusicVolumeSlider", "Musik Volume", 450, 70, 0, 0, 0);
		this->musicVolumeSlider->setRange(0, 100, 101);
		this->musicVolumeSlider->setValue(static_cast<Ogre::Real>(OgreALModule::getInstance()->getMusicVolume()));
		this->musicVolumeSlider->hide();
		//Event muss bei Slidern 1x aufgerufen werden, sonst gibt es einen Absturz wenn Sliderfunktion verwendet wird
		this->sliderMoved(this->musicVolumeSlider);

		//alle Konfikgurationsenues mit Daten fuellen
		this->populateOptions();
	}

	void MainConfigurationState::populateOptions(void)
	{
		//Rendersysteme ins Menue laden
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
			strOptionNames.push_back(possibleOptions.at(i));
		this->fSAAMenu->setItems(strOptionNames);
		this->fSAAMenu->selectItem(configOption.currentValue);
		this->strOldFSAA = this->fSAAMenu->getSelectedItem();
		strOptionNames.clear();

		//Vollbildoptionen
		configOption = configOptions.find("Full Screen")->second;
		possibleOptions = configOption.possibleValues;
		for (unsigned int i = 0; i < possibleOptions.size(); i++)
			strOptionNames.push_back(possibleOptions.at(i));
		this->fullscreenMenu->setItems(strOptionNames);
		this->fullscreenMenu->selectItem(configOption.currentValue);
		this->strOldFullscreen = this->fullscreenMenu->getSelectedItem();
		strOptionNames.clear();

		//Vertikale Synchronisation Optionen
		configOption = configOptions.find("VSync")->second;
		possibleOptions = configOption.possibleValues;
		for (unsigned int i = 0; i < possibleOptions.size(); i++)
			strOptionNames.push_back(possibleOptions.at(i));
		this->vSyncMenu->setItems(strOptionNames);
		this->vSyncMenu->selectItem(configOption.currentValue);
		this->strOldVSync = this->vSyncMenu->getSelectedItem();
		strOptionNames.clear();

		//Aufloesungsoptionen
		configOption = configOptions.find("Video Mode")->second;
		possibleOptions = configOption.possibleValues;
		for (unsigned int i = 0; i < possibleOptions.size(); i++)
			strOptionNames.push_back(possibleOptions.at(i));
		this->videoModeMenu->setItems(strOptionNames);
		this->videoModeMenu->selectItem(configOption.currentValue);
		this->strOldVideoMode = this->videoModeMenu->getSelectedItem();
		strOptionNames.clear();

		//Textur-Filterung Optionen
		strOptionNames.push_back("Keine");
		strOptionNames.push_back("Bilinear");
		strOptionNames.push_back("Trilinear");
		strOptionNames.push_back("Anisotropic");
		this->textureFilteringMenu->setItems(strOptionNames);
		this->textureFilteringMenu->selectItem(Core::getSingletonPtr()->getOptionTextureFiltering());
		strOptionNames.clear();

		//Position Wii-Infrarotleiste
		strOptionNames.push_back("Auf dem Bildschirm");
		strOptionNames.push_back("Unter dem Bildschirm");
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
		if (!Core::getSingletonPtr()->isStartedAsServer())
		{
			if (Core::getSingletonPtr()->isWiiAvailable())
				this->wiiUseCheckBox->setChecked(Core::getSingletonPtr()->getOptionWiiUse());
			else
			{
				Core::getSingletonPtr()->setOptionWiiUse(false);
				this->wiiUseCheckBox->setChecked(false);
			}
		}
	}

	void MainConfigurationState::itemSelected(OgreBites::SelectMenu *pMenu)
	{
		this->infoLabel->setCaption("");
		if (pMenu == this->rendererMenu)
		{
			if (this->strOldRenderSystem != this->rendererMenu->getSelectedItem())
			{
				this->infoLabel->setCaption("Attention: Restart required!");
			}
		}
		if (pMenu == this->fSAAMenu)
		{
			if (this->strOldFSAA != this->fSAAMenu->getSelectedItem())
			{
				this->infoLabel->setCaption("Attention: Restart required!");
			}
		}
		else if (pMenu == this->fullscreenMenu)
		{
			if (this->strOldFullscreen != this->fullscreenMenu->getSelectedItem())
			{
				this->infoLabel->setCaption("Attention: Restart required!");
			}
		}
		else if (pMenu == this->vSyncMenu)
		{
			if (this->strOldVSync != this->vSyncMenu->getSelectedItem())
			{
				this->infoLabel->setCaption("Attention: Restart required!");
			}
		}
		else if (pMenu == this->videoModeMenu)
		{
			if (this->strOldVideoMode != this->videoModeMenu->getSelectedItem())
			{
				this->infoLabel->setCaption("Attention: Restart required!");
			}
		}
	}

	void MainConfigurationState::buttonHit(OgreBites::Button *pButton)
	{
		if (pButton->getName() == "ExitButton")
		{
			//Core::getSingletonPtr()->getTrayManager()->showYesNoDialog("Simulation", "Zurueck zu Windows?");
			this->bQuit = true;
		}
		else if (pButton->getName() == "ClientEnterButton")
		{
			if (!Core::getSingletonPtr()->isStartedAsServer())
			{
				this->menuMusic->pause();
			}
			this->changeAppState(this->findByName("ClientConfigurationState"));
			// this->changeAppState(this->findByName(this->nextAppStateName));
		}
		else if (pButton->getName() == "ServerEnterButton")
		{
			this->changeAppState(this->findByName("ServerConfigurationState"));
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
	}

	void MainConfigurationState::applyOptions(void)
	{
		// when a critical setting has been adjusted, a restart will be required
		if (this->strOldRenderSystem != this->rendererMenu->getSelectedItem())
		{
			this->bQuit = true;
		}
		if (this->strOldFSAA != this->fSAAMenu->getSelectedItem())
		{
			this->bQuit = true;
		}
		if (this->strOldFullscreen != this->fullscreenMenu->getSelectedItem())
		{
			this->bQuit = true;
		}
		if (this->strOldVSync != this->vSyncMenu->getSelectedItem())
		{
			this->bQuit = true;
		}
		if (this->strOldVideoMode != this->videoModeMenu->getSelectedItem())
		{
			this->bQuit = true;
		}
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

		if (!Core::getSingletonPtr()->isStartedAsServer())
		{
			if (Core::getSingletonPtr()->isWiiAvailable())
			{
				Core::getSingletonPtr()->getWiiManager()->setConfiguration(this->wiiIRPosMenu->getSelectionIndex(), this->wiiAspectRatioMenu->getSelectionIndex());
			}
			Core::getSingletonPtr()->getOgreRoot()->saveConfig();
			Core::getSingletonPtr()->setOptionViewRange(this->viewRangeSlider->getValue());
			Core::getSingletonPtr()->setOptionLODBias(this->lODBiasSlider->getValue());
			Core::getSingletonPtr()->setOptionTextureFiltering(this->textureFilteringMenu->getSelectionIndex());
			Core::getSingletonPtr()->setOptionAnisotropyLevel(this->anisotropySlider->getValue());
			Core::getSingletonPtr()->setOptionWiiIRPos(this->wiiIRPosMenu->getSelectionIndex());
			Core::getSingletonPtr()->setOptionWiiAspectRatio(this->wiiAspectRatioMenu->getSelectionIndex());
			Core::getSingletonPtr()->setOptionWiiUse(this->wiiUseCheckBox->isChecked());
			Core::getSingletonPtr()->setOptionSoundVolume(this->soundVolumeSlider->getValue());
			Core::getSingletonPtr()->setOptionMusicVolume(this->musicVolumeSlider->getValue());
		}
		
		Core::getSingletonPtr()->saveCustomConfiguration();
	}

	void MainConfigurationState::switchConfiguration(int state)
	{
		// main menu
		if (state == 0)
		{
			// hide all configs
			while (Core::getSingletonPtr()->getTrayManager()->getTrayContainer(OgreBites::TL_CENTER)->isVisible())
			{
				Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(OgreBites::TL_CENTER, 0, OgreBites::TL_NONE);
			}
			this->configurationButton->hide();
			this->wiiConfigurationButton->hide();
			this->audioConfigurationButton->hide();
			this->configSeparator->hide();
			this->rendererMenu->hide();
			this->fSAAMenu->hide();
			this->fullscreenMenu->hide();
			this->vSyncMenu->hide();
			this->videoModeMenu->hide();
			this->viewRangeSlider->hide();
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

			//this->pGameEnterButton->show();
			//Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->pGameEnterButton, OgreBites::TL_CENTER);
			this->clientEnterButton->show();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->clientEnterButton, OgreBites::TL_CENTER);
			this->serverEnterButton->show();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->serverEnterButton, OgreBites::TL_CENTER);
			this->optionsButton->show();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->optionsButton, OgreBites::TL_CENTER);
			this->exitButton->show();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->exitButton, OgreBites::TL_CENTER);
			this->infoLabel->setCaption("");
		}
		// config menu
		else if (state == 1)
		{
			// hide main menu

			Core::getSingletonPtr()->getTrayManager()->removeWidgetFromTray(this->optionsButton);
			Core::getSingletonPtr()->getTrayManager()->removeWidgetFromTray(this->exitButton);

			this->optionsButton->hide();
			this->exitButton->hide();

			Core::getSingletonPtr()->getTrayManager()->removeWidgetFromTray(this->clientEnterButton);
			Core::getSingletonPtr()->getTrayManager()->removeWidgetFromTray(this->serverEnterButton);
			this->clientEnterButton->hide();
			this->serverEnterButton->hide();

			// show config menu
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->configurationButton, OgreBites::TL_CENTER);
			this->configurationButton->show();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->wiiConfigurationButton, OgreBites::TL_CENTER);
			this->wiiConfigurationButton->show();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->audioConfigurationButton, OgreBites::TL_CENTER);
			this->audioConfigurationButton->show();

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
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->viewRangeSlider, OgreBites::TL_CENTER);
			this->viewRangeSlider->show();
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

			// hide wii configs
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

			// hide audio configs
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->soundVolumeSlider, OgreBites::TL_NONE);
			this->soundVolumeSlider->hide();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->musicVolumeSlider, OgreBites::TL_NONE);
			this->musicVolumeSlider->hide();
		}
		// wii config menu
		else if (state == 2)
		{
			// hide config menu
			this->configSeparator->hide();
			this->rendererMenu->hide();
			this->fSAAMenu->hide();
			this->fullscreenMenu->hide();
			this->vSyncMenu->hide();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->vSyncMenu, OgreBites::TL_NONE);
			this->videoModeMenu->hide();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->viewRangeSlider, OgreBites::TL_NONE);
			this->viewRangeSlider->hide();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->lODBiasSlider, OgreBites::TL_NONE);
			this->lODBiasSlider->hide();

			//hide audio menu
			this->soundVolumeSlider->hide();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->soundVolumeSlider, OgreBites::TL_NONE);
			this->musicVolumeSlider->hide();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->musicVolumeSlider, OgreBites::TL_NONE);

			// the last 2 objects will be replaced with the new ones, hence moveWidgetToTray
			this->textureFilteringMenu->hide();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->anisotropySlider, OgreBites::TL_NONE);
			this->anisotropySlider->hide();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->infoLabel, OgreBites::TL_NONE);
			this->infoLabel->hide();

			// show wii config menu
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->wiiInfoLabel, OgreBites::TL_CENTER);
			this->wiiInfoLabel->show();
			if (Core::getSingletonPtr()->isWiiAvailable())
			{
				this->wiiInfoLabel->setCaption("The Wii-Controller is ready!");
			}
			else
			{
				this->wiiInfoLabel->setCaption("The Wii-Controller is not ready!");
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
		// audio config
		else if (state == 3)
		{
			// hide config menu
			this->configSeparator->hide();
			this->rendererMenu->hide();
			this->fSAAMenu->hide();
			this->fullscreenMenu->hide();
			this->vSyncMenu->hide();
			//Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->vSyncMenu, OgreBites::TL_NONE);
			this->videoModeMenu->hide();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->viewRangeSlider, OgreBites::TL_NONE);
			this->viewRangeSlider->hide();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->lODBiasSlider, OgreBites::TL_NONE);
			this->lODBiasSlider->hide();

			// hide audio config menu
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

			// the last 2 objects will be replaced with the new ones, hence moveWidgetToTray
			this->textureFilteringMenu->hide();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->anisotropySlider, OgreBites::TL_NONE);
			this->anisotropySlider->hide();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->infoLabel, OgreBites::TL_NONE);
			this->infoLabel->hide();

			// show audio config
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->soundVolumeSlider, OgreBites::TL_CENTER);
			this->soundVolumeSlider->show();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->musicVolumeSlider, OgreBites::TL_CENTER);
			this->musicVolumeSlider->show();

			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->optionsApplyButton, OgreBites::TL_CENTER);
			this->optionsApplyButton->show();
			Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->optionsAbordButton, OgreBites::TL_CENTER);
			this->optionsAbordButton->show();
		}
	}

	void MainConfigurationState::sliderMoved(OgreBites::Slider *pSlider)
	{
		if (!Core::getSingletonPtr()->isStartedAsServer())
		{
			if (this->menuMusic && pSlider->getName() == "MusicVolumeSlider")
			{
				this->menuMusic->setGain(pSlider->getValue() / 100.0f);
			}
		}
	}

	bool MainConfigurationState::keyPressed(const OIS::KeyEvent &keyEventRef)
	{
		if (Core::getSingletonPtr()->getKeyboard()->isKeyDown(OIS::KC_ESCAPE))
		{
			this->bQuit = true;
			return true;
		}
		// pushing F1 exits the wii
		if (Core::getSingletonPtr()->getKeyboard()->isKeyDown(OIS::KC_F1))
		{
			this->wiiUseCheckBox->setChecked(false);
			Core::getSingletonPtr()->setOptionWiiUse(false);
			this->applyOptions();
			return true;
		}

		Core::getSingletonPtr()->keyPressed(keyEventRef);
		return true;
	}

	bool MainConfigurationState::keyReleased(const OIS::KeyEvent &keyEventRef)
	{
		Core::getSingletonPtr()->keyReleased(keyEventRef);
		return true;
	}

	bool MainConfigurationState::mouseMoved(const OIS::MouseEvent &evt)
	{
		if (Core::getSingletonPtr()->getTrayManager()->injectMouseMove(evt))
		{
			return true;
		}
		return true;
	}

	bool MainConfigurationState::mousePressed(const OIS::MouseEvent &evt, OIS::MouseButtonID id)
	{
		if (Core::getSingletonPtr()->getTrayManager()->injectMouseDown(evt, id))
		{
			return true;
		}
		return true;
	}

	bool MainConfigurationState::mouseReleased(const OIS::MouseEvent &evt, OIS::MouseButtonID id)
	{
		if (Core::getSingletonPtr()->getTrayManager()->injectMouseUp(evt, id))
		{
			return true;
		}
		return true;
	}

	void MainConfigurationState::update(Ogre::Real dt)
	{
		if (Core::getSingletonPtr()->isWiiAvailable())
		{
			this->wiiBatteryLevelLabel->setCaption("Wii battery power: " + Ogre::StringConverter::toString(Core::getSingletonPtr()->getWiiManager()->getBatteryLevel() * 100) + " %");
		}
		//if (Core::getSingletonPtr()->isWiiAvailable() && Core::getSingletonPtr()->getOptionWiiUse())
		//{
		//	Ogre::Real wiiX = Core::getSingletonPtr()->getWiiManager()->wiiX;
		//	Ogre::Real wiiY = Core::getSingletonPtr()->getWiiManager()->wiiY;

		//	//Maus aus der Gui erhalten
		//	Ogre::OverlayContainer *pCursor = Core::getSingletonPtr()->getTrayManager()->getCursorContainer();
		//	//Mauszeiger an Wiikoordinater der Infrarotkamera setzen
		//	pCursor->setPosition(wiiX, wiiY);

		//	this->wiiBatteryLevelLabel->setCaption("Batterieanzeige: " + Ogre::StringConverter::toString( Core::getSingletonPtr()->getWiiManager()->getBatteryLevel() * 100) + " %");
		//	
		//	//Hier while schleife, die durch alle widgets laeuft!
		//	//Abfragen ueber welchem Objekt sich der Wii-Controller befindet und behandlung des Events
		//	if (Core::getSingletonPtr()->getWiiManager()->buttonA)
		//	{
		//	   this->clientEnterButton->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
		//	   this->serverEnterButton->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
		//	   this->optionsButton->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
		//	   this->exitButton->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
		//	   this->rendererMenu->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
		//	   this->configurationButton->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
		//	   this->wiiConfigurationButton->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
		//	   this->audioConfigurationButton->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
		//	   this->fSAAMenu->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
		//	   this->vSyncMenu->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
		//	   this->videoModeMenu->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
		//	   this->viewRangeSlider->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
		//	   this->lODBiasSlider->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
		//	   this->textureFilteringMenu->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
		//	   this->exitButton->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
		//	   this->anisotropySlider->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
		//	   this->optionsApplyButton->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
		//	   this->optionsAbordButton->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
		//	   this->wiiUseCheckBox->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
		//	   this->wiiIRPosMenu->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
		//	   this->wiiAspectRatioMenu->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
		//	   this->soundVolumeSlider->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
		//	   this->musicVolumeSlider->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
		//	   //Core::getSingletonPtr()->getTrayManager()->getWidget("AOFTrayMgr/YesButton")->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
		//	   //Core::getSingletonPtr()->getTrayManager()->getWidget("AOFTrayMgr/NoButton")->_cursorPressed(Ogre::Vector2(wiiX, wiiY));
		//	}
		//	else
		//	{
		//	   this->clientEnterButton->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
		//	   this->serverEnterButton->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
		//	   this->optionsButton->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
		//	   this->exitButton->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
		//	   this->rendererMenu->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
		//	   this->configurationButton->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
		//	   this->wiiConfigurationButton->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
		//	   this->audioConfigurationButton->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
		//	   this->fSAAMenu->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
		//	   this->vSyncMenu->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
		//	   this->videoModeMenu->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
		//	   this->viewRangeSlider->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
		//	   this->lODBiasSlider->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
		//	   this->textureFilteringMenu->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
		//	   this->exitButton->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
		//	   this->anisotropySlider->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
		//	   this->optionsApplyButton->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
		//	   this->optionsAbordButton->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
		//	   this->wiiUseCheckBox->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
		//	   this->wiiIRPosMenu->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
		//	   this->wiiAspectRatioMenu->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
		//	   this->soundVolumeSlider->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
		//	   this->musicVolumeSlider->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
		//	   //Core::getSingletonPtr()->getTrayManager()->getWidget("AOFTrayMgr/YesButton")->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
		//	  // Core::getSingletonPtr()->getTrayManager()->getWidget("AOFTrayMgr/NoButton")->_cursorReleased(Ogre::Vector2(wiiX, wiiY));
		//	} 
		//}

		if (this->bQuit)
		{
			this->shutdown();
		}
	}

	void MainConfigurationState::yesNoDialogClosed(const Ogre::DisplayString &question, bool yesHit)
	{
		if (yesHit == true)
		{
			this->bQuit = true;
		}
		else
		{
			Core::getSingletonPtr()->getTrayManager()->closeDialog();
		}
	}

}; // namespace end

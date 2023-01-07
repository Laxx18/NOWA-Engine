#include "main/ClientConfigurationState.h"
#include "main/Core.h"
#include "main/WiiManager.h"
#include "utilities/Fader.h"
#include "modules/RakNetModule.h"
#include "modules/OgreALModule.h"

namespace NOWA
{

	ClientConfigurationState::ClientConfigurationState()
		: AppState()
	{
		// do not init something here
	}

	void ClientConfigurationState::enter(void)
	{
		Ogre::String windowTitle = "OBA [Client]";
		NOWA::Core::getSingletonPtr()->changeWindowTitle(windowTitle.c_str());

		this->nameTextBoxActive = false;
		this->setServerIPTextBoxActive = false;
		this->interpolationFirstTime = true;

		FaderPlugin::getSingletonPtr()->startFadeIn(2.5f);

		//SceneManager erstellen
		this->sceneManager = Core::getSingletonPtr()->getOgreRoot()->createSceneManager(Ogre::ST_GENERIC, "ClientConfigurationState");
		this->sceneManager->setAmbientLight(Ogre::ColourValue(0.7f, 0.7f, 0.7f));
		this->sceneManager->addRenderQueueListener(Core::getSingletonPtr()->getOverlaySystem());

		//Kamera erstellen
		this->camera = this->sceneManager->createCamera("ClientConfigurationState");
		this->camera->setPosition(Ogre::Vector3(0, 25, -50));
		this->camera->lookAt(Ogre::Vector3(0, 0, 0));
		this->camera->setNearClipDistance(1);

		//Bildschirmverhaeltnis angeben
		//this->camera->setAspectRatio(Ogre::Real(Core::getSingletonPtr()->getOgreViewport()->getActualWidth()) /
		//Ogre::Real(Core::getSingletonPtr()->getOgreViewport()->getActualHeight()));
		this->camera->setAutoAspectRatio(true);

		Core::getSingletonPtr()->getOgreViewport()->setCamera(this->camera);

		this->setupWidgets();

		this->createScene();
		if (!Core::getSingletonPtr()->isStartedAsServer())
		{
			this->createBackgroundMusic();
		}

		RakNetModule::getInstance()->createRakNetForClient();

		this->searchServer();
	}

	void ClientConfigurationState::searchServer(void)
	{
		bool serverFound = RakNetModule::getInstance()->findServerAndCreateClient();
		if (serverFound)
		{
			this->foundServerLabel->setCaption(RakNetModule::getInstance()->getServerName() + ":"
				+ RakNetModule::getInstance()->getResourceGroupToVirtualEnvironment()
				+ "-" + RakNetModule::getInstance()->getVirtualEnvironmentName());
			this->serverIPLabel->setCaption("ServerIP: " + RakNetModule::getInstance()->getServerIP());
		}
		else
		{
			this->foundServerLabel->setCaption("No server found!");
		}
	}

	//Kamera, GUI und den SceneManager zerstoeren
	void ClientConfigurationState::exit()
	{
		FaderPlugin::getSingletonPtr()->startFadeOut();

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ClientConfigurationState] Leaving...");

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

	void ClientConfigurationState::createBackgroundMusic(void)
	{
		this->menuMusic = OgreALModule::getInstance()->createSound("MenuMusic1", "kickstarter.ogg", true, true);
		//Hintergrundmusik kann beginnen, falls sie erfolgreich erstellt wurde
		if (this->menuMusic)
		{
			this->menuMusic->play();
		}
		else
		{
			OgreALModule::getInstance()->getSound("MenuMusic1")->play();
		}
	}

	void ClientConfigurationState::setupWidgets(void)
	{
		//GUI erstellen
		Core::getSingletonPtr()->getTrayManager()->destroyAllWidgets();
		Core::getSingletonPtr()->getTrayManager()->showCursor();
		Core::getSingletonPtr()->getTrayManager()->createLabel(OgreBites::TL_TOP, "ClientConfigurationState", "Client Optionen", 380);
		this->startGameButton = Core::getSingletonPtr()->getTrayManager()->createButton(OgreBites::TL_CENTER, "StartGameButton", "Start game", 380);
		this->startGameButton->hide();
		this->connectToServerButton = Core::getSingletonPtr()->getTrayManager()->createButton(OgreBites::TL_CENTER, "ConnectToServerButton", "Connect to server", 380);
		//TextBox Hoehe muss auf 80 gesetzt sein um den Text eintragen zu koennen
		this->nameTextBox = Core::getSingletonPtr()->getTrayManager()->createTextBox(OgreBites::TL_CENTER, "NameTextBox", "player name:", 380, 80);
		//StandartSpielernamen einfuegen
		this->nameTextBox->setText(RakNetModule::getInstance()->getPlayerName());
		this->nameOkButton = Core::getSingletonPtr()->getTrayManager()->createButton(OgreBites::TL_CENTER, "NameOkButton", "commit name", 380);
		this->selectPlayerMenu = Core::getSingletonPtr()->getTrayManager()->createLongSelectMenu(OgreBites::TL_CENTER, "SelectPlayerMenu", "player colour:", 380, 180, 10);
		Core::getSingletonPtr()->getTrayManager()->createSeparator(OgreBites::TL_NONE, "Separator1");

		this->searchServerButton = Core::getSingletonPtr()->getTrayManager()->createButton(OgreBites::TL_CENTER, "SearchServerButton", "Search server...", 380);
		this->serverIPLabel = Core::getSingletonPtr()->getTrayManager()->createLabel(OgreBites::TL_CENTER, "ServerIPLabel", "___.___.___.___", 380);
		//TextBox Hoehe muss auf 80 gesetzt sein um den Text eintragen zu koennen
		this->setServerIPTextBox = Core::getSingletonPtr()->getTrayManager()->createTextBox(OgreBites::TL_CENTER, "SetServerIPTextBox", "Input server-ip manually (Internet)", 380, 80);
		this->setServerIPButton = Core::getSingletonPtr()->getTrayManager()->createButton(OgreBites::TL_CENTER, "SetServerIPButton", "commit IP", 380);
		this->foundServerLabel = Core::getSingletonPtr()->getTrayManager()->createLabel(OgreBites::TL_CENTER, "FoundServerLabel", "Found server:", 380);
		//this->packetSendRateSlider = Core::getSingletonPtr()->getTrayManager()->createThickSlider(OgreBites::TL_CENTER, "PacketSendRateSlider", "Pakete pro Sekunde", 380, 30, 0, 0, 0);
		//this->packetSendRateSlider->setRange(10, 30, 3);
		//this->packetSendRateSlider->setValue(10);
		this->packetSendRateMenu = Core::getSingletonPtr()->getTrayManager()->createLongSelectMenu(OgreBites::TL_CENTER, "PacketSendRateMenu", "Paketsendrate (ms):", 380, 180, 10);
		this->interpolationTimeMenu = Core::getSingletonPtr()->getTrayManager()->createLongSelectMenu(OgreBites::TL_CENTER, "InterpolationTimeMenu", "Interpolationrange (ms):", 380, 180, 10);

		//this->pInterpolationTimeSlider = NOWA::Core::getSingletonPtr()->getTrayManager()->createThickSlider(OgreBites::TL_CENTER, "InterpolationTimeSlider", "Interpolationsspanne", 380, 50, 0, 0, 0);
		////this->pInterpolationTimeSlider->setRange(30.0f, 510.0f, 17);
		//this->pInterpolationTimeSlider->setRange(17.0f, 510.0f, 30);
		////510(max) / 17 (min) = 30(snaps)
		////17, 34, 51, 68, 85, 102...
		//this->pInterpolationTimeSlider->setValue(NOWA::Core::getSingletonPtr()->interpolationTime);

		this->backButton = Core::getSingletonPtr()->getTrayManager()->createButton(OgreBites::TL_CENTER, "BackButton", "Back", 380);
		//this->configSeparator = Core::getSingletonPtr()->getTrayManager()->createSeparator(OgreBites::TL_CENTER, "ConfigSeparator");

		//alle Konfikgurationsenues mit Daten fuellen
		this->populateMenus();

		Core::getSingletonPtr()->getTrayManager()->showCursor();
	}

	void ClientConfigurationState::populateMenus(void)
	{
		//Interpolationsspannen in Menu eintragen
		//this->pInterpolationTimeMenu->addItem("16");
		this->interpolationTimeMenu->addItem("33");
		this->interpolationTimeMenu->addItem("50");
		this->interpolationTimeMenu->addItem("75");
		this->interpolationTimeMenu->addItem("100");
		this->interpolationTimeMenu->addItem("150");
		this->interpolationTimeMenu->addItem("200");
		this->interpolationTimeMenu->addItem("250");
		this->interpolationTimeMenu->addItem("500");
		this->interpolationTimeMenu->selectItem(0);

		//Paketsendraten in Menue eintragen
		this->packetSendRateMenu->addItem("16");
		this->packetSendRateMenu->addItem("33");
		this->packetSendRateMenu->addItem("50");
		this->packetSendRateMenu->addItem("100");
		this->packetSendRateMenu->selectItem(0);
	}

	void ClientConfigurationState::buttonHit(OgreBites::Button* button)
	{
		if (button->getName() == "ConnectToServerButton")
		{

			RakNetModule::getInstance()->setInterpolationTimeMS(Ogre::StringConverter::parseInt(this->interpolationTimeMenu->getSelectedItem(), 100.0f));
			//Core::getSingletonPtr()->packetSendRate = 1000 / this->packetSendRateSlider->getValue();
			RakNetModule::getInstance()->setPacketSendRateMS(Ogre::StringConverter::parseInt(this->packetSendRateMenu->getSelectedItem(), 100.0f));

			RakNetModule::getInstance()->startNetworking("Client");
			this->startGameButton->show();
		}
		else if (button->getName() == "StartGameButton")
		{
			if (!RakNetModule::getInstance()->isConnectionFailed())
			{
				this->changeAppState(this->findByName(this->nextAppStateName));
			}
			else
			{
				Core::getSingletonPtr()->getTrayManager()->showOkDialog("NOWA",
					"Failed to connect to server! Cannot start the app. Either the server is no reachable or all slots in the virtual environment are occupied.");
			}
		}
		else if (button->getName() == "BackButton")
		{
			this->disconnect();
			RakNetModule::getInstance()->destroyContent();
			this->changeAppState(this->findByName("MainConfigurationState"));
		}
		else if (button->getName() == "NameOkButton")
		{
			if (this->nameTextBox->getText().size() > 0)
			{
				this->nameTextBox->getOverlayElement()->setMaterialName("SdkTrays/TextBox");
				RakNetModule::getInstance()->setPlayerName(this->nameTextBox->getText());
				this->nameTextBoxActive = false;
			}
			else
			{
				Core::getSingletonPtr()->getTrayManager()->showOkDialog("NOWA", "Please specify a name.");
			}
		}
		else if (button->getName() == "SearchServerButton")
		{
			this->searchServer();
		}
		else if (button->getName() == "SetServerIPButton")
		{
			if (this->setServerIPTextBox->getText().size() > 0)
			{
				this->setServerIPTextBox->getOverlayElement()->setMaterialName("SdkTrays/TextBox");
				RakNetModule::getInstance()->setServerIP(this->setServerIPTextBox->getText());
				this->setServerIPTextBoxActive = false;
			}
			else
			{
				Core::getSingletonPtr()->getTrayManager()->showOkDialog("NOWA", "Please enter an ip address. example: '192.168.0.1'.");
			}
		}
	}

	void ClientConfigurationState::itemSelected(OgreBites::SelectMenu* menu)
	{
		if (menu == this->packetSendRateMenu)
		{
			this->packetSendRateMenu->setCaption(Ogre::StringConverter::toString(1000 / Ogre::StringConverter::parseInt(this->packetSendRateMenu->getSelectedItem())) + " Packets Per Second");
		}
	}

	void ClientConfigurationState::sliderMoved(OgreBites::Slider* slider)
	{
		//if (pSlider == this->pInterpolationTimeSlider)
		//{
		//	//Fehler bei der GUI: diese Funktion wird 1x beim initialisieren aufgerufen, somit war es nicht ohne weiteres moeglich
		//	//selbst festzulegen welchen Wert die Interpolationsspanne zu Beginn haben soll
		//	//Daher wird erst wenn der Slider auch bewegt wird,  der Wert gesetzt
		//	if (!this->interpolationFirstTime)
		//	    Core::getSingletonPtr()->interpolationTime = this->pInterpolationTimeSlider->getValue();
		//	else
		//		this->interpolationFirstTime = false;
		//}
	}

	void ClientConfigurationState::createScene()
	{

	}

	void ClientConfigurationState::disconnect(void)
	{
		RakNet::BitStream bitstream;

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ClientConfigurationState] exit for player Id: "
			+ Ogre::StringConverter::toString(RakNetModule::getInstance()->getClientID()) + " to connection: " 
			+ Ogre::String(RakNetModule::getInstance()->getServerAddress().ToString()));

		bitstream.Write((RakNet::MessageID)RakNetModule::ID_CLIENT_EXIT);
		
		// send data with high priority, reliable (TCP), 0, IP + Port, do not broadcast but just send to the client which attempted a connection
		RakNetModule::getInstance()->getRakPeer()->Send(&bitstream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNetModule::getInstance()->getServerAddress(), false);
	}

	bool ClientConfigurationState::keyPressed(const OIS::KeyEvent& keyEventRef)
	{
		
		if (Core::getSingletonPtr()->getKeyboard()->isKeyDown(OIS::KC_ESCAPE))
		{
			this->disconnect();
			RakNetModule::getInstance()->destroyContent();
			this->changeAppState(this->findByName("MainConfigurationState"));
			return true;
		}
		/*else if(Core::getSingletonPtr()->getKeyboard()->isKeyDown(OIS::KC_RETURN)) {
			//Paketsendrate berechnen
			RakNetModule::getInstance()->setInterpolationTimeMS(Ogre::StringConverter::parseInt(this->interpolationTimeMenu->getSelectedItem(), 100.0f));
			//Core::getSingletonPtr()->packetSendRate = 1000 / this->packetSendRateSlider->getValue();
			RakNetModule::getInstance()->setPacketSendRateMS(Ogre::StringConverter::parseInt(this->packetSendRateMenu->getSelectedItem(), 100.0f));
			//Audio loeschen
			OgreALModule::getInstance()->deleteSound(OgreALModule::getInstance()->getSound("MenuMusic1"));

			//Neuen Zustand beginnen
			this->changeAppState(this->findByName(this->nextAppStateName));
			}*/
		if (this->nameTextBoxActive)
		{
			Core::getSingletonPtr()->writeText(this->nameTextBox, keyEventRef, 15);
		}
		else if (this->setServerIPTextBoxActive)
		{
			Core::getSingletonPtr()->writeText(this->setServerIPTextBox, keyEventRef, 15);
		}

		Core::getSingletonPtr()->keyPressed(keyEventRef);
		return true;
	}

	bool ClientConfigurationState::keyReleased(const OIS::KeyEvent& keyEventRef)
	{
		Core::getSingletonPtr()->keyReleased(keyEventRef);
		return true;
	}

	//Mausbewegung an der GUI
	bool ClientConfigurationState::mouseMoved(const OIS::MouseEvent& evt)
	{
		if (Core::getSingletonPtr()->getTrayManager()->injectMouseMove(evt))
		{
			return true;
		}
		return true;
	}

	//Maustastendruck an der GUI
	bool ClientConfigurationState::mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
	{
		if (this->nameTextBox->isCursorOver(this->nameTextBox->getOverlayElement(), Ogre::Vector2(evt.state.X.abs, evt.state.Y.abs)))
		{
			this->nameTextBox->getOverlayElement()->setMaterialName("SdkTrays/Button/Over");
			this->setServerIPTextBox->getOverlayElement()->setMaterialName("SdkTrays/TextBox");
			this->nameTextBoxActive = true;
			this->setServerIPTextBoxActive = false;
		}
		else if (this->nameTextBox->isCursorOver(this->setServerIPTextBox->getOverlayElement(), Ogre::Vector2(evt.state.X.abs, evt.state.Y.abs)))
		{
			this->setServerIPTextBox->getOverlayElement()->setMaterialName("SdkTrays/Button/Over");
			this->nameTextBox->getOverlayElement()->setMaterialName("SdkTrays/TextBox");
			this->nameTextBoxActive = false;
			this->setServerIPTextBoxActive = true;
		}
		if (Core::getSingletonPtr()->getTrayManager()->injectMouseDown(evt, id))
		{
			return true;
		}
		return true;
	}

	//loslassen der Maustasten an der GUI
	bool ClientConfigurationState::mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
	{
		if (Core::getSingletonPtr()->getTrayManager()->injectMouseUp(evt, id))
		{
			return true;
		}
		return true;
	}

	void ClientConfigurationState::update(Ogre::Real dt)
	{
		RakNetModule::getInstance()->update(dt);
	}

	//Event which button has been pressed
	void ClientConfigurationState::yesNoDialogClosed(const Ogre::DisplayString& question, bool yesHit)
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

	void ClientConfigurationState::okDialogClosed(const Ogre::DisplayString& message)
	{

	}

}; // namespace end


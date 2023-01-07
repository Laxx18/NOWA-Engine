#include "main/ServerConfigurationState.h"
#include "main/Core.h"
#include "main/WiiManager.h"
#include "utilities/Fader.h"
#include "modules/RakNetModule.h"

namespace NOWA
{

	ServerConfigurationState::ServerConfigurationState()
		: AppState()
	{
		// do not init something here
	}

	void ServerConfigurationState::enter(void)
	{
		// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LogManager::getSingleton().createLog("OgreMyDebugLogServer.log", false, false, false));

		this->serverNameTextBoxActive = false;

		NOWA::Core::getSingletonPtr()->changeWindowTitle("OBA [Server]");

		FaderPlugin::getSingletonPtr()->startFadeIn(2.5f);

		//SceneManager erstellen
		this->sceneManager = Core::getSingletonPtr()->getOgreRoot()->createSceneManager(Ogre::ST_GENERIC, "ServerConfigurationState");
		this->sceneManager->setAmbientLight(Ogre::ColourValue(0.7f, 0.7f, 0.7f));
		this->sceneManager->addRenderQueueListener(Core::getSingletonPtr()->getOverlaySystem());

		//Kamera erstellen
		this->camera = this->sceneManager->createCamera("ServerConfigurationState");
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
	}

	//Kamera, GUI und den SceneManager zerstoeren
	void ServerConfigurationState::exit()
	{
		FaderPlugin::getSingletonPtr()->startFadeOut();

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ServerConfigurationState] Leaving...");

		this->sceneManager->removeRenderQueueListener(Core::getSingletonPtr()->getOverlaySystem());
		Core::getSingletonPtr()->deleteGui();

		if (!Core::getSingletonPtr()->isStartedAsServer())
		{
			//Wii Infrarotkamera soll nicht mehr direkt benutzbar sein
			Core::getSingletonPtr()->getWiiManager()->useInfrateCameraDirectly(false);
		}

		Core::getSingletonPtr()->destroyScene(this->sceneManager);
	}

	void ServerConfigurationState::setupWidgets(void)
	{
		//GUI erstellen
		Core::getSingletonPtr()->getTrayManager()->destroyAllWidgets();
		Core::getSingletonPtr()->getTrayManager()->showCursor();
		Core::getSingletonPtr()->getTrayManager()->createLabel(OgreBites::TL_TOP, "ServerConfigurationState", "Server Options", 380);

		this->serverSimulationButton = Core::getSingletonPtr()->getTrayManager()->createButton(OgreBites::TL_CENTER, "ServerSimulationButton", "Start server simulation", 380);
		this->serverConsoleButton = Core::getSingletonPtr()->getTrayManager()->createButton(OgreBites::TL_CENTER, "ServerConsoleButton", "Start server in console", 380);

		//TextBox Hoehe muss auf 80 gesetzt sein um den Text eintragen zu koennen
		this->serverNameTextBox = Core::getSingletonPtr()->getTrayManager()->createTextBox(OgreBites::TL_CENTER, "ServerNameTextBox", "Servername:", 380, 80);
		//Servernamen setzen
		this->serverNameTextBox->setText(RakNetModule::getInstance()->getServerName());

		this->serverNameButton = Core::getSingletonPtr()->getTrayManager()->createButton(OgreBites::TL_CENTER, "ServerNameButton", "Commit Servername", 380);

		this->mapMenu = Core::getSingletonPtr()->getTrayManager()->createLongSelectMenu(OgreBites::TL_CENTER, "MapMenu", "Maps:", 380, 200, 10);
		this->maxPlayerLabel = Core::getSingletonPtr()->getTrayManager()->createLabel(OgreBites::TL_CENTER, "MaxPlayerLabel", "Max. allowed playercount: ", 380);

		this->areaOfInterestSlider = NOWA::Core::getSingletonPtr()->getTrayManager()->createThickSlider(OgreBites::TL_CENTER, "AreaOfInterestSlider", "Area of interest", 380, 50, 0, 0, 0);
		this->areaOfInterestSlider->setRange(0, 100, 11);
		this->areaOfInterestSlider->setValue(static_cast<Ogre::Real>(Core::getSingletonPtr()->getOptionAreaOfInterest()));

		this->packetSendRateSlider = NOWA::Core::getSingletonPtr()->getTrayManager()->createThickSlider(OgreBites::TL_CENTER, "PacketSendRateSlider", "Packet send rate", 380, 50, 0, 0, 0);
		this->packetSendRateSlider->setRange(5, 60, 12);
		this->packetSendRateSlider->setValue(static_cast<Ogre::Real>(Core::getSingletonPtr()->getOptionPacketSendRate()));


		this->backButton = Core::getSingletonPtr()->getTrayManager()->createButton(OgreBites::TL_CENTER, "BackButton", "Back", 380);

		this->populateVirtualEnvironments();
	}

	//Alle virtuellen Umgebungen aus dem Ressourcenordner (media) erhalten und in eine Liste einfuegen
	void ServerConfigurationState::populateVirtualEnvironments(void)
	{
		std::map<Ogre::String, Ogre::String> mapNames = RakNetModule::getInstance()->parseVirtualEnvironments("World", "resourcesNetwork.cfg");
		//Karten hinzufuegen
		for (std::map<Ogre::String, Ogre::String>::iterator it = mapNames.begin(); it != mapNames.end(); ++it)
		{
			this->mapMenu->addItem((*it).first);
		}
		//this->pMapMenu->setItems(strMapNames);
		if (!mapNames.empty())
		{
			RakNetModule::getInstance()->setVirtualEnvironmentName(this->mapMenu->getSelectedItem());

			//Ressourcegruppe zur virtuellen Umgebung finden
			std::map<Ogre::String, Ogre::String>::iterator it = mapNames.find(this->mapMenu->getSelectedItem());

			RakNetModule::getInstance()->setResourceGroupToVirtualEnvironment(it->second);

			//Ogre::LogManager::getSingletonPtr()->logMessage("------------------->" + this->pMapMenu->getSelectedItem() + ".scene");
			//Startpositionen aus der virtuellen Umgebung erhalten
			RakNetModule::getInstance()->preParseVirtualEnvironment(
				RakNetModule::getInstance()->getResourceGroupToVirtualEnvironment(),
				RakNetModule::getInstance()->getVirtualEnvironmentName() + ".scene");
			this->maxPlayerLabel->setCaption("Player count: "
				+ Ogre::StringConverter::toString(RakNetModule::getInstance()->getAllowedPlayerCount()));
		}
	}

	void ServerConfigurationState::buttonHit(OgreBites::Button* button)
	{
		if (button->getName() == "ServerSimulationButton")
		{
			//Der Servername darf nicht leer sein
			if (this->serverNameTextBox->getText().size() > 0)
			{
				//Es kann nur eine karte gespielt werden, die Startpositionen besitzt
				if (RakNetModule::getInstance()->getAllowedPlayerCount() > 0)
				{
					RakNetModule::getInstance()->setAreaOfInterest(this->areaOfInterestSlider->getValue());
					RakNetModule::getInstance()->setPacketSendRateMS(1000.0f / this->packetSendRateSlider->getValue());
					Core::getSingletonPtr()->setOptionAreaOfInterest(this->areaOfInterestSlider->getValue());
					Core::getSingletonPtr()->setOptionPacketSendRate(this->packetSendRateSlider->getValue());
					RakNetModule::getInstance()->createRakNetForServer(this->serverNameTextBox->getText());
					Core::getSingletonPtr()->saveCustomConfiguration();
					//Neuen Zustand beginnen
					this->changeAppState(this->findByName(this->nextAppStateName));
				}
				else
				{
					Core::getSingletonPtr()->getTrayManager()->showOkDialog("NOWA",
						"This map can not be choosen, because it does not have any start position.");
				}
			}
			else
			{
				Core::getSingletonPtr()->getTrayManager()->showOkDialog("NOWA", "Please specify a server name.");
			}

		}
		else if (button->getName() == "ServerConsoleButton")
		{
			//Der Servername darf nicht leer sein
			if (this->serverNameTextBox->getText().size() > 0)
			{
				//Es kann nur eine karte gespielt werden, die Startpositionen besitzt
				if (RakNetModule::getInstance()->getAllowedPlayerCount() > 0)
				{
					RakNetModule::getInstance()->setServerInConsole(true);

					RakNetModule::getInstance()->createRakNetForServer(this->serverNameTextBox->getText());
					//Neuen Zustand beginnen
					this->changeAppState(this->findByName(this->nextAppStateName));
				}
				else
				{
					Core::getSingletonPtr()->getTrayManager()->showOkDialog("NOWA", "This map can not be choosen, because it does not have any start position.");
				}
			}
			else
				Core::getSingletonPtr()->getTrayManager()->showOkDialog("NOWA", "Please specify a server name.");
		}
		else if (button->getName() == "ServerNameButton")
		{
			if (this->serverNameTextBox->getText().size() > 0)
			{
				this->serverNameTextBox->getOverlayElement()->setMaterialName("SdkTrays/TextBox");
				RakNetModule::getInstance()->setServerName(this->serverNameTextBox->getText());
				this->serverNameTextBoxActive = false;
			}
			else
			{
				Core::getSingletonPtr()->getTrayManager()->showOkDialog("NOWA", "Please specify a server name.");
			}
		}
		else if (button->getName() == "BackButton")
		{
			//Wenn zurueck gegangen wird, dann werden RakNet geloescht und alle Daten geleert
			RakNetModule::getInstance()->destroyContent();

			this->changeAppState(this->findByName("MainConfigurationState"));
		}
	}

	void ServerConfigurationState::itemSelected(OgreBites::SelectMenu* menu)
	{
		if (menu == this->mapMenu)
		{
			//virtueller Umgebung auswaehlen
			RakNetModule::getInstance()->setVirtualEnvironmentName(this->mapMenu->getSelectedItem());
			//Ressourcegruppe zur virtuellen Umgebung finden
			std::map<Ogre::String, Ogre::String>::iterator it = RakNetModule::getInstance()->findVirtualEnvironment(this->mapMenu->getSelectedItem());
			RakNetModule::getInstance()->setResourceGroupToVirtualEnvironment(it->second);

			//Startpositonen aus der virtuellen Umgebung erhalten
			RakNetModule::getInstance()->preParseVirtualEnvironment(
				RakNetModule::getInstance()->getResourceGroupToVirtualEnvironment(),
				RakNetModule::getInstance()->getVirtualEnvironmentName() + ".scene");
			this->maxPlayerLabel->setCaption("Allowable player count: "
				+ Ogre::StringConverter::toString(RakNetModule::getInstance()->getAllowedPlayerCount()));
		}
	}

	void ServerConfigurationState::sliderMoved(OgreBites::Slider* slider)
	{

	}

	void ServerConfigurationState::createScene()
	{
	}

	bool ServerConfigurationState::keyPressed(const OIS::KeyEvent& keyEventRef)
	{
		if (this->serverNameTextBoxActive)
			Core::getSingletonPtr()->writeText(this->serverNameTextBox, keyEventRef, 15);
		//Zustand beenden
		if (Core::getSingletonPtr()->getKeyboard()->isKeyDown(OIS::KC_ESCAPE))
		{
			//Wenn zurueck gegangen wird, dann werden RakNet geloescht und alle Daten geleert
			RakNetModule::getInstance()->destroyContent();

			this->changeAppState(this->findByName("MainConfigurationState"));
			return true;
		}

		Core::getSingletonPtr()->keyPressed(keyEventRef);
		return true;
	}

	bool ServerConfigurationState::keyReleased(const OIS::KeyEvent& keyEventRef)
	{
		Core::getSingletonPtr()->keyReleased(keyEventRef);
		return true;
	}

	//Mausbewegung an der GUI
	bool ServerConfigurationState::mouseMoved(const OIS::MouseEvent& evt)
	{
		if (Core::getSingletonPtr()->getTrayManager()->injectMouseMove(evt))
			return true;
		return true;
	}

	//Maustastendruck an der GUI
	bool ServerConfigurationState::mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
	{
		if (this->serverNameTextBox->isCursorOver(this->serverNameTextBox->getOverlayElement(), Ogre::Vector2(evt.state.X.abs, evt.state.Y.abs)))
		{
			this->serverNameTextBox->getOverlayElement()->setMaterialName("SdkTrays/Button/Over");
			this->serverNameTextBoxActive = true;
		}
		if (Core::getSingletonPtr()->getTrayManager()->injectMouseDown(evt, id))
			return true;
		return true;
	}

	//loslassen der Maustasten an der GUI
	bool ServerConfigurationState::mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
	{
		if (Core::getSingletonPtr()->getTrayManager()->injectMouseUp(evt, id))
		{
			return true;
		}
		return true;
	}

	void ServerConfigurationState::update(Ogre::Real dt)
	{
		/* Scheinbar um IP auﬂerhalb von LAN fuer den Server zu erhalten. Siehe: ReplicaManager3 Beispiel
		RakSleep(30);
		for (int i=0; i < 32; i++)
		{
		if (rakPeer->GetInternalID(RakNet::UNASSIGNED_SYSTEM_ADDRESS,0).port!=SERVER_PORT+i)
		rakPeer->AdvertiseSystem("255.255.255.255", SERVER_PORT+i, 0,0,0);
		}
		*/
	}

	//Event welche Taste gedruekt wurde
	void ServerConfigurationState::yesNoDialogClosed(const Ogre::DisplayString& question, bool yesHit)
	{
		//Falls beenden gedrueckt wurde, wird der Zustand beendet
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



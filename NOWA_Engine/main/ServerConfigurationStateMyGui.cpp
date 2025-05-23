#include "NOWAPrecompiled.h"
#include "ServerConfigurationStateMyGui.h"
#include "Core.h"
#include "InputDeviceCore.h"
#include "utilities/FaderProcess.h"
#include "modules/RakNetModule.h"
#include "modules/WorkspaceModule.h"
#include "main/AppStateManager.h"

namespace NOWA
{

	ServerConfigurationStateMyGui::ServerConfigurationStateMyGui()
		: AppState()
	{
		// do not init something here
	}

	void ServerConfigurationStateMyGui::enter(void)
	{
		NOWA::AppState::enter();

		// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LogManager::getSingleton().createLog("OgreMyDebugLogServer.log", false, false, false));

		this->mapsCombo = nullptr;
		this->packetSendRateCombo = nullptr;
		this->areaOfInterestSlider = nullptr;

		NOWA::Core::getSingletonPtr()->changeWindowTitle("NOWA [Server]");

		ProcessManager::getInstance()->attachProcess(ProcessPtr(new FaderProcess(FaderProcess::FadeOperation::FADE_IN, 2.5f)));

		ENQUEUE_RENDER_COMMAND_WAIT("ServerConfigurationStateMyGui::enter",
		{
			this->sceneManager = Core::getSingletonPtr()->getOgreRoot()->createSceneManager(Ogre::ST_GENERIC, 1, "ServerConfigurationStateMyGui");
			// this->sceneManager->setAmbientLight(Ogre::ColourValue(0.7f, 0.7f, 0.7f));
			this->sceneManager->addRenderQueueListener(Core::getSingletonPtr()->getOverlaySystem());

			this->camera = this->sceneManager->createCamera("ServerConfigurationStateMyGui");
			this->camera->setPosition(Ogre::Vector3(0, 25, -50));
			this->camera->lookAt(Ogre::Vector3(0, 0, 0));
			this->camera->setNearClipDistance(1);
			this->camera->setAutoAspectRatio(true);

			WorkspaceModule::getInstance()->setPrimaryWorkspace(this->sceneManager, this->camera, nullptr);

			this->initializeModules(false, false);

			this->setupWidgets();
			this->createScene();
		});
	}

	//Kamera, GUI und den SceneManager zerstoeren
	void ServerConfigurationStateMyGui::exit()
	{
		NOWA::AppState::exit();

		ProcessManager::getInstance()->attachProcess(ProcessPtr(new FaderProcess(FaderProcess::FadeOperation::FADE_OUT, 2.5f)));
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ServerConfigurationStateMyGui] Leaving...");
		
		ENQUEUE_RENDER_COMMAND_WAIT("ServerConfigurationStateMyGui::exit",
		{
			MyGUI::Gui::getInstancePtr()->destroyWidget(MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::ImageBox>("Background"));
			MyGUI::LayoutManager::getInstancePtr()->unloadLayout(this->widgets);
		});
		this->mapsCombo = nullptr;
		this->packetSendRateCombo = nullptr;
		this->areaOfInterestSlider = nullptr;

		this->destroyModules();
	}

	void ServerConfigurationStateMyGui::setupWidgets(void)
	{
		// Create Gui
		Core::getSingletonPtr()->setSceneManagerForMyGuiPlatform(this->sceneManager);

		MyGUI::Gui::getInstancePtr()->createWidget<MyGUI::ImageBox>("RotatingSkin",
			MyGUI::IntCoord(0, 0, Core::getSingletonPtr()->getOgreRenderWindow()->getWidth(), Core::getSingletonPtr()->getOgreRenderWindow()->getHeight()),
			MyGUI::Align::Default, "Overlapped", "Background")->setImageTexture("BackgroundShadeBlue.png");

		this->widgets = MyGUI::LayoutManager::getInstancePtr()->loadLayout("ServerConfigurationState.layout");
		// http://www.ogre3d.org/addonforums/viewtopic.php?f=17&t=12934&p=72481&hilit=inputmanager+unloadLayout#p72481
		MyGUI::FloatPoint windowPosition;
		windowPosition.left = 0.1f;
		windowPosition.top = 0.1f;

		MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Window>("serverConfigurationPanel")->setRealPosition(windowPosition);

		MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("startServerSimulationButton")->eventMouseButtonClick += MyGUI::newDelegate(this, &ServerConfigurationStateMyGui::buttonHit);
		MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("startServerConsoleButton")->eventMouseButtonClick += MyGUI::newDelegate(this, &ServerConfigurationStateMyGui::buttonHit);
		MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("backButton")->eventMouseButtonClick += MyGUI::newDelegate(this, &ServerConfigurationStateMyGui::buttonHit);
		MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("serverNameButton")->eventMouseButtonClick += MyGUI::newDelegate(this, &ServerConfigurationStateMyGui::buttonHit);

		MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("serverNameEdit")->eventEditTextChange += MyGUI::newDelegate(this, &ServerConfigurationStateMyGui::editChange);
		this->mapsCombo = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::ComboBox>("mapsCombo");
		this->packetSendRateCombo = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::ComboBox>("packetSendRateCombo");
		this->areaOfInterestSlider = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::ScrollBar>("areaOfInterestSlider");

		this->areaOfInterestSlider->eventScrollChangePosition += MyGUI::newDelegate(this, &ServerConfigurationStateMyGui::sliderMoved);

		// fill with data
		this->populateOptions();
	}
	
	void ServerConfigurationStateMyGui::populateOptions(void)
	{
		this->packetSendRateCombo->addItem("5");
		this->packetSendRateCombo->addItem("10");
		this->packetSendRateCombo->addItem("15");
		this->packetSendRateCombo->addItem("20");
		this->packetSendRateCombo->addItem("25");
		this->packetSendRateCombo->addItem("30");
		this->packetSendRateCombo->addItem("35");
		this->packetSendRateCombo->addItem("40");
		this->packetSendRateCombo->addItem("45");
		this->packetSendRateCombo->addItem("50");
		this->packetSendRateCombo->addItem("55");
		this->packetSendRateCombo->addItem("60");

		this->packetSendRateCombo->setIndexSelected(this->packetSendRateCombo->findItemIndexWith(Ogre::StringConverter::toString(Core::getSingletonPtr()->getOptionPacketSendRate())));

		this->areaOfInterestSlider->setScrollPosition(Core::getSingletonPtr()->getOptionAreaOfInterest());
		if (areaOfInterestSlider->getScrollPosition() > 0)
		{
			MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("areaOfInterestLabel")->setCaptionWithReplacing("#{AreaOfInterest}: ("
				+ Ogre::StringConverter::toString(areaOfInterestSlider->getScrollPosition()) + " m)");
		}
		else
		{
			MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("areaOfInterestLabel")->setCaptionWithReplacing("#{AreaOfInterest}: (#{Off})");
		}

		std::map<Ogre::String, Ogre::String> sceneNames = AppStateManager::getSingletonPtr()->getRakNetModule()->parseScenes("Projects", Core::getSingletonPtr()->getResourcesName());
		//Karten hinzufuegen
		for (std::map<Ogre::String, Ogre::String>::iterator it = sceneNames.begin(); it != sceneNames.end(); ++it)
		{
			this->mapsCombo->addItem((*it).first);
		}
		//this->pMapMenu->setItems(strMapNames);
		if (false == sceneNames.empty())
		{
			Ogre::String selectedMap = Ogre::String(this->mapsCombo->getItemNameAt(0));
			AppStateManager::getSingletonPtr()->getRakNetModule()->setSceneName(selectedMap);

			//Ressourcegruppe zur virtuellen Umgebung finden
			std::map<Ogre::String, Ogre::String>::iterator it = sceneNames.find(selectedMap);

			AppStateManager::getSingletonPtr()->getRakNetModule()->setProjectName(it->second);

			//Ogre::LogManager::getSingletonPtr()->logMessage("------------------->" + this->pMapMenu->getSelectedItem() + ".scene");
			//Startpositionen aus der virtuellen Umgebung erhalten
			AppStateManager::getSingletonPtr()->getRakNetModule()->preParseScene("Projects", AppStateManager::getSingletonPtr()->getRakNetModule()->getProjectName(), AppStateManager::getSingletonPtr()->getRakNetModule()->getSceneName());

			MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("maxPlayerInfoLabel")->setCaptionWithReplacing("#{MaxPlayer}: "
				+ Ogre::StringConverter::toString(AppStateManager::getSingletonPtr()->getRakNetModule()->getAllowedPlayerCount()));
		}
	}

	void ServerConfigurationStateMyGui::buttonHit(MyGUI::Widget* _sender)
	{
		MyGUI::EditBox* serverNameEdit = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("serverNameEdit");
		if (0 == serverNameEdit->getCaption().size())
		{
			ENQUEUE_RENDER_COMMAND_WAIT("ServerConfigurationStateMyGui::buttonHit noServerName",
			{
				MyGUI::Message * messageBox = MyGUI::Message::createMessageBox("Server", MyGUI::LanguageManager::getInstancePtr()->replaceTags("#{NoServerName}"),
					MyGUI::MessageBoxStyle::IconWarning | MyGUI::MessageBoxStyle::Ok, "Popup", true);
			});
			return;
		}
		if ("startServerSimulationButton" == _sender->getName())
		{
			if (AppStateManager::getSingletonPtr()->getRakNetModule()->getAllowedPlayerCount() > 0)
			{
				AppStateManager::getSingletonPtr()->getRakNetModule()->setAreaOfInterest(static_cast<int>(this->areaOfInterestSlider->getScrollPosition()));

				Ogre::String packetSendRateSelected = Ogre::String(this->packetSendRateCombo->getItemNameAt(this->packetSendRateCombo->getIndexSelected()));

				AppStateManager::getSingletonPtr()->getRakNetModule()->setPacketSendRateMS(static_cast<int>(1000.0f / Ogre::StringConverter::parseReal(packetSendRateSelected)));
				Core::getSingletonPtr()->setOptionAreaOfInterest(static_cast<int>(this->areaOfInterestSlider->getScrollPosition()));
				Core::getSingletonPtr()->setOptionPacketSendRate(Ogre::StringConverter::parseInt(packetSendRateSelected));
				AppStateManager::getSingletonPtr()->getRakNetModule()->createRakNetForServer(serverNameEdit->getCaption());
				Core::getSingletonPtr()->saveCustomConfiguration();

				this->changeAppState(this->findByName(this->nextAppStateName));
			}
			else
			{
				ENQUEUE_RENDER_COMMAND_WAIT("ServerConfigurationStateMyGui::buttonHit noStartPosition",
				{
					MyGUI::Message * messageBox = MyGUI::Message::createMessageBox("Server", MyGUI::LanguageManager::getInstancePtr()->replaceTags("#{NoStartPosition}"),
						MyGUI::MessageBoxStyle::IconWarning | MyGUI::MessageBoxStyle::Ok, "Popup", true);
				});
			}

		}
		else if ("startServerConsoleButton" == _sender->getName())
		{
			if (AppStateManager::getSingletonPtr()->getRakNetModule()->getAllowedPlayerCount() > 0)
			{
				AppStateManager::getSingletonPtr()->getRakNetModule()->setServerInConsole(true);

				AppStateManager::getSingletonPtr()->getRakNetModule()->createRakNetForServer(serverNameEdit->getCaption());

				this->changeAppState(this->findByName(this->nextAppStateName));
			}
			else
			{
				ENQUEUE_RENDER_COMMAND_WAIT("ServerConfigurationStateMyGui::buttonHit noStartPosition2",
				{
					MyGUI::Message * messageBox = MyGUI::Message::createMessageBox("Server", MyGUI::LanguageManager::getInstancePtr()->replaceTags("#{NoStartPosition}"),
						MyGUI::MessageBoxStyle::IconWarning | MyGUI::MessageBoxStyle::Ok, "Popup", true);
				});
			}
		}
		else if ("serverNameButton" == _sender->getName())
		{
			ENQUEUE_RENDER_COMMAND_MULTI("ServerConfigurationStateMyGui::buttonHit serverNameButton", _1(serverNameEdit),
			{
				serverNameEdit->setTextShadow(true);
			});
			AppStateManager::getSingletonPtr()->getRakNetModule()->setServerName(serverNameEdit->getCaption());
		}
		else if ("backButton" == _sender->getName())
		{
			this->applyOptions();
			AppStateManager::getSingletonPtr()->getRakNetModule()->destroyContent();
			this->changeAppState(this->findByName("MainConfigurationStateMyGui"));
		}
	}

	void ServerConfigurationStateMyGui::itemSelected(MyGUI::ComboBox* _sender, size_t _index)
	{
		if (_sender == this->mapsCombo)
		{
			Ogre::String selectedMap = Ogre::String(this->mapsCombo->getItemNameAt(this->mapsCombo->getIndexSelected()));
			AppStateManager::getSingletonPtr()->getRakNetModule()->setSceneName(selectedMap);

			std::map<Ogre::String, Ogre::String>::iterator it = AppStateManager::getSingletonPtr()->getRakNetModule()->findProject(selectedMap);
			AppStateManager::getSingletonPtr()->getRakNetModule()->setProjectName(it->second);

			AppStateManager::getSingletonPtr()->getRakNetModule()->preParseScene("Projects", AppStateManager::getSingletonPtr()->getRakNetModule()->getProjectName() 
				+ "/" + AppStateManager::getSingletonPtr()->getRakNetModule()->getSceneName(), AppStateManager::getSingletonPtr()->getRakNetModule()->getSceneName() + ".scene");

			ENQUEUE_RENDER_COMMAND("ServerConfigurationStateMyGui::itemSelected",
			{
				MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("maxPlayerInfoLabel")->setCaptionWithReplacing("#{MaxPlayer}: "
					+ Ogre::StringConverter::toString(AppStateManager::getSingletonPtr()->getRakNetModule()->getAllowedPlayerCount()));
			});
		}
	}

	void ServerConfigurationStateMyGui::sliderMoved(MyGUI::ScrollBar* _sender, size_t _position)
	{
		if (_sender == this->areaOfInterestSlider)
		{
			ENQUEUE_RENDER_COMMAND("ServerConfigurationStateMyGui::sliderMoved",
			{
				if (areaOfInterestSlider->getScrollPosition() > 0)
				{
					MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("areaOfInterestLabel")->setCaptionWithReplacing("#{AreaOfInterest}: ("
						+ Ogre::StringConverter::toString(areaOfInterestSlider->getScrollPosition()) + " m)");
				}
				else
				{
					MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("areaOfInterestLabel")->setCaptionWithReplacing("#{AreaOfInterest}: (#{Off})");
				}
			});
		}
	}

	void ServerConfigurationStateMyGui::editChange(MyGUI::EditBox* _sender)
	{
		if ("serverNameEdit" == _sender->getName())
		{
			ENQUEUE_RENDER_COMMAND("ServerConfigurationStateMyGui::editChange",
			{
				MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("serverNameEdit")->setTextShadow(false);
			});
		}
	}

	void ServerConfigurationStateMyGui::createScene()
	{
	}

	bool ServerConfigurationStateMyGui::keyPressed(const OIS::KeyEvent& keyEventRef)
	{
		
		if (InputDeviceCore::getSingletonPtr()->getKeyboard()->isKeyDown(OIS::KC_ESCAPE))
		{
			this->applyOptions();
			AppStateManager::getSingletonPtr()->getRakNetModule()->destroyContent();

			this->changeAppState(this->findByName("MainConfigurationStateMyGui"));
			return true;
		}

		Core::getSingletonPtr()->keyPressed(keyEventRef);
		return true;
	}

	bool ServerConfigurationStateMyGui::keyReleased(const OIS::KeyEvent& keyEventRef)
	{
		Core::getSingletonPtr()->keyReleased(keyEventRef);
		return true;
	}

	bool ServerConfigurationStateMyGui::mouseMoved(const OIS::MouseEvent& evt)
	{
		NOWA::Core::getSingletonPtr()->mouseMoved(evt);
		return true;
	}

	bool ServerConfigurationStateMyGui::mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
	{
		NOWA::Core::getSingletonPtr()->mousePressed(evt, id);
		return true;
	}

	bool ServerConfigurationStateMyGui::mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
	{
		NOWA::Core::getSingletonPtr()->mouseReleased(evt, id);
		return true;
	}

	bool ServerConfigurationStateMyGui::axisMoved(const OIS::JoyStickEvent& evt, int axis)
	{
		return true;
	}

	bool ServerConfigurationStateMyGui::buttonPressed(const OIS::JoyStickEvent& evt, int button)
	{
		return true;
	}

	bool ServerConfigurationStateMyGui::buttonReleased(const OIS::JoyStickEvent& evt, int button)
	{
		return true;
	}

	void ServerConfigurationStateMyGui::update(Ogre::Real dt)
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

	void ServerConfigurationStateMyGui::applyOptions(void)
	{
		Ogre::String areaOfInterest = Ogre::StringConverter::toString(this->areaOfInterestSlider->getScrollPosition());

		Core::getSingletonPtr()->setOptionAreaOfInterest(static_cast<unsigned int>(this->areaOfInterestSlider->getScrollPosition()));
		Core::getSingletonPtr()->setOptionPacketSendRate(Ogre::StringConverter::parseInt(this->packetSendRateCombo->getItemNameAt(this->packetSendRateCombo->getIndexSelected())));

		// save options in XML
		Core::getSingletonPtr()->saveCustomConfiguration();
	}

}; // namespace end
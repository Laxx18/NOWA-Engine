#include "NOWAPrecompiled.h"
#include "ClientConfigurationStateMyGui.h"
#include "Core.h"
#include "InputDeviceCore.h"
#include "utilities/FaderProcess.h"
#include "main/AppStateManager.h"

namespace NOWA
{

	ClientConfigurationStateMyGui::ClientConfigurationStateMyGui()
		: AppState()
	{
		// do not init something here
	}

	void ClientConfigurationStateMyGui::enter(void)
	{
		NOWA::AppState::enter();

		this->playerColorCombo = nullptr;
		this->packetsPerSecondCombo = nullptr;
		this->interpolationRateCombo = nullptr;
		
		Ogre::String windowTitle = "NOWA [Client]";
		NOWA::Core::getSingletonPtr()->changeWindowTitle(windowTitle.c_str());

		this->interpolationFirstTime = true;

		ProcessManager::getInstance()->attachProcess(ProcessPtr(new FaderProcess(FaderProcess::FadeOperation::FADE_IN, 2.5f)));
		
		ENQUEUE_RENDER_COMMAND_WAIT("ClientConfigurationStateMyGui::enter",
		{
			this->sceneManager = Core::getSingletonPtr()->getOgreRoot()->createSceneManager(Ogre::ST_GENERIC, 1, "ClientConfigurationStateMyGui");
			// this->sceneManager->setAmbientLight(Ogre::ColourValue(0.7f, 0.7f, 0.7f));
			this->sceneManager->addRenderQueueListener(Core::getSingletonPtr()->getOverlaySystem());
			this->sceneManager->getRenderQueue()->setSortRenderQueue(Ogre::v1::OverlayManager::getSingleton().mDefaultRenderQueueId, Ogre::RenderQueue::StableSort);

			this->camera = this->sceneManager->createCamera("ClientConfigurationStateMyGui");
			this->camera->setPosition(Ogre::Vector3(0, 25, -50));
			this->camera->lookAt(Ogre::Vector3(0, 0, 0));
			this->camera->setNearClipDistance(1);
			//this->camera->setAspectRatio(Ogre::Real(Core::getSingletonPtr()->getOgreViewport()->getActualWidth()) /
			//Ogre::Real(Core::getSingletonPtr()->getOgreViewport()->getActualHeight()));
			this->camera->setAutoAspectRatio(true);

			WorkspaceModule::getInstance()->setPrimaryWorkspace(this->sceneManager, this->camera, nullptr);

			this->initializeModules(false, false);

			this->setupWidgets();

			this->createScene();
			if (!Core::getSingletonPtr()->isStartedAsServer())
			{
				this->createBackgroundMusic();
			}
		});
		

		AppStateManager::getSingletonPtr()->getRakNetModule()->createRakNetForClient();

		this->searchServer();
	}

	void ClientConfigurationStateMyGui::exit()
	{
		NOWA::AppState::exit();

		ProcessManager::getInstance()->attachProcess(ProcessPtr(new FaderProcess(FaderProcess::FadeOperation::FADE_OUT, 2.5f)));

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ClientConfigurationStateMyGui] Leaving...");
		
		ENQUEUE_RENDER_COMMAND_WAIT("ClientConfigurationStateMyGui::exit",
		{
			MyGUI::Gui::getInstancePtr()->destroyWidget(MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::ImageBox>("Background"));
			MyGUI::LayoutManager::getInstancePtr()->unloadLayout(this->widgets);
			this->playerColorCombo = nullptr;
			this->packetsPerSecondCombo = nullptr;
			this->interpolationRateCombo = nullptr;

			if (!Core::getSingletonPtr()->isStartedAsServer())
			{
				OgreALModule::getInstance()->deleteSound(this->sceneManager, this->menuMusic);
			}

			this->destroyModules();
		});
	}

	void ClientConfigurationStateMyGui::setupWidgets(void)
	{
		// Create Gui
		Core::getSingletonPtr()->setSceneManagerForMyGuiPlatform(this->sceneManager);

		MyGUI::Gui::getInstancePtr()->createWidget<MyGUI::ImageBox>("RotatingSkin",
			MyGUI::IntCoord(0, 0, Core::getSingletonPtr()->getOgreRenderWindow()->getWidth(), Core::getSingletonPtr()->getOgreRenderWindow()->getHeight()),
			MyGUI::Align::Default, "Overlapped", "Background")->setImageTexture("BackgroundShadeBlue.png");

		this->widgets = MyGUI::LayoutManager::getInstancePtr()->loadLayout("ClientConfigurationState.layout");
		// http://www.ogre3d.org/addonforums/viewtopic.php?f=17&t=12934&p=72481&hilit=inputmanager+unloadLayout#p72481
		MyGUI::FloatPoint windowPosition;
		windowPosition.left = 0.1f;
		windowPosition.top = 0.1f;

		MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Window>("clientConfigurationPanel")->setRealPosition(windowPosition);

		MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("playerNameButton")->eventMouseButtonClick += MyGUI::newDelegate(this, &ClientConfigurationStateMyGui::buttonHit);
		MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("searchServerButton")->eventMouseButtonClick += MyGUI::newDelegate(this, &ClientConfigurationStateMyGui::buttonHit);
		MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("enterIPButton")->eventMouseButtonClick += MyGUI::newDelegate(this, &ClientConfigurationStateMyGui::buttonHit);

		MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("connectToServerButton")->eventMouseButtonClick += MyGUI::newDelegate(this, &ClientConfigurationStateMyGui::buttonHit);
		MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("backButton")->eventMouseButtonClick += MyGUI::newDelegate(this, &ClientConfigurationStateMyGui::buttonHit);
		MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("startGameButton")->eventMouseButtonClick += MyGUI::newDelegate(this, &ClientConfigurationStateMyGui::buttonHit);

		this->playerColorCombo = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::ComboBox>("playerColorCombo");
		this->packetsPerSecondCombo = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::ComboBox>("packetsPerSecondCombo");
		this->interpolationRateCombo = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::ComboBox>("interpolationRateCombo");

		MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("playerNameEdit")->eventEditTextChange += MyGUI::newDelegate(this, &ClientConfigurationStateMyGui::editChange);
		MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("enterIPEdit")->eventEditTextChange += MyGUI::newDelegate(this, &ClientConfigurationStateMyGui::editChange);

		this->interpolationRateCombo->eventComboChangePosition += MyGUI::newDelegate(this, &ClientConfigurationStateMyGui::itemSelected);

		// fill with data
		this->populateOptions();
	}

	void ClientConfigurationStateMyGui::searchServer(void)
	{
		ENQUEUE_RENDER_COMMAND_WAIT("ClientConfigurationStateMyGui::searchServer",
		{
			bool serverFound = AppStateManager::getSingletonPtr()->getRakNetModule()->findServerAndCreateClient();
			if (serverFound)
			{
				MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("searchServerLabel")->setCaption(AppStateManager::getSingletonPtr()->getRakNetModule()->getServerName() + ":"
					+ AppStateManager::getSingletonPtr()->getRakNetModule()->getProjectName()
					+ "-" + AppStateManager::getSingletonPtr()->getRakNetModule()->getSceneName());

				MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("searchServerLabel")->setCaption("Server IP: " + AppStateManager::getSingletonPtr()->getRakNetModule()->getServerIP());
				MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("connectToServerButton")->setEnabled(true);
			}
			else
			{
				MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("searchServerLabel")->setCaptionWithReplacing("#{ServerNotFound}");
			}
		});
	}

	void ClientConfigurationStateMyGui::createBackgroundMusic(void)
	{
		OgreALModule::getInstance()->init(this->sceneManager);
		this->menuMusic = OgreALModule::getInstance()->createSound(this->sceneManager, "MenuMusic1", "kickstarter.ogg", true, true);
		if (this->menuMusic)
		{
			this->menuMusic->play();
		}
		else
		{
			OgreALModule::getInstance()->getSound(this->sceneManager, "MenuMusic1")->play();
		}
	}

	void ClientConfigurationStateMyGui::populateOptions(void)
	{
		ENQUEUE_RENDER_COMMAND_WAIT("ClientConfigurationStateMyGui::populateOptions",
		{
			size_t index = 0;

			//this->pInterpolationTimeMenu->addItem("16");
			this->interpolationRateCombo->addItem("33");
			this->interpolationRateCombo->addItem("50");
			this->interpolationRateCombo->addItem("75");
			this->interpolationRateCombo->addItem("100");
			this->interpolationRateCombo->addItem("150");
			this->interpolationRateCombo->addItem("200");
			this->interpolationRateCombo->addItem("250");
			this->interpolationRateCombo->addItem("500");

			this->interpolationRateCombo->setIndexSelected(this->interpolationRateCombo->findItemIndexWith(Ogre::StringConverter::toString(Core::getSingletonPtr()->getOptionInterpolationRate())));
			this->interpolationRateCombo->setCaption(Ogre::StringConverter::toString(Core::getSingletonPtr()->getOptionInterpolationRate()));

			this->packetsPerSecondCombo->addItem("16");
			this->packetsPerSecondCombo->addItem("33");
			this->packetsPerSecondCombo->addItem("50");
			this->packetsPerSecondCombo->addItem("100");
			this->packetsPerSecondCombo->setIndexSelected(this->packetsPerSecondCombo->findItemIndexWith(Ogre::StringConverter::toString(Core::getSingletonPtr()->getOptionPacketsPerSecond())));
			this->packetsPerSecondCombo->setCaption(Ogre::StringConverter::toString(Core::getSingletonPtr()->getOptionPacketsPerSecond()));

			this->playerColorCombo->addItem(MyGUI::LanguageManager::getInstancePtr()->replaceTags("#{Red}"));
			this->playerColorCombo->addItem(MyGUI::LanguageManager::getInstancePtr()->replaceTags("#{Green}"));
			this->playerColorCombo->addItem(MyGUI::LanguageManager::getInstancePtr()->replaceTags("#{Blue}"));
			this->playerColorCombo->addItem(MyGUI::LanguageManager::getInstancePtr()->replaceTags("#{Black}"));
			this->playerColorCombo->addItem(MyGUI::LanguageManager::getInstancePtr()->replaceTags("#{White}"));
			this->playerColorCombo->addItem(MyGUI::LanguageManager::getInstancePtr()->replaceTags("#{Yellow}"));
			this->playerColorCombo->addItem(MyGUI::LanguageManager::getInstancePtr()->replaceTags("#{Orange}"));

			this->playerColorCombo->setIndexSelected(Core::getSingletonPtr()->getOptionPlayerColor());
			this->playerColorCombo->setCaption(Ogre::String(this->playerColorCombo->getItemNameAt(this->playerColorCombo->getIndexSelected())));
		});
	}

	void ClientConfigurationStateMyGui::buttonHit(MyGUI::Widget* _sender)
	{
		if ("connectToServerButton" == _sender->getName())
		{
			ENQUEUE_RENDER_COMMAND_WAIT("ClientConfigurationStateMyGui::buttonHit connectToServerButton",
			{
				Ogre::String interpolationRateSelected = Ogre::String(this->interpolationRateCombo->getItemNameAt(this->interpolationRateCombo->getIndexSelected()));
				AppStateManager::getSingletonPtr()->getRakNetModule()->setInterpolationTimeMS(Ogre::StringConverter::parseInt(interpolationRateSelected, 33));
				//Core::getSingletonPtr()->packetSendRate = 1000 / this->packetSendRateSlider->getValue();
				Ogre::String packetsPerSecondSelected = Ogre::String(this->packetsPerSecondCombo->getItemNameAt(this->packetsPerSecondCombo->getIndexSelected()));
				// Attention, here if always 16
							AppStateManager::getSingletonPtr()->getRakNetModule()->setPacketSendRateMS(Ogre::StringConverter::parseInt(packetsPerSecondSelected, 16));

							AppStateManager::getSingletonPtr()->getRakNetModule()->startNetworking("Client");
							MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("startGameButton")->setEnabled(true);
			});
		}
		else if ("startGameButton" == _sender->getName())
		{
			ENQUEUE_RENDER_COMMAND_WAIT("ClientConfigurationStateMyGui::buttonHit startGameButton",
			{
				this->applyOptions();
				if (!AppStateManager::getSingletonPtr()->getRakNetModule()->isConnectionFailed())
				{
					this->changeAppState(this->findByName(this->nextAppStateName));
				}
				else
				{
					MyGUI::Message* messageBox = MyGUI::Message::createMessageBox("Client", MyGUI::LanguageManager::getInstancePtr()->replaceTags("#{ConnectionFailed}"),
						MyGUI::MessageBoxStyle::IconWarning | MyGUI::MessageBoxStyle::Ok, "Popup", true);
				}
			});
		}
		else if ("backButton" == _sender->getName())
		{
			this->applyOptions();
			this->disconnect();
			AppStateManager::getSingletonPtr()->getRakNetModule()->destroyContent();
			this->changeAppState(this->findByName("MainConfigurationStateMyGui"));
		}
		else if ("playerNameButton" == _sender->getName())
		{
			ENQUEUE_RENDER_COMMAND_WAIT("ClientConfigurationStateMyGui::buttonHit playerNameButton",
			{
				MyGUI::EditBox * playerNameEdit = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("playerNameEdit");
				if (playerNameEdit->getCaption().size() > 0)
				{
					playerNameEdit->setTextShadow(true);
					AppStateManager::getSingletonPtr()->getRakNetModule()->setPlayerName(playerNameEdit->getCaption());
				}
				else
				{
					MyGUI::Message* messageBox = MyGUI::Message::createMessageBox("Client", MyGUI::LanguageManager::getInstancePtr()->replaceTags("#{NoName}"),
						MyGUI::MessageBoxStyle::IconWarning | MyGUI::MessageBoxStyle::Ok, "Popup", true);
				}
			});
		}
		else if ("searchServerButton" == _sender->getName())
		{
			this->searchServer();
		}
		else if ("enterIPButton" == _sender->getName())
		{
			ENQUEUE_RENDER_COMMAND_WAIT("ClientConfigurationStateMyGui::buttonHit enterIPButton",
			{
				MyGUI::EditBox * enterIPEdit = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("enterIPEdit");
				MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("enterIPEdit")->setTextShadow(true);
				if (enterIPEdit->getCaption().size() > 0)
				{
					AppStateManager::getSingletonPtr()->getRakNetModule()->setServerIP(enterIPEdit->getCaption());
				}
				else
				{
					MyGUI::Message* messageBox = MyGUI::Message::createMessageBox("Client", MyGUI::LanguageManager::getInstancePtr()->replaceTags("#{EnterIP}"),
						MyGUI::MessageBoxStyle::IconWarning | MyGUI::MessageBoxStyle::Ok, "Popup", true);
				}
			});
		}
	}

	void ClientConfigurationStateMyGui::itemSelected(MyGUI::ComboBox* _sender, size_t _index)
	{
// Does work faulty
		if (_sender == this->interpolationRateCombo)
		{
			// Ogre::String interpolationRateSelected = Ogre::String(this->interpolationRateCombo->getItemNameAt(this->interpolationRateCombo->getIndexSelected()));
			// this->interpolationRateCombo->setCaptionWithReplacing(Ogre::StringConverter::toString(1000 / Ogre::StringConverter::parseInt(interpolationRateSelected)) + "#{PacketsPerSecond}");
		}
	}

	void ClientConfigurationStateMyGui::editChange(MyGUI::EditBox* _sender)
	{
		if ("playerNameEdit" == _sender->getName())
		{
			ENQUEUE_RENDER_COMMAND("ClientConfigurationStateMyGui::editChange playerNameEdit",
			{
				MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("playerNameEdit")->setTextShadow(false);
			});
		}
		else if ("enterIPEdit" == _sender->getName())
		{
			ENQUEUE_RENDER_COMMAND("ClientConfigurationStateMyGui::editChange enterIPEdit",
			{
				MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("enterIPEdit")->setTextShadow(false);
			});
		}
	}

	void ClientConfigurationStateMyGui::createScene()
	{

	}

	void ClientConfigurationStateMyGui::disconnect(void)
	{
		RakNet::BitStream bitstream;

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ClientConfigurationStateMyGui] exit for player Id: "
			+ Ogre::StringConverter::toString(AppStateManager::getSingletonPtr()->getRakNetModule()->getClientID()) + " to connection: " 
			+ Ogre::String(AppStateManager::getSingletonPtr()->getRakNetModule()->getServerAddress().ToString()));

		bitstream.Write((RakNet::MessageID)RakNetModule::ID_CLIENT_EXIT);
		
		// send data with high priority, reliable (TCP), 0, IP + Port, do not broadcast but just send to the client which attempted a connection
		AppStateManager::getSingletonPtr()->getRakNetModule()->getRakPeer()->Send(&bitstream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, AppStateManager::getSingletonPtr()->getRakNetModule()->getServerAddress(), false);
	}

	bool ClientConfigurationStateMyGui::keyPressed(const OIS::KeyEvent& keyEventRef)
	{
		
		if (InputDeviceCore::getSingletonPtr()->getKeyboard()->isKeyDown(OIS::KC_ESCAPE))
		{
			this->applyOptions();
			this->disconnect();
			AppStateManager::getSingletonPtr()->getRakNetModule()->destroyContent();
			this->changeAppState(this->findByName("MainConfigurationStateMyGui"));
			return true;
		}
		/*else if(Core::getSingletonPtr()->getKeyboard()->isKeyDown(OIS::KC_RETURN)) {
			//Paketsendrate berechnen
			AppStateManager::getSingletonPtr()->getRakNetModule()->setInterpolationTimeMS(Ogre::StringConverter::parseInt(this->interpolationTimeMenu->getSelectedItem(), 100.0f));
			//Core::getSingletonPtr()->packetSendRate = 1000 / this->packetSendRateSlider->getValue();
			AppStateManager::getSingletonPtr()->getRakNetModule()->setPacketSendRateMS(Ogre::StringConverter::parseInt(this->packetSendRateMenu->getSelectedItem(), 100.0f));
			//Audio loeschen
			OgreALModule::getInstance()->deleteSound(OgreALModule::getInstance()->getSound("MenuMusic1"));

			//Neuen Zustand beginnen
			this->changeAppState(this->findByName(this->nextAppStateName));
			}*/
		
		Core::getSingletonPtr()->keyPressed(keyEventRef);
		return true;
	}

	bool ClientConfigurationStateMyGui::keyReleased(const OIS::KeyEvent& keyEventRef)
	{
		Core::getSingletonPtr()->keyReleased(keyEventRef);
		return true;
	}

	//Mausbewegung an der GUI
	bool ClientConfigurationStateMyGui::mouseMoved(const OIS::MouseEvent& evt)
	{
		NOWA::Core::getSingletonPtr()->mouseMoved(evt);
		return true;
	}

	//Maustastendruck an der GUI
	bool ClientConfigurationStateMyGui::mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
	{
		NOWA::Core::getSingletonPtr()->mousePressed(evt, id);
		return true;
	}

	//loslassen der Maustasten an der GUI
	bool ClientConfigurationStateMyGui::mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
	{
		NOWA::Core::getSingletonPtr()->mouseReleased(evt, id);
		return true;
	}

	bool ClientConfigurationStateMyGui::axisMoved(const OIS::JoyStickEvent& evt, int axis)
	{
		return true;
	}

	bool ClientConfigurationStateMyGui::buttonPressed(const OIS::JoyStickEvent& evt, int button)
	{
		return true;
	}

	bool ClientConfigurationStateMyGui::buttonReleased(const OIS::JoyStickEvent& evt, int button)
	{
		return true;
	}

	void ClientConfigurationStateMyGui::update(Ogre::Real dt)
	{
		AppStateManager::getSingletonPtr()->getRakNetModule()->update(dt);
	}

	void ClientConfigurationStateMyGui::applyOptions(void)
	{
		Core::getSingletonPtr()->setOptionPlayerColor(static_cast<unsigned int>(this->playerColorCombo->getIndexSelected()));
		Core::getSingletonPtr()->setOptionInterpolationRate(Ogre::StringConverter::parseInt(this->interpolationRateCombo->getItemNameAt(this->interpolationRateCombo->getIndexSelected())));
		Core::getSingletonPtr()->setOptionPacketsPerSecond(Ogre::StringConverter::parseInt(this->packetsPerSecondCombo->getItemNameAt(this->packetsPerSecondCombo->getIndexSelected())));
		
		// save options in XML
		Core::getSingletonPtr()->saveCustomConfiguration();
	}

}; // namespace end
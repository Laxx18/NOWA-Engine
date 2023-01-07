#include "NOWAPrecompiled.h"
#include "Client.h"
#include <WTypes.h>
#include "main/Core.h"
#include "main/InputDeviceCore.h"
#include "main/AppStateManager.h"

namespace NOWA
{
	namespace NET
	{

		Client::Client(Ogre::SceneManager* sceneManager, OgreNewt::World* ogreNewt)
			: ConnectionType(sceneManager, ogreNewt),
			// chatTextboxActive(false),
			raySceneQuery(0)// ,
			// pSelectedCharacter(0)
		{

			//// this->pRaySceneQuery = this->sceneManager->createRayQuery(Ogre::Ray(), Core::getSingletonPtr()->PLAYERMASK);

			// here create Replica etc.

		}

		Client::~Client()
		{
			//Hier wird mit einer Liste mit replizierten Objekten gearbeitet, da wenn der Server ausgefaellt oder beendet wird, sendet er das alle Objekte geloescht werden sollen
			//Somit sind in dieser Liste dann keine mehr vorhanden, ansonsten gaebe es einen Fehler
			/*DataStructures::List<RakNet::Replica3*> replicaList;
			//Meinen lokalen Spieler erhalten
			AppStateManager::getSingletonPtr()->getRakNetModule()->appReplicaManager.GetReplicasCreatedByMe(replicaList);

			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Client has replicas created by him: "
				+ Ogre::StringConverter::toString(replicaList.Size()));
			
			if (0 < replicaList.Size())
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DistributedComponent] Client deletes all remote game object clients");
				//Broadcast an alle, dass der lokale Spieler auf den anderen Systemen (bekannt als entfernter Spieler) geloescht wird
				AppStateManager::getSingletonPtr()->getRakNetModule()->appReplicaManager.BroadcastDestructionList(replicaList, RakNet::UNASSIGNED_SYSTEM_ADDRESS);
			}*/

			//Alle entfernten Spieler beim Client loeschen
			/*for (unsigned int i = 0; i < Flubber::remoteFlubbers.Size(); i++)
			{
				Flubber* pCurrentFlubber =  Flubber::remoteFlubbers.Get(i);
				delete pCurrentFlubber;
				pCurrentFlubber = NULL;
			}*/
			AppStateManager::getSingletonPtr()->getRakNetModule()->remoteComponents.Clear();

			//Der vom Client erstellte Flubber wird geloescht indem, zunaechst eine Nachricht an alle Clients und den Server geschickt wird, das der Client dort geloescht werden soll
			//anschliessend wird der Zeiger geloescht
			/*NOWA::Ogre::LogManager::getSingletonPtr()->logMessage("myFlubber deleted: " + Ogre::StringConverter::toString(Flubber::pMyFlubber->getClientID()));
			if (Flubber::pMyFlubber)
			{
				NOWA::Ogre::LogManager::getSingletonPtr()->logMessage("hier wird myFlubber deleted");
				NOWA::Ogre::LogManager::getSingletonPtr()->logMessage("hier wird myFlubber deleted");
				delete Flubber::pMyFlubber;
			}*/
			AppStateManager::getSingletonPtr()->getRakNetModule()->appReplicaManager.Clear();
		}

		void Client::setupWidgets(void)
		{
			////Textfeld zum Chatten erstellen
			//this->chatMessageTextBox = NOWA::Core::getSingletonPtr()->getTrayManager()->createTextBox(OgreBites::TL_TOPLEFT, "ChatMessageTextBox", "", NOWA::Core::getSingletonPtr()->getOgreRenderWindow()->getWidth() - 20, 80);
			//this->chatMessageTextBox->hide();
			////Label erstellen um erhaltene Nachrichten anzuzeigen
			//this->chatMessageLabel = NOWA::Core::getSingletonPtr()->getTrayManager()->createLabel(OgreBites::TL_TOPLEFT, "ChatMessageLabel", "", NOWA::Core::getSingletonPtr()->getOgreRenderWindow()->getWidth() - 20);
			//this->chatMessageLabel->hide();
			////Label erstellen um die Latenz zu sehen
			//this->latencyLabel = NOWA::Core::getSingletonPtr()->getTrayManager()->createLabel(OgreBites::TL_BOTTOMLEFT, "LatencyLabel", "", 150);
			//while (NOWA::Core::getSingletonPtr()->getTrayManager()->getTrayContainer(OgreBites::TL_TOPLEFT)->isVisible())
			//	NOWA::Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(OgreBites::TL_TOPLEFT, 0, OgreBites::TL_NONE);
			//////Logo und fps Anzeige usw. verstecken, ueber die Taste O kann sie wieder angezeigt werden
			////NOWA::Core::getSingletonPtr()->getTrayManager()->hideLogo();
			////NOWA::Core::getSingletonPtr()->getTrayManager()->hideFrameStats();
			//////NOWA::Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(OgreBites::TL_BOTTOMLEFT, 0, OgreBites::TL_NONE);
		}

		void Client::showStatus(bool bShow)
		{
			//if (bShow) {
			//	//Steuerelemente zum Chatten anzeigen
			//	this->chatMessageLabel->show();
			//	NOWA::Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->chatMessageLabel, OgreBites::TL_TOPLEFT);
			//	this->chatMessageTextBox->show();
			//	NOWA::Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->chatMessageTextBox, OgreBites::TL_TOPLEFT);

			//}
			//else {
			//	//Steuerelemente zum Chatten verstecken
			//	while (NOWA::Core::getSingletonPtr()->getTrayManager()->getTrayContainer(OgreBites::TL_TOPLEFT)->isVisible()) {
			//		NOWA::Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(OgreBites::TL_TOPLEFT, 0, OgreBites::TL_NONE);
			//	}
			//	this->chatMessageTextBox->hide();
			//	this->chatMessageLabel->hide();
			//}
		}

		/*//Chat zunaechst zum Server senden
		void Client::sendChatMessage(Character* pSelectedCharacter)
		{
			//Nur senden, wenn ein Spieler selektiert  und eine Nachricht verfasst wurde
			if (pSelectedCharacter && this->chatMessageTextBox->getText().size() > 0)
			{
				RakNet::BitStream bitstream;
				//Protokoll zum Chatten
				bitstream.Write((RakNet::MessageID)RakNetModule::ID_CHAT_MESSAGE_PACKET);
				//Chatnachricht verpacken
				RakNet::StringTable::Instance()->EncodeString(Ogre::String(this->chatMessageTextBox->getText()).c_str(), 255, &bitstream);

				//Adresse vom Sender und Empfaenger mitsenden
				RakNet::StringTable::Instance()->EncodeString(pSelectedFlubber->getStrSystemAddress().c_str(), 255, &bitstream); //Empfaenger
				bitstream.Write(Flubber::pMyFlubber->getClientID()); //Sender

				//NOWA::Ogre::LogManager::getSingletonPtr()->logMessage("chat senden an: " + Ogre::String(pSelectedFlubber->getStrSystemAddress()));

				//RakNet::SystemAddress systemAddress = pSelectedFlubber->getStrSystemAddress().c_str();
				//Direkt an Peer senden geht leider nicht, da die Clients nur ueber den Server verbunden sind
				NOWA::Core::getSingletonPtr()->rakpeer->Send(&bitstream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, NOWA::Core::getSingletonPtr()->serverAddress, false);
				//Spieler deselektieren
				pSelectedFlubber->getNode()->showBoundingBox(false);
				pSelectedFlubber = NULL;
			}
		}*/

		//Chatnachricht vom Server empfangen
		void Client::readChatMessage(RakNet::Packet* packet)
		{
			/*char message[255];
			char strSystemAddressReceiver[255];
			char senderID;

			RakNet::BitStream bitstream(packet->data, packet->length, false); //false: Daten werden nicht kopiert, sondern orginal genommen
			bitstream.IgnoreBytes(sizeof(RakNet::MessageID));
			RakNet::StringTable::Instance()->DecodeString(message, 255, &bitstream);
			RakNet::StringTable::Instance()->DecodeString(strSystemAddressReceiver, 255, &bitstream);
			bitstream.Read(senderID);

			//NOWA::Ogre::LogManager::getSingletonPtr()->logMessage("chat angekommen: " + Ogre::String(strSystemAddressReceiver));

			Ogre::String strMessage = Ogre::String(message);
			//Flubber* Sender = Flubber::getFlubberByStrAddress(Ogre::String(strSystemAddressReceiver));
			//Spieler suchen, der die Nachricht erhalten soll
			Flubber* pSender = NULL;
			for (unsigned int i = 0; i < Flubber::remoteFlubbers.Size(); i++)
			{
				if(Flubber::remoteFlubbers[i]->getClientID() == senderID)
				{
					pSender = Flubber::remoteFlubbers[i];
					break;
				}
			}

			//Wenn der Sender gefunden wurde, wird die Nachricht mit Namen vom Sender beim Empfaenger im Textfeld angezeigt
			if (pSender)
			{
				Ogre::String strChatMessage = pSender->getName() + Ogre::StringConverter::toString(senderID) + ": " + strMessage;
				this->chatMessageLabel->setCaption(strChatMessage);
			}*/
		}

		//Nur Mein Spieler bekommt die Daten wann sich ein entfernter Spieler bewegt hat
		void Client::simulateMovedGameEventFromServer(RakNet::Packet* packet)
		{
			/*sGamestate gamestateFromServer;
			unsigned char playerID;
			RakNet::BitStream bitstream(packet->data, packet->length, false); //false: Daten werden nicht kopiert, sondern orginal genommen
			bitstream.IgnoreBytes(sizeof(RakNet::MessageID));
			bitstream.Read(gamestateFromServer.move);
			bitstream.Read(playerID);

			for (unsigned int i = 0; i < Flubber::remoteFlubbers.Size(); i++)
			{
				if (Flubber::remoteFlubbers[i]->getClientID() == playerID)
				{
					//Von mir als entfernter Spieler fuer einen Klient werden alle Spielzustaende ausgefuehrt
					Flubber* CurrentFlubber = Flubber::remoteFlubbers[i];
					//Spielzustand aktualisieren
					pCurrentFlubber->gamestate.move = gamestateFromServer.move;
					//Bewegung starten
					pCurrentFlubber->applyMovement(gamestateFromServer.move);
				}
			}*/
		}

		void Client::simulateJumpedGameEventFromServer(RakNet::Packet *packet)
		{
			/*sGamestate gamestateFromServer;
			unsigned char playerID;
			RakNet::BitStream bitstream(packet->data, packet->length, false); //false: Daten werden nicht kopiert, sondern orginal genommen
			bitstream.IgnoreBytes(sizeof(RakNet::MessageID));
			bitstream.Read(gamestateFromServer.jump);
			bitstream.Read(playerID);

			for (unsigned int i = 0; i < Flubber::remoteFlubbers.Size(); i++)
			{
				if (Flubber::remoteFlubbers[i]->getClientID() == playerID)
				{
					//Von mir als entfernter Spieler fuer einen Klient werden alle Spielzustaende ausgefuehrt
					Flubber* CurrentFlubber = Flubber::remoteFlubbers[i];
					//Spielzustand aktualisieren
					pCurrentFlubber->gamestate.jump = gamestateFromServer.jump;
					//Sprung starten
					pCurrentFlubber->applyJump(gamestateFromServer.jump);
				}
			}*/
		}

		void Client::simulateShotGameEventFromServer(RakNet::Packet *packet)
		{
			/*sGamestate gamestateFromServer;
			unsigned char playerID;
			RakNet::BitStream bitstream(packet->data, packet->length, false); //false: Daten werden nicht kopiert, sondern orginal genommen
			bitstream.IgnoreBytes(sizeof(RakNet::MessageID));
			bitstream.Read(gamestateFromServer.shoot);
			bitstream.Read(gamestateFromServer.bulletDirection.x);
			bitstream.Read(gamestateFromServer.bulletDirection.y);
			bitstream.Read(gamestateFromServer.bulletDirection.z);
			bitstream.Read(playerID);

			for (unsigned int i = 0; i < Flubber::remoteFlubbers.Size(); i++)
			{
				if (Flubber::remoteFlubbers[i]->getClientID() == playerID)
				{
					//NOWA::Ogre::LogManager::getSingletonPtr()->logMessage("flubber shoot and not hit mit index " + Ogre::StringConverter::toString(i) + " mit playerid: " + Ogre::StringConverter::toString(Flubber::remoteFlubbers[i]->getClientID()) + " shoot: " + Ogre::StringConverter::toString(gamestateFromServer.shoot));
					//Von mir als entfernter Spieler fuer einen Klient werden alle Spielzustaende ausgefuehrt
					Flubber* CurrentFlubber = Flubber::remoteFlubbers[i];
					//Spielzustaende aktualisieren
					pCurrentFlubber->gamestate.shoot = gamestateFromServer.shoot;
					pCurrentFlubber->gamestate.bulletDirection = gamestateFromServer.bulletDirection;
					//Schuss starten
					pCurrentFlubber->applyShoot(gamestateFromServer.shoot);
				}
			}*/
		}

		void Client::simulateShotHitGameEventFromServer(RakNet::Packet *packet)
		{
			/*sGamestate gamestateFromServer;
			unsigned char playerID;
			unsigned char hitPlayerID;
			int hitPlayerEnergy;
			RakNet::BitStream bitstream(packet->data, packet->length, false); //false: Daten werden nicht kopiert, sondern orginal genommen
			bitstream.IgnoreBytes(sizeof(RakNet::MessageID));
			bitstream.Read(hitPlayerEnergy);
			bitstream.Read(hitPlayerID);
			bitstream.Read(gamestateFromServer.shoot);
			bitstream.Read(gamestateFromServer.bulletDirection.x);
			bitstream.Read(gamestateFromServer.bulletDirection.y);
			bitstream.Read(gamestateFromServer.bulletDirection.z);
			bitstream.Read(playerID);

			//NOWA::Ogre::LogManager::getSingletonPtr()->logMessage("remoteflubber " + Ogre::StringConverter::toString(playerID) + " shoot and hit: " + Ogre::StringConverter::toString(hitPlayerID));

			//Es koennen 3 Faelle eintreten:
			//1. Ich werde von einem anderen Spieler getroffen
			//2. Ein Spieler beobachtet wie ein anderer Spieler schießt
			//2. Der lokale Spieler schießt und trifft einen anderen entfernten Spieler

			//Fall 1:
			if (Flubber::pMyFlubber->getClientID() == hitPlayerID)
			{
				Flubber::pMyFlubber->updateDamage(hitPlayerEnergy);
			}

			for (unsigned int i = 0; i < Flubber::remoteFlubbers.Size(); i++)
			{
				//Fall 2:
				if (Flubber::remoteFlubbers[i]->getClientID() == playerID)
				{
					//Von mir als entfernter Spieler fuer einen Klient werden alle Spielzustaende ausgefuehrt
					Flubber* CurrentFlubber = Flubber::remoteFlubbers[i];
					pCurrentFlubber->gamestate.shoot = gamestateFromServer.shoot;
					pCurrentFlubber->gamestate.bulletDirection = gamestateFromServer.bulletDirection;
					pCurrentFlubber->applyShoot(gamestateFromServer.shoot);
				}
				//Fall 3:
				if (Flubber::remoteFlubbers[i]->getClientID() == hitPlayerID)
				{
					Flubber *pCurrentFlubber = Flubber::remoteFlubbers[i];
					pCurrentFlubber->updateDamage(hitPlayerEnergy);
				}
			}*/
		}

		//Staendig versendete Spielzustaende vom Server erhalten
		void Client::simulateGamestateFromServer(RakNet::Packet* acket, Ogre::Real dt)
		{
			/*Ogre::Vector3 positionFromServer;
			Ogre::Quaternion orientationFromServer;
			sGamestate gamestateFromServer;
			unsigned char playerID;

			RakNet::BitStream bitstream(packet->data, packet->length, false); //false: Daten werden nicht kopiert, sondern orginal genommen
			bitstream.IgnoreBytes(sizeof(RakNet::MessageID));
			bitstream.Read(gamestateFromServer.timeStamp);
			bitstream.IgnoreBytes(sizeof(RakNet::MessageID));
			//finale Position kommt hinzu
			bitstream.Read(positionFromServer.x);
			bitstream.Read(positionFromServer.y);
			bitstream.Read(positionFromServer.z);
			//finale Orientierung kommt hinzu
			bitstream.Read(orientationFromServer.w);
			bitstream.Read(orientationFromServer.x);
			bitstream.Read(orientationFromServer.y);
			bitstream.Read(orientationFromServer.z);
			bitstream.Read(gamestateFromServer.height);
			bitstream.Read(playerID);


			//NOWA::Ogre::LogManager::getSingletonPtr()->logMessage("korrektur id: " + Ogre::StringConverter::toString(playerID));
			if (Flubber::pMyFlubber)
			{
				//Wenn es sich um mich handelt, bekomme ich die Korrektur
				if (Flubber::pMyFlubber->getClientID() == playerID)
				{
					//Vorher muss getestet werden, ob der Zeitstempel groesser ist als die jetzige Zeit, da es sich um unsigned handelt, wuerde man
					//ohne zu testen einfach abziehen: z.b. 23424534 - 23424535, dann kaeme bei signed -1 heraus, aber bei unsigned: 4294967394, da unsigned long 4 byte hat und es einen Ueberlauf gibt, somit wird vom Maximum 1 abgezogen
					//da unsigned nicht Minus werden kann!
					if (RakNet::GetTime() > gamestateFromServer.timeStamp + (unsigned long)(dt*1000.0f))
						Flubber::pMyFlubber->latencyToServer = ((unsigned long)(RakNet::GetTime() - gamestateFromServer.timeStamp - (unsigned long)(dt*1000.0f)));
					else
						Flubber::pMyFlubber->latencyToServer = 0;

					this->latencyLabel->setCaption("Latenz: " + Ogre::StringConverter::toString(Flubber::pMyFlubber->highestLatencyToServer) + "ms");

					//vom Server erhaltenen Spielzustand in History fuer den lokalen Spieler einfuegen
					Flubber::pMyFlubber->entityStateHistory.enqeue(positionFromServer, orientationFromServer, RakNet::GetTimeMS());
				}
				else
				{
					//Ansonsten bekommt der entfernte Client die Korrektur vom Server ueber mich
					//In dieser Liste sind beim Server alle Spieler enthalten. Beim Client alle entfernten, ausser er selbst
					for (unsigned int i = 0; i < Flubber::remoteFlubbers.Size(); i++)
					{
						if (Flubber::remoteFlubbers[i]->getClientID() == playerID)
						{
							//NOWA::Ogre::LogManager::getSingletonPtr()->logMessage("entfernte id: " + Ogre::StringConverter::toString((unsigned long)playerID));
							//Von mir als entfernter Spieler fuer einen Klient werden alle Spielzustaende ausgefuehrt
							Flubber* CurrentFlubber = Flubber::remoteFlubbers[i];
							//pCurrentFlubber->gamestate = gamestateFromServer;

							pCurrentFlubber->gamestate.useTimeStamp = gamestateFromServer.useTimeStamp;
							pCurrentFlubber->gamestate.timeStamp = gamestateFromServer.timeStamp;
							pCurrentFlubber->gamestate.height = gamestateFromServer.height;

							//NOWA::Ogre::LogManager::getSingletonPtr()->logMessage("id: " + Ogre::StringConverter::toString(i) + " latencyToRemotePlayer: " + Ogre::StringConverter::toString(pCurrentFlubber->latencyToRemotePlayer));
							if (RakNet::GetTime() > gamestateFromServer.timeStamp + (unsigned long)(dt*1000.0f))
								pCurrentFlubber->latencyToServer = (unsigned long)(RakNet::GetTime() - gamestateFromServer.timeStamp - (unsigned long)(dt*1000.0f));
							else
								pCurrentFlubber->latencyToServer = 0;

							//vom Server erhaltenen Spielzustand in History fuer den entfernten Spieler einfuegen
							pCurrentFlubber->entityStateHistory.enqeue(positionFromServer, orientationFromServer, RakNet::GetTimeMS());
						}
					}
				}
			}*/
		}

		//void Client::updateGamestate(Ogre::Real dt)
		void Client::updateGamestatesOfRemotePlayers(Ogre::Real dt)
		{
			/*//Beim Client sind in dieser Liste alle entfernten, ausser der lokale Spieler enthalten
			for (unsigned int i = 0; i < Flubber::remoteFlubbers.Size(); i++)
			{
				Flubber::remoteFlubbers[i]->update(dt);
			}*/
		}

		/*void Client::getPlayerInformation(RakNet::Packet* packet)
		{
			RakNet::BitStream bitstream(packet->data, packet->length, false);

			//ID_GAME_MESSAGE_1 ignorieren
			//Wer ist welcher Spieler
			bitstream.IgnoreBytes(sizeof(RakNet::MessageID));

			int playerID = -1;
			unsigned int areaOfInterest = 0;


			//Jeder Client erhaelt vom Server eine Idee, wenn eine Verbindung erfolgreich war! Diese ID ist einzigartig
			bitstream.Read(playerID);
			bitstream.Read(areaOfInterest);

			AppStateManager::getSingletonPtr()->getRakNetModule()->setClientID(playerID);
			AppStateManager::getSingletonPtr()->getRakNetModule()->setAreaOfInterest(areaOfInterest);

			Ogre::Vector3 startPosition;
			Ogre::Quaternion startOrientation;
			bitstream.ReadVector(startPosition.x, startPosition.y, startPosition.z);
			bitstream.ReadNormQuat(startOrientation.w, startOrientation.x, startOrientation.y, startOrientation.z);

			char strSystemAddress[255];
			RakNet::StringTable::Instance()->DecodeString(strSystemAddress, 255, &bitstream);

			char strID[4];
			sprintf(strID, "%d", AppStateManager::getSingletonPtr()->getRakNetModule()->getClientID());
			Ogre::String strTitle = "Client_"  + Ogre::String(strID) + "_" + AppStateManager::getSingletonPtr()->getRakNetModule()->getPlayerName();
			//Fensertitel aendern, damit jeder Spieler sieht welche ID er hat
			NOWA::Core::getSingletonPtr()->changeWindowTitle(strTitle.c_str());

			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[Client] getting player ID: "
				+ Ogre::StringConverter::toString(AppStateManager::getSingletonPtr()->getRakNetModule()->getClientID()) + " assigned from server");

			int desktopWidth;
			int desktopHeight;
			Core::getSingletonPtr()->getDesktopSize(desktopWidth, desktopHeight);

			if (AppStateManager::getSingletonPtr()->getRakNetModule()->getClientID() == 0)
				Core::getSingletonPtr()->setWindowPosition(0, 0);
			else if (AppStateManager::getSingletonPtr()->getRakNetModule()->getClientID() == 1)
				Core::getSingletonPtr()->setWindowPosition(desktopWidth - NOWA::Core::getSingletonPtr()->getOgreRenderWindow()->getWidth(), 0);
			else if (AppStateManager::getSingletonPtr()->getRakNetModule()->getClientID() == 2)
				Core::getSingletonPtr()->setWindowPosition( 0, desktopHeight - NOWA::Core::getSingletonPtr()->getOgreRenderWindow()->getHeight());

			//Hier wird der eigene Flubber erstellt wenn die Verbindung zum Server erfolgreich war
			//Die Funktion darf auch erst hier ausgefuehrt werden, da vorher der Fenstertitel und noch wichtiger der Debuglog erstellt werden!!

			// here everything with character which is derived from NetworkObject
			// NOWA::Ogre::LogManager::getSingletonPtr()->logMessage("MyFlubber erstellen");
			//Flubber::createMyFlubber(&ConnectionType::flubberReplicaManager, startPosition, startOrientation);
			////PlayerID muss hier gesetzt werden, damit sie an alle anderen Spieler weitergesendet werden kann
			//Flubber::pMyFlubber->setClientID(NOWA::Core::getSingletonPtr()->playerID);
			////Namen setzen
			//Flubber::pMyFlubber->setObjectTitle();
			////NOWA::Ogre::LogManager::getSingletonPtr()->logMessage("myFlubber hat: " + Ogre::StringConverter::toString(Flubber::pMyFlubber->getClientID()));
			////NOWA::Ogre::LogManager::getSingletonPtr()->logMessage("jetzt playerid: " + Ogre::StringConverter::toString(Flubber::pMyFlubber->getClientID()));
			//Flubber::pMyFlubber->setStrSystemAddress(strSystemAddress);
			//NOWA::Ogre::LogManager::getSingletonPtr()->logMessage("meine Adresse: " + Ogre::String(Flubber::pMyFlubber->getStrSystemAddress()));

			////Niedrigsten Ping zum Server erhalten

			////RakNet stellt keine Funktion bereit um direkt den schlechtesten Ping zu erhalten, dieser kann aber durch: (Mittelwert * 2) - kleinster Wert berechnet werden, und das Ganze durch 2 geteilt ergibt dann die Latenz
			//NOWA::Core::getSingletonPtr()->highestLatencyToServer = ((NOWA::Core::getSingletonPtr()->rakpeer->GetAveragePing(packet->systemAddress) * 2) - NOWA::Core::getSingletonPtr()->rakpeer->GetLowestPing(packet->systemAddress)) / 2;
			//Flubber::pMyFlubber->highestLatencyToServer = NOWA::Core::getSingletonPtr()->highestLatencyToServer;
			//NOWA::Ogre::LogManager::getSingletonPtr()->logMessage("Client mit Adresse: " + Flubber::pMyFlubber->getStrSystemAddress() + " hat Latenz: " + Ogre::StringConverter::toString(Flubber::pMyFlubber->highestLatencyToServer));
			////EntitystateHistory hier erst initalisieren, da jetzt erst die hoechste Latenz zum Server feststeht
			//Flubber::pMyFlubber->initEntityStateHistory();
		}*/

		void  Client::sendClientReadyForReplication(void)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[Client] Sending ID_CLIENT_READY to server: " + Ogre::String(AppStateManager::getSingletonPtr()->getRakNetModule()->getServerAddress().ToString()));
			RakNet::BitStream bitstream;
			bitstream.Write((RakNet::MessageID)RakNetModule::ID_CLIENT_READY);
			AppStateManager::getSingletonPtr()->getRakNetModule()->getRakPeer()->Send(&bitstream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, AppStateManager::getSingletonPtr()->getRakNetModule()->getServerAddress(), false);
		}

		void Client::updatePackets(Ogre::Real dt)
		{
			RakNet::Packet* packet;
			for (packet = AppStateManager::getSingletonPtr()->getRakNetModule()->getRakPeer()->Receive(); packet;
			AppStateManager::getSingletonPtr()->getRakNetModule()->getRakPeer()->DeallocatePacket(packet), packet = AppStateManager::getSingletonPtr()->getRakNetModule()->getRakPeer()->Receive())
			{
				// Bei dem ersten Byte handelt es sich um die Paket ID (Nachrichten-ID)
				unsigned char packetID = AppStateManager::getSingletonPtr()->getRakNetModule()->getPacketIdentifier(packet);
				// Ogre::LogManager::getSingletonPtr()->logMessage("packetID: " + Ogre::StringConverter::toString(packetID));
				switch (packetID)
				{
				case ID_CONNECTION_ATTEMPT_FAILED:
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[Client] ID_CONNECTION_ATTEMPT_FAILED to server");
					AppStateManager::getSingletonPtr()->getRakNetModule()->setConnectionFailed(true);
					break;
				case ID_NO_FREE_INCOMING_CONNECTIONS:
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[Client] ID_NO_FREE_INCOMING_CONNECTIONS to server");
					AppStateManager::getSingletonPtr()->getRakNetModule()->setConnectionFailed(true);
					break;
					/*case ID_CONNECTION_REQUEST_ACCEPTED:
						Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[Client] ID_CONNECTION_REQUEST_ACCEPTED to server");
						AppStateManager::getSingletonPtr()->getRakNetModule()->setConnectionFailed(false);
						break;*/
				case ID_DISCONNECTION_NOTIFICATION:
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[Client] ID_DISCONNECTION_NOTIFICATION to server");
					AppStateManager::getSingletonPtr()->getRakNetModule()->setConnectionFailed(true);
					break;
					/*case ID_NEW_INCOMING_CONNECTION:
						Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[Client] ID_NEW_INCOMING_CONNECTION from " + Ogre::String(packet->systemAddress.ToString()));
						break;*/
				case ID_CONNECTION_LOST:
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[Client] ID_CONNECTION_LOST to server");
					AppStateManager::getSingletonPtr()->getRakNetModule()->setConnectionFailed(true);
					// here: AppStateManager::getSingletonPtr()->getRakNetModule()->destroy() -> test it
					break;
				case RakNetModule::ID_REMOTE_CLIENT_EXIT:
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[Client] ID_REMOTE_CLIENT_EXIT from server");
					RakNet::BitStream bitstream(packet->data, packet->length, false);
					// ignore ID_GAME_MESSAGE_1
					bitstream.IgnoreBytes(sizeof(RakNet::MessageID));

					int clientID = -1;
					char strSystemAddressOtherClient[255];
					// each client receives an unique player id from the server
					bitstream.Read(clientID);
					RakNet::StringTable::Instance()->DecodeString(strSystemAddressOtherClient, 255, &bitstream);
					// remove the other client from the list
					AppStateManager::getSingletonPtr()->getRakNetModule()->getClients().erase(clientID);
					break;
				}
				case RakNetModule::ID_CLIENT_START_REPLICATION:
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[Client] got ID_CLIENT_START_REPLICATION from " + Ogre::String(packet->systemAddress.ToString()));
					// start the replication progress
					AppStateManager::getSingletonPtr()->getRakNetModule()->appReplicaManager.PushConnection(AppStateManager::getSingletonPtr()->getRakNetModule()->appReplicaManager.AllocConnection(packet->systemAddress, packet->guid));
					break;
				}
					/*case RakNetModule::ID_PLAYERASSIGNMENT_PACKET:
						//Spieler ID usw. an das Paket senden, das die Anfrage an der Server gemacht hatte
						this->getPlayerInformation(packet);
						break;
					case RakNetModule::ID_NO_FREE_SPACE:
						//Wenn kein freier Platz mehr existiert, kann sich der Client nicht verbinden
						Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[Client] ID_NO_FREE_SPACE in virtual environment");
						AppStateManager::getSingletonPtr()->getRakNetModule()->setConnectionFailed(true);
						break;*/
						//case RakNetModule::ID_SERVER_MOVED_REMOTEPLAYERS_RESULT_PACKET:
						//	//Erhaltenen Spielzustand vom Server simulieren
						//	this->simulateGamestateFromServer(packet, dt);
						//	break;
						//case RakNetModule::ID_SERVER_MOVEDGAMEEVENT_PACKET:
						//	//Finale Position und Orientierung usw. vom Server erhalten und in die History abspeichern
						//	this->simulateMovedGameEventFromServer(packet);
						//	break;
						//case RakNetModule::ID_SERVER_JUMPEDGAMEEVENT_PACKET:
						//	//Protokoll erhalten damit ein Spieler springt
						//	this->simulateJumpedGameEventFromServer(packet);
						//	break;
						//case RakNetModule::ID_SERVER_SHOTGAMEEVENT_PACKET:
						//	//Protokoll erhalten damit ein Spieler schießt
						//	this->simulateShotGameEventFromServer(packet);
						//	break;
						//case RakNetModule::ID_SERVER_SHOTHITGAMEEVENT_PACKET:
						//	//Protokoll erhalten, dass ein Spieler von einem Schuss getroffen wurde
						//	this->simulateShotHitGameEventFromServer(packet);
						//	break;
						//case RakNetModule::ID_CHAT_MESSAGE_PACKET:
						//	//Chat-Nachricht
						//	this->readChatMessage(packet);
						//	break;
						//	//Funktioniert leider nicht
				case ID_SND_RECEIPT_ACKED:
				case ID_SND_RECEIPT_LOSS:
					{
						// When using UNRELIABLE_WITH_ACK_RECEIPT, the system tracks which variables were updated with which sends
						// So it is then necessary to inform the system of messages arriving or lost
						// Lost messages will flag each variable sent in that update as dirty, meaning the next Serialize() call will resend them with the current values
						uint32_t msgNumber;
						memcpy(&msgNumber, packet->data + 1, 4);

						DataStructures::List<RakNet::Replica3*> replicaList;
						AppStateManager::getSingletonPtr()->getRakNetModule()->appReplicaManager.GetReplicasCreatedByMe(replicaList);
						unsigned int idx;
						for (idx = 0; idx < replicaList.Size(); idx++)
						{
							dynamic_cast<DistributedComponent*>(replicaList[idx])->NotifyReplicaOfMessageDeliveryStatus(packet->guid, msgNumber, packet->data[0] == ID_SND_RECEIPT_ACKED);
						}
						Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[Client] ID_SND_RECEIPT_LOSS from Server");
						////this->NotifyReplicaOfMessageDeliveryStatus(packet);
						break;
					}
				}
			}
			//muss 0 sein, sonst geht nichts
			// RakSleep(0);
		}

		//Zu Analysezwecken eine Statistik in Excel herausschreiben
		void Client::writeStatistics(void)
		{
			/*NOWA::Core::getSingletonPtr()->rakpeer->AttachPlugin(&this->messageHandler);
			this->messageHandler.StartLog("ClientStatistics.txt");
			this->messageHandler.LogHeader();*/
		}

		//Spieler zum Chatten selektieren
		void Client::selectPlayer(int mx, int my)
		{
			/*Ogre::Real x = 0;
			Ogre::Real y = 0;
			NOWA::Core::getSingletonPtr()->getMouse()ToViewPort(mx, my, x, y, this->camera->getViewport());
			//Aus Mausposition, Position in der 3D-Szene berechnen
			Ogre::Ray selectRay = this->camera->getCameraToViewportRay(x, y);
			this->pRaySceneQuery->setRay(selectRay);
			Ogre::RaySceneQueryResult &result = this->pRaySceneQuery->execute();

			Ogre::RaySceneQueryResult::iterator itr;
			for (itr = result.begin(); itr != result.end(); itr++)
			{
				if ((*itr).movable->getQueryFlags() == NOWA::Core::getSingletonPtr()->PLAYERMASK)
				{
					//Ogre Movableobject in Flubber casten
					this->pSelectedFlubber = Ogre::any_cast<Flubber*>((*itr).movable->getUserAny());
					//Man soll sich nicht selbst anwaehlen koennen
					if (this->pSelectedFlubber != Flubber::pMyFlubber)
						this->pSelectedFlubber->getNode()->showBoundingBox(true);
					else
						this->pSelectedFlubber = NULL;
				}
				else
				{
					//Wenn ein Flubber selektiert wurde und irgendwo anders hingeklickt wird, so muss die Selektion entfernt werden
					if (this->pSelectedFlubber)
					{
						this->pSelectedFlubber->getNode()->showBoundingBox(false);
						this->pSelectedFlubber = NULL;
					}
				}
			}*/
		}

		bool Client::keyPressed(const OIS::KeyEvent &keyEventRef)
		{
			/*//Nur der Client soll seinen Spieler ueber Benutzereingaben bewegen koennen
			Ogre::Vector3 direction = Ogre::Vector3::ZERO;
			switch (keyEventRef.key)
			{
			case OIS::KC_W:
				{
					//Bewegen nur wenn sich der Benutzer nicht im Chat-Modus befindet
					if (!this->chatTextboxActive)
					{
						bool move = true;
						Flubber::pMyFlubber->applyMovement(move);
					}
					break;
				}
			case OIS::KC_RETURN:
				{
					//Nur Chatbox aktivieren, wenn die Statusleiste aktiviert ist
					if (this->statusActive)
					{
						this->chatTextboxActive = !this->chatTextboxActive;
						if (this->chatTextboxActive)
						{
							this->chatMessageTextBox->getOverlayElement()->setMaterialName("SdkTrays/Button/Over");
						}
						else
						{
							this->chatMessageTextBox->getOverlayElement()->setMaterialName("SdkTrays/TextBox");
							this->sendChatMessage(this->pSelectedFlubber);
							this->chatMessageTextBox->clearText();
						}
					}
					break;
				}
			}

			//Wenn das Textfeld aktiviert wurde, kann der Text eingetragen werden
			if (this->statusActive && this->chatTextboxActive)
				NOWA::Core::getSingletonPtr()->writeText(this->chatMessageTextBox, keyEventRef, 254);

			if (Flubber::pMyFlubber)
			{
				//Nur wenn sich der Spieler am Boden befinden soll er Springen koennen
				if (Flubber::pMyFlubber->getHeight() < 0.5f)
				{
					if (keyEventRef.key == OIS::KC_SPACE)
					{
						if (!this->chatTextboxActive)
						{
							bool jump = true;
							Flubber::pMyFlubber->applyJump(jump);
						}
					}
				}
			}

			switch (keyEventRef.key)
			{
			case OIS::KC_TAB:
				{
					//Statusanzeige vom jeweiligen Verbindungstyp anzeigen
					this->statusActive = !this->statusActive;
					this->showStatus(this->statusActive);
					break;
				}
			default:
				break;
			}*/
			return true;
		}

		bool Client::keyReleased(const OIS::KeyEvent &keyEventRef)
		{
			/*//Nur der Client soll seinen Spieler ueber Benutzereingaben bewegen koennen

			if (Flubber::pMyFlubber)
			{
				switch (keyEventRef.key)
				{
				case OIS::KC_W:
					{
						if (!this->chatTextboxActive)
						{
							bool move = false;
							Flubber::pMyFlubber->applyMovement(move);
						}
						break;
					}
				}

				if (keyEventRef.key == OIS::KC_SPACE)
				{
					//if (!this->chatTextboxActive)
					{
						bool jump = false;
						Flubber::pMyFlubber->applyJump(jump);
					}
				}
			}*/
			return true;
		}

		bool Client::mouseMoved(const OIS::MouseEvent &arg)
		{
			return true;
		}

		bool Client::mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
		{
			/*if (Flubber::pMyFlubber)
			{
				if (id == OIS::MB_Right)
				{
					////Funktioniert leider nicht
					//int x = arg.state.X.abs / this->camera->getViewport()->getActualWidth();
					//int y = arg.state.Y.abs / this->camera->getViewport()->getActualHeight();
					////SetCursorPos(x/2, y/2);
					//COORD coord = {x, y};
					//   SetConsoleCursorPosition ( GetStdHandle ( STD_OUTPUT_HANDLE ), coord );
					//Ogre::OverlayContainer *Cursor = NOWA::Core::getSingletonPtr()->getTrayManager()->getCursorContainer();
					//pCursor->setPosition(x/2, y/2);
				}
				//Mit der linken Maustaste eine Kugel abfeuern
				else if (id == OIS::MB_Left)
				{
					if (!this->chatTextboxActive)
					{
						//Schuss ausfuehren, wenn sich der Spieler im Schussmodus befindet
						if (Flubber::pMyFlubber->isInShootMode())
						{
							bool shoot = true;
							Flubber::pMyFlubber->applyShoot(shoot);
						}
						else
						{
							//Spieler per Rechtsklick selektieren
							this->selectPlayer(arg.state.X.abs, arg.state.Y.abs);
						}
					}
				}
			}*/
			return true;
		}

		bool Client::mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
		{
			return true;
		}

		void Client::processUnbufferedKeyInput(Ogre::Real dt)
		{

		}

		void Client::processUnbufferedMouseInput(Ogre::Real dt)
		{
			const OIS::MouseState& ms = NOWA::InputDeviceCore::getSingletonPtr()->getMouse()->getMouseState();

			/*if (Flubber::pMyFlubber)
			{
				if (!this->chatTextboxActive)
				{
					//Wenn der Wii-Controller nicht benutzt wird
					if (!NOWA::Core::getSingletonPtr()->getOptionWiiUse())
					{
						//Kamera vom Spieler rotieren
						Flubber::pMyFlubber->rotateTo(ms, dt);
						//Mauszeiger nur zeigen, wenn ein linksclick erfolgt
						if (ms.buttonDown(OIS::MB_Left) && (!Flubber::pMyFlubber->isInShootMode()))
						{
							if (!NOWA::Core::getSingletonPtr()->getTrayManager()->isCursorVisible())
								NOWA::Core::getSingletonPtr()->getTrayManager()->showCursor();
						}
						else
						{
							if (NOWA::Core::getSingletonPtr()->getTrayManager()->isCursorVisible())
								NOWA::Core::getSingletonPtr()->getTrayManager()->hideCursor();
						}
					}
				}
			}*/
		}

		void Client::update(Ogre::Real dt)
		{
			this->updatePackets(dt);
			// this->ogreNewt->update(dt);
			/*if (Flubber::pMyFlubber)
			{
				if (NOWA::Core::getSingletonPtr()->getOptionWiiUse())
				{
					Flubber::pMyFlubber->wiiRotateTo(dt);
					Flubber::pMyFlubber->wiiShoot();
				}
				if (Flubber::pMyFlubber->getHeight() >= 0.5f)
				{
					//Wenn der Spieler gesprungen war, soll er fallen
					//ansonsten würde diese Funktion laufend aufgerufen!
					bool jump = false;
					Flubber::pMyFlubber->applyJump(jump);
				}
				//Anhand der Spielzustandsdaten die neue Geschwindigkeit, Orientierung usw. ermitteln
				Flubber::pMyFlubber->update(dt);
				//this->ogreNewt->update(dt);
				//Spielzustand zum Server mit der aktuellen Position, Geschwindigkeit usw senden.
				Flubber::pMyFlubber->sendGamestateToServer(dt);
			}*/
		}

		void Client::disconnect(void)
		{
			RakNet::BitStream bitstream;

			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[Client] exit for player Id: "
				+ Ogre::StringConverter::toString(AppStateManager::getSingletonPtr()->getRakNetModule()->getClientID()));

			bitstream.Write((RakNet::MessageID)RakNetModule::ID_CLIENT_EXIT);
			bitstream.Write(AppStateManager::getSingletonPtr()->getRakNetModule()->getClientID());

			// send data with high priority, reliable (TCP), 0, IP + Port, do not broadcast but just send to the client which attempted a connection
			AppStateManager::getSingletonPtr()->getRakNetModule()->getRakPeer()->Send(&bitstream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, AppStateManager::getSingletonPtr()->getRakNetModule()->getServerAddress(), false);
		}

		// here better config, if reliable etc.
		void Client::sendMousePosition(const Ogre::Vector2& position)
		{
			RakNet::BitStream bitstream;
			bitstream.Write(RakNetModule::ID_CLIENT_MOUSE_MOVE_EVENT_PACKET);
			bitstream.Write(position.x);
			bitstream.Write(position.y);

			AppStateManager::getSingletonPtr()->getRakNetModule()->getRakPeer()->Send(&bitstream, LOW_PRIORITY, UNRELIABLE, 0, AppStateManager::getSingletonPtr()->getRakNetModule()->getServerAddress(), false);
		}

		void Client::sendMouseButtonState(unsigned short button, char pressed)
		{
			RakNet::BitStream bitstream;
			bitstream.Write(RakNetModule::ID_CLIENT_MOUSE_INPUT_EVENT_PACKET);
			bitstream.Write(button);
			bitstream.Write(pressed);

			AppStateManager::getSingletonPtr()->getRakNetModule()->getRakPeer()->Send(&bitstream, LOW_PRIORITY, UNRELIABLE, 0, AppStateManager::getSingletonPtr()->getRakNetModule()->getServerAddress(), false);
		}

	} // namespace end NET

} // namespace end NOWA
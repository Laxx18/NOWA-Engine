#include "NOWAPrecompiled.h"
#include "Server.h"
#include "main/Core.h"
#include "main/AppStateManager.h"

namespace NOWA
{
	namespace NET
	{

		Server::Server(Ogre::SceneManager* sceneManager, OgreNewt::World* ogreNewt)
			: ConnectionType(sceneManager, ogreNewt),
			currentPlayerID(-1)/*,
			connectedClientsMenu(0),
			paramsPanel(0)*/
		{
			// this->cameraNode->setOrientation(Ogre::Quaternion(Ogre::Degree(-90), Ogre::Vector3::UNIT_X));
			// NOWA::Core::getSingletonPtr()->getMyDebugLog() = Ogre::LogManager::getSingleton().createLog("OgreMyDebugLogServer.log", false, false, false);
		}

		Server::~Server(void)
		{
			// DataStructures::List<RakNet::Replica3*> replicaList;
			// AppStateManager::getSingletonPtr()->getRakNetModule()->appReplicaManager.GetReferencedReplicaList(replicaList);
			AppStateManager::getSingletonPtr()->getRakNetModule()->remoteComponents.Clear();
			AppStateManager::getSingletonPtr()->getRakNetModule()->appReplicaManager.Clear();
		}

		void Server::setupWidgets(void)
		{
			//Platz fuer Statusliste beim Client schaffen, es wird so viel Platz geschafft, wie die virtuelle Umgebung Plaetze fuer Spieler besitzt
			Ogre::StringVector strParamNames;
			for (unsigned int i = 0; i < AppStateManager::getSingletonPtr()->getRakNetModule()->getClients().size(); i++)
			{
				strParamNames.push_back("Client" + Ogre::StringConverter::toString(i));
			}

			//Statusliste erstellen
			//this->paramsPanel = NOWA::Core::getSingletonPtr()->getTrayManager()->createParamsPanel(OgreBites::TL_NONE, "ConnectedClientsMenu",
			//	NOWA::Core::getSingletonPtr()->getOgreRenderWindow()->getWidth(), strParamNames);*/
			////Zunaechst verstecken
			//this->paramsPanel->hide();
		}

		void Server::showStatus(bool bShow)
		{
			//Status zeigen
			//if (bShow)
			//{
			//	this->paramsPanel->show();
			//	NOWA::Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(this->paramsPanel, OgreBites::TL_TOPLEFT);
			//}
			//else
			//{
			//	//Status verstecken
			//	while (NOWA::Core::getSingletonPtr()->getTrayManager()->getTrayContainer(OgreBites::TL_TOPLEFT)->isVisible())
			//	{
			//		NOWA::Core::getSingletonPtr()->getTrayManager()->moveWidgetToTray(OgreBites::TL_TOPLEFT, 0, OgreBites::TL_NONE);
			//	}
			//	this->paramsPanel->hide();
			//}
		}

		void Server::simulateClientMoveGameEvent(RakNet::Packet *packet, Ogre::Real dt)
		{
			/*//Server liest Daten vom Client
			RakNet::BitStream bitstream(packet->data, packet->length, false);
			unsigned char playerID;
			sGamestate currentGamestate;
			//Daten empfangen
			bitstream.IgnoreBytes(sizeof(RakNet::MessageID));
			bitstream.Read(currentGamestate.move);
			//Wenn der Spieler sich begonnen hat zu bewegen, wird die Bewegungsrichtung auch gelesen
			if (currentGamestate.move == 1)
			bitstream.Read(currentGamestate.angularVelocity);

			bitstream.Read(playerID);

			if (Flubber::remoteFlubbers.Size() > 0)
			{
			//Aktuellen Spieler erhalten
			Flubber* pCurrentFlubber = Flubber::getFlubberById(playerID);

			pCurrentFlubber->gamestate.move = currentGamestate.move;
			pCurrentFlubber->applyMovement(currentGamestate.move);
			//Wenn der Spieler sich begonnen hat zu bewegen, wird die Bewegungsrichtung gesetzt
			if (currentGamestate.move == 1)
			pCurrentFlubber->gamestate.angularVelocity = currentGamestate.angularVelocity;
			//NOWA::Ogre::LogManager::getSingletonPtr()->logMessage("applyMovement: " + Ogre::StringConverter::toString(currentGamestate.move));

			//Daten an alle Clients weiterleiten
			RakNet::BitStream bitstream;
			//Aufpassen: wenn Packet ID gesendet wird, muss diese gecastet werden!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			bitstream.Write((RakNet::MessageID)NOWA::Core::ID_SERVER_MOVEDGAMEEVENT_PACKET);
			bitstream.Write(currentGamestate.move);
			bitstream.Write(playerID);

			//Server sendet an alle ausser an den Client, von dem das Paket stammt, damit diese die entsprechende Animation abspielen koennen
			NOWA::Core::getSingletonPtr()->pRakPeer->Send(&bitstream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, true);
			}*/
		}

		void Server::simulateClientJumpGameEvent(RakNet::Packet *packet)
		{
			/*//Server liest Daten vom Client
			RakNet::BitStream bitstream(packet->data, packet->length, false); // The false is for efficiency so we don't make a copy of the passed data
			unsigned char playerID;
			sGamestate currentGamestate;
			bitstream.IgnoreBytes(sizeof(RakNet::MessageID));
			bitstream.Read(currentGamestate.jump);
			bitstream.Read(playerID);

			if (Flubber::remoteFlubbers.Size() > 0)
			{
			Flubber* pCurrentFlubber = Flubber::getFlubberById(playerID);
			//Sprungstart oder ende setzen
			pCurrentFlubber->gamestate.jump = currentGamestate.jump;
			//Sprung ausfuehren, falls jump == 1
			pCurrentFlubber->applyJump(currentGamestate.jump);

			//Daten an alle Clients schicken
			RakNet::BitStream bitstream;
			//Aufpassen: wenn Packet ID gesendet wird, muss diese gecastet werden!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			bitstream.Write((RakNet::MessageID)NOWA::Core::ID_SERVER_JUMPEDGAMEEVENT_PACKET);
			bitstream.Write(currentGamestate.jump);
			bitstream.Write(playerID);

			//Server sendet an alle ausser an den Client, von dem das Paket stammt
			NOWA::Core::getSingletonPtr()->pRakPeer->Send(&bitstream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, true);
			}*/
		}

		void Server::simulateClientShootGameEvent(RakNet::Packet *packet)
		{
			/*//Server liest Daten vom Client
			RakNet::BitStream bitstream(packet->data, packet->length, false); // The false is for efficiency so we don't make a copy of the passed data
			unsigned char playerID;
			sGamestate currentGamestate;
			//bitstream.IgnoreBytes(sizeof(RakNet::MessageID));
			//bitstream.Read(currentGamestate.timeStamp);
			bitstream.IgnoreBytes(sizeof(RakNet::MessageID));
			bitstream.Read(currentGamestate.shoot);
			bitstream.Read(currentGamestate.bulletDirection.x);
			bitstream.Read(currentGamestate.bulletDirection.y);
			bitstream.Read(currentGamestate.bulletDirection.z);
			bitstream.Read(playerID);

			if (Flubber::remoteFlubbers.Size() > 0)
			{
			Flubber* pHitFlubber = NULL;

			//Spieler aus ID erhalten
			Flubber* pCurrentFlubber = Flubber::getFlubberById(playerID);

			//Spielzustaende setzen
			pCurrentFlubber->gamestate.shoot = currentGamestate.shoot;
			//Schneeball Flugbahnrichtung setzen
			pCurrentFlubber->gamestate.bulletDirection = currentGamestate.bulletDirection;
			//Schuss ausfuehren, oder beenden
			pCurrentFlubber->applyShoot(currentGamestate.shoot);
			//Wenn geschossen wurde
			if (currentGamestate.shoot == 1)
			{
			//Der aktuelle Spieler muss Latenz viel zurückgesetzt werden und von dieser Position aus wird das Kommando ausgeführt...
			for (unsigned int i = 0; i < Flubber::remoteFlubbers.Size(); i++)
			{
			//Alle Spieler in der Zeit zuruecksetzen

			Flubber* pRemoteFlubber = Flubber::remoteFlubbers[i];

			pRemoteFlubber->presentPosition = pRemoteFlubber->getNode()->getPosition();

			sEntityShootState pastState(RakNet::GetTimeMS(), pRemoteFlubber->getNode()->getPosition());
			//Nach Valve Formel: Command Execution Time = Current Server Time - Packet-Latency - Client View Interpolation

			//unsigned long pastTime = pCurrentFlubber->latencyToServer + unsigned long(pCurrentFlubber->interpolationTime);
			//pRemoteFlubber->latencyToRemotePlayer = Flubber::remoteFlubbers[playerID]->highestLatencyToServer / 2;

			//Latenz zum Spieler berechnen, der geschossen hat
			pRemoteFlubber->latencyToRemotePlayer = pCurrentFlubber->latencyToServer;
			//pRemoteFlubber->latencyToRemotePlayer = pCurrentFlubber->highestLatencyToServer;

			//+ 254, da der Server erst nach 254ms den Schuss ausfuehrt vom Client der eine Latenz von 254 hat
			unsigned long pastTime = unsigned long(pRemoteFlubber->latencyToRemotePlayer * 2) + unsigned long(pRemoteFlubber->interpolationTime);
			//unsigned long pastTime = unsigned long(17) + pRemoteFlubber->highestLatencyToServer + unsigned long(pRemoteFlubber->interpolationTime);

			pRemoteFlubber->entityShootStateHistoryOfTheServer.timewarp(pastState, RakNet::GetTimeMS() - pastTime);
			NOWA::Ogre::LogManager::getSingletonPtr()->logMessage("shotTimeInPast remote: " + Ogre::StringConverter::toString(pastTime));
			NOWA::Ogre::LogManager::getSingletonPtr()->logMessage("pCurrentFlubber->latencyToRemotePlayer remote: " + Ogre::StringConverter::toString(pRemoteFlubber->latencyToRemotePlayer));
			//Nur die Position zum alten Zeitpunkt spielt eine Rolle, die Orientierung nicht
			pRemoteFlubber->getBody()->setPositionOrientation(pastState.position, pRemoteFlubber->getNode()->getOrientation());
			NOWA::Ogre::LogManager::getSingletonPtr()->logMessage("RemotePlayer at pastposition: " + Ogre::StringConverter::toString(pRemoteFlubber->getBody()->getPosition()));


			//Der aktuelle Spieler fuehrt den Schuss aus
			if(Flubber::remoteFlubbers[i]->getClientID() == playerID)
			{
			//vier Rays werden abgefeuert, die in etwa der Breite und Hoehe des Schneeballs entsprechen, damit ein Gegner besser getroffen werden kann
			OgreNewt::BasicRaycast::BasicRaycastInfo contactLeft =		pCurrentFlubber->getContactToDirection(currentGamestate.bulletDirection, Ogre::Vector3(0.1f, 0.8f, -0.2f), (Ogre::Real)NOWA::Core::getSingletonPtr()->areaOfInterest);
			OgreNewt::BasicRaycast::BasicRaycastInfo contactRight =		pCurrentFlubber->getContactToDirection(currentGamestate.bulletDirection, Ogre::Vector3(0.7f, 0.8f, -0.2f), (Ogre::Real)NOWA::Core::getSingletonPtr()->areaOfInterest);
			OgreNewt::BasicRaycast::BasicRaycastInfo contactTop =		pCurrentFlubber->getContactToDirection(currentGamestate.bulletDirection, Ogre::Vector3(0.4f, 1.1f, -0.2f), (Ogre::Real)NOWA::Core::getSingletonPtr()->areaOfInterest);
			OgreNewt::BasicRaycast::BasicRaycastInfo contactBottom =	pCurrentFlubber->getContactToDirection(currentGamestate.bulletDirection, Ogre::Vector3(0.4f, 0.5f, -0.2f), (Ogre::Real)NOWA::Core::getSingletonPtr()->areaOfInterest);
			OgreNewt::BasicRaycast::BasicRaycastInfo contactMiddle =	pCurrentFlubber->getContactToDirection(currentGamestate.bulletDirection, Ogre::Vector3(0.4f, 0.8f, -0.2f), (Ogre::Real)NOWA::Core::getSingletonPtr()->areaOfInterest);

			//Testen ob ein Spieler von irgend einem Ray getroffen wurde
			//Dabei werden jedoch 5 Rays parallel in einem Abstand zueinander gefeuert, um die groesse des Schneebald in etwa zu umgrenzen
			if (contactLeft.mBody)
			{
			if (contactLeft.mBody->getType() == NOWA::Core::getSingletonPtr()->PLAYERTYPE)
			pHitFlubber = OgreNewt::any_cast<Flubber*>(contactLeft.mBody->getUserData());
			}
			if (contactRight.mBody)
			{
			if (contactRight.mBody->getType() == NOWA::Core::getSingletonPtr()->PLAYERTYPE)
			pHitFlubber = OgreNewt::any_cast<Flubber*>(contactRight.mBody->getUserData());
			}
			if (contactTop.mBody)
			{
			if (contactTop.mBody->getType() == NOWA::Core::getSingletonPtr()->PLAYERTYPE)
			pHitFlubber = OgreNewt::any_cast<Flubber*>(contactTop.mBody->getUserData());
			}
			if (contactBottom.mBody)
			{
			if (contactBottom.mBody->getType() == NOWA::Core::getSingletonPtr()->PLAYERTYPE)
			pHitFlubber = OgreNewt::any_cast<Flubber*>(contactBottom.mBody->getUserData());
			}
			if (contactMiddle.mBody)
			{
			if (contactMiddle.mBody->getType() == NOWA::Core::getSingletonPtr()->PLAYERTYPE)
			pHitFlubber = OgreNewt::any_cast<Flubber*>(contactMiddle.mBody->getUserData());
			}

			//aktuellen Spieler an die richtige Position bringen
			pCurrentFlubber->getBody()->setPositionOrientation(pCurrentFlubber->presentPosition, pCurrentFlubber->getNode()->getOrientation());

			//Ein Spieler wurde getroffen
			if (pHitFlubber)
			{
			//Dem betroffenen Spieler Energie abziehen
			pHitFlubber->setEnergy(pHitFlubber->getEnergy() - 10);
			//Statusanzeige aktualisieren (nicht beim Server!)
			//pHitFlubber->changeObjectTitle(Ogre::StringConverter::toString(pHitFlubber->getEnergy()) + "%");
			//NOWA::Ogre::LogManager::getSingletonPtr()->logMessage("pHitFlubber id " + Ogre::StringConverter::toString(pHitFlubber->getClientID()) + " Energy: " + Ogre::StringConverter::toString(pHitFlubber->getEnergy()));
			}
			}
			}

			//Positionen wieder auf den aktuellen Stand bringen
			for (unsigned int i = 0; i < Flubber::remoteFlubbers.Size(); i++)
			{
			//Alle Spieler wieder in die Gegenwart bringen
			if (Flubber::remoteFlubbers[i]->getClientID() != playerID)
			{
			Flubber* pRemoteFlubber = Flubber::remoteFlubbers[i];
			//sEntityShootState lastState = pRemoteFlubber->entityShootStateHistoryOfTheServer.getLastState();
			pRemoteFlubber->getBody()->setPositionOrientation(pRemoteFlubber->presentPosition, pRemoteFlubber->getNode()->getOrientation());
			}
			}
			}

			RakNet::BitStream bitstream;
			//Aufpassen: wenn Packet ID gesendet wird, muss diese gecastet werden!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			//bitstream.Write((RakNet::MessageID)ID_TIMESTAMP);
			//bitstream.Write(RakNet::GetTime());

			//Ein Spieler wurde getroffen, Daten an alle Clients senden
			if (pHitFlubber)
			{
			bitstream.Write((RakNet::MessageID)NOWA::Core::ID_SERVER_SHOTHITGAMEEVENT_PACKET);
			//restliche Energie senden
			bitstream.Write(pHitFlubber->getEnergy());
			bitstream.Write(pHitFlubber->getClientID());
			}
			else
			//Wenn er nicht getoffen wurde, dann nur das Protokoll senden, dass ein Schuss abgefeuert wurde
			bitstream.Write((RakNet::MessageID)NOWA::Core::ID_SERVER_SHOTGAMEEVENT_PACKET);

			//Schuss und Schneeball Flugbahnrichtung senden
			bitstream.Write(currentGamestate.shoot);
			bitstream.Write(currentGamestate.bulletDirection.x);
			bitstream.Write(currentGamestate.bulletDirection.y);
			bitstream.Write(currentGamestate.bulletDirection.z);
			bitstream.Write(playerID);

			//Server sendet das Schussresultat an alle, damit wenn der Spieler einen anderen getroffen hat, der entsprechende Sound abgespielt wird
			// daher spaeter: RakNet::UNASSIGNED_SYSTEM_ADDRESS
			//Nur wenn ein Spieler getroffen wurde, soll auch zurueck an den Sender gesendet werden, damit er sieht, wie dem getroffenen Spieler Energie abgezogen wird und ein Sound abgespielt wird
			if (pHitFlubber)
			NOWA::Core::getSingletonPtr()->pRakPeer->Send(&bitstream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
			else
			//Wenn er nicht getroffen hat, muss der Sender nichts empfangen, da er sowieso lokal geschossen hatte
			NOWA::Core::getSingletonPtr()->pRakPeer->Send(&bitstream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, true);
			}*/
		}

		//Serverseitige Simulation
		//Vorher waren es mehrere Funktionen, die ausgefuhert wurde, allerdings wurde der Server dermaßen ueberflutet, das er den gamestate nicht mehr richtig zuordnen konnte, spieler nahmen position von anderen Spielern an
		//Daher gegen die Beschreibung von Valve alles in einem Zug ausfuehren und Resultate zuruecksenden
		void Server::simulateClientMovement(RakNet::Packet *packet, Ogre::Real dt)
		{
			/*//Server liest Daten vom Client
			RakNet::BitStream bitstream(packet->data, packet->length, false); // The false is for efficiency so we don't make a copy of the passed data
			unsigned char playerID;
			sGamestate currentGamestate;
			bitstream.IgnoreBytes(sizeof(RakNet::MessageID));
			bitstream.Read(currentGamestate.timeStamp);
			bitstream.IgnoreBytes(sizeof(RakNet::MessageID));
			//Drehung des Spieler erhalten
			bitstream.Read(currentGamestate.angularVelocity);
			//Hoehe, wie hoch er gesprungen ist, erhalten
			bitstream.Read(currentGamestate.height);
			bitstream.Read(playerID);

			if (Flubber::remoteFlubbers.Size() > 0)
			{
			//ID zwischenspeichern, da der aktuelle Spieler hier schon aktualisiert wird, dafuer darf er dann nicht in der Funktion updateGamestatesOfRemotePlayers aktualisiert werden
			this->currentPlayerID = playerID;

			//NOWA::Ogre::LogManager::getSingletonPtr()->logMessage("serverlatency: " + Ogre::StringConverter::toString(Flubber::remoteFlubbers[playerID]->latencyToServer));
			//Aktuellen Spieler erhalten
			Flubber* pCurrentFlubber = Flubber::getFlubberById(playerID);
			//Flubber* pCurrentFlubber = Flubber::remoteFlubbers[playerID];

			//Latenz zum Server berechnen (aktuelle zeit - Zeitstempel der vom Client versendet wurde
			//Von der Latenz muss dt abgezogen werden, da immer mit 16ms Verspaetung, die Daten versendet werden
			if (RakNet::GetTime() > currentGamestate.timeStamp + (unsigned long)(dt*1000.0f))
			pCurrentFlubber->latencyToServer = (unsigned long)(RakNet::GetTime() - (currentGamestate.timeStamp + (unsigned long)(dt*1000.0f)));
			else
			pCurrentFlubber->latencyToServer = 0;

			this->insertLatencyOnServerInClientList(pCurrentFlubber);
			//Flubber::remoteFlubbers[playerID]->latencyToServer =  (unsigned long)(RakNet::GetTime() - currentGamestate.timeStamp);

			//Spielzustand beim Serverspieler aktualisieren (Ich sende meinen Spielzustand an den Server und bin ein entfernter Spieler beim Server, dieser erhaelt meine Daten, damit der Server simulieren kann)
			pCurrentFlubber->gamestate.useTimeStamp = currentGamestate.useTimeStamp;
			pCurrentFlubber->gamestate.timeStamp = currentGamestate.timeStamp;
			//pCurrentFlubber->gamestate.velocity = currentGamestate.velocity;
			pCurrentFlubber->gamestate.angularVelocity = currentGamestate.angularVelocity;
			pCurrentFlubber->gamestate.height = currentGamestate.height;

			//Schießposition eintragen fuer TimeWarp
			pCurrentFlubber->entityShootStateHistoryOfTheServer.enqeue(pCurrentFlubber->getNode()->getPosition(), RakNet::GetTimeMS());

			//Physik fuer den aktuellen Spieler extrapolieren
			pCurrentFlubber->update(dt);
			//aktuellen Spielerzustand nach der Extraplation von Physik erhalten
			sGamestate newGamestate = pCurrentFlubber->gamestate;

			//Finale Position des entfernten Spielers setzen
			Ogre::Vector3 resultPosition = Ogre::Vector3::ZERO;
			Ogre::Quaternion resultOrientation = Ogre::Quaternion::IDENTITY;

			//Resultierende Position und Orientierung an alle Clients senden, damit diese zu diesen Daten interpolieren koennen
			if (pCurrentFlubber->getNode())
			{
			resultPosition = pCurrentFlubber->getNode()->getPosition();
			//Finale Orientierung des entfernten Spielers setzen
			resultOrientation = pCurrentFlubber->getNode()->getOrientation();
			}


			RakNet::BitStream bitstream;
			//Aufpassen: wenn Packet ID gesendet wird, muss diese gecastet werden!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			bitstream.Write((RakNet::MessageID)ID_TIMESTAMP);
			bitstream.Write(RakNet::GetTime());
			bitstream.Write((RakNet::MessageID)NOWA::Core::ID_SERVER_MOVED_REMOTEPLAYERS_RESULT_PACKET);

			//finale Position wird versandt
			bitstream.Write(resultPosition.x);
			bitstream.Write(resultPosition.y);
			bitstream.Write(resultPosition.z);
			//finale Orientierung wird versandt
			bitstream.Write(resultOrientation.w);
			bitstream.Write(resultOrientation.x);
			bitstream.Write(resultOrientation.y);
			bitstream.Write(resultOrientation.z);

			bitstream.Write(currentGamestate.height);
			bitstream.Write(playerID);

			//Nach Spielern innerhalb von 60 Meter suchen, Spieler außerhalb der Grenze erhalten keine Updates, da sie nicht im Intresse fuer den aktuellen Spieler sind
			//und zu diesen Spielern Daten senden.
			pCurrentFlubber->sendDataToInterestedPlayers(&bitstream);
			}
			//ID wird wieder auf nicht besetzt gesetzt
			this->currentPlayerID = -1;*/
		}

		void Server::updateGamestatesOfRemotePlayers(Ogre::Real dt)
		{
			/*//In dieser Liste sind beim Server alle Spieler enthalten. Der Server simuliert alle entfernten Spieler
			for (unsigned int i = 0; i < Flubber::remoteFlubbers.Size(); i++)
			{
			//Alle Spieler ausser den, der schon in der vorigen Prozedtur aktualisiert wurde, aktualisieren
			if (Flubber::remoteFlubbers[i]->getClientID() != this->currentPlayerID)
			Flubber::remoteFlubbers[i]->update(dt);
			}*/
		}

		/*void Server::insertLatencyOnServerInClientList(Character* pCurrentCharacter)
		{
		//Rubrik aus Liste erhalten
		Ogre::String strItem = NOWA::Core::getSingletonPtr()->pParamsPanel->getParamValue(pCurrentFlubber->getClientID());
		//Zeichen suchen: latenz/ und ersetzen durch den Wert latencyToServer und eintragen
		Ogre::String strSubItem = strItem.replace(strItem.find("/", strItem.size() - 12) + 1, strItem.size(), Ogre::StringConverter::toString((unsigned long)pCurrentFlubber->latencyToServer));
		NOWA::Core::getSingletonPtr()->pParamsPanel->setParamValue(pCurrentFlubber->getClientID(), strSubItem);
		//NOWA::Ogre::LogManager::getSingletonPtr()->logMessage("LatencyToServer: " + Ogre::StringConverter::toString(pCurrentFlubber->latencyToServer));
		}*/

		//systemIdentifier = IP + Port
		/*void Server::sendPlayerInformationToClient(RakNet::Packet* packet)
		{
		RakNet::BitStream bitstream;

		Ogre::Vector3 startPosition;
		Ogre::Quaternion startOrientation;
		unsigned int playerID = 0;
		//Durch alle in der virtuellen Umgebung verfuegbaren Startpositionen laufen
		while (AppStateManager::getSingletonPtr()->getRakNetModule()->getClientID() < AppStateManager::getSingletonPtr()->getRakNetModule()->getClientsStartData().size()) {
		//Wenn eine Startposition besetzt ist, wird zur naechsten propagiert und es wird die ID hochgezaehlt
		if (AppStateManager::getSingletonPtr()->getRakNetModule()->getClientsStartData().at(AppStateManager::getSingletonPtr()->getRakNetModule()->getClientID()).occupied) {
		AppStateManager::getSingletonPtr()->getRakNetModule()->incrementPlayerID();
		} else {
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[Server] sending player id: "
		+ Ogre::StringConverter::toString(AppStateManager::getSingletonPtr()->getRakNetModule()->getClientID()));
		//Position besetzen
		AppStateManager::getSingletonPtr()->getRakNetModule()->getClientsStartData().at(AppStateManager::getSingletonPtr()->getRakNetModule()->getClientID()).occupied = true;
		//Wenn eine freie Position gefunden wurde, wird diese besetzt
		//Nachrichten-ID senden
		bitstream.Write((RakNet::MessageID)RakNetModule::ID_PLAYERASSIGNMENT_PACKET);
		//SpielerID an Client senden
		bitstream.Write(AppStateManager::getSingletonPtr()->getRakNetModule()->getClientID());
		//Area von Intresse senden
		bitstream.Write(AppStateManager::getSingletonPtr()->getRakNetModule()->getAreaOfInterest());
		//gewaehlte Karte senden
		//bsOut.Write(NOWA::Core::getSingletonPtr()->virtualEnvironmentName.c_str(), 128); --> wurde schon über setofflineping gesendet
		startPosition = AppStateManager::getSingletonPtr()->getRakNetModule()->getClientsStartData().at(AppStateManager::getSingletonPtr()->getRakNetModule()->getClientID()).position;
		startOrientation = AppStateManager::getSingletonPtr()->getRakNetModule()->getClientsStartData().at(AppStateManager::getSingletonPtr()->getRakNetModule()->getClientID()).orientation;

		//Lokationsdaten an alle anderen Hosts senden
		bitstream.WriteVector(startPosition.x, startPosition.y, startPosition.z);
		bitstream.WriteNormQuat(startOrientation.w, startOrientation.x, startOrientation.y, startOrientation.z);

		RakNet::StringTable::Instance()->EncodeString(packet->systemAddress.ToString(), 255, &bitstream);

		//Datenstrom, Hohe Prioritaet, Zuverlaessig (TCP), 0, An IP + Port, Nicht Broadcasten sondern nur an den jenigen Klienten der sich verbunden hat
		AppStateManager::getSingletonPtr()->getRakNetModule()->getRakPeer()->Send(&bitstream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, false);
		return;
		}
		}
		//Nachrichten senden das kein freier Platz mehr in der virtuellen Umgebung existiert
		bitstream.Write((RakNet::MessageID)RakNetModule::ID_NO_FREE_SPACE);

		//Datenstrom, Hohe Prioritaet, Zuverlaessig (TCP), 0, An IP + Port, Nicht Broadcasten sondern nur an den jenigen Klienten der sich verbunden hat
		AppStateManager::getSingletonPtr()->getRakNetModule()->getRakPeer()->Send(&bitstream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, false);
		}*/

		//Vom Client chat erhalten und weiter an den Empfaenger senden
		void Server::readAndDispatchChatMessage(RakNet::Packet *packet)
		{
			char message[255];
			char strSystemAddress[255];
			RakNet::BitStream bitstream(packet->data, packet->length, false); //false: Daten werden nicht kopiert, sondern orginal genommen
			bitstream.IgnoreBytes(sizeof(RakNet::MessageID));
			//Nachricht erhalten
			RakNet::StringTable::Instance()->DecodeString(message, 255, &bitstream);
			//Adresse fuer den Empfaenger erhalten
			RakNet::StringTable::Instance()->DecodeString(strSystemAddress, 255, &bitstream);
			//IP und Port in RakNet Format umwandeln
			Ogre::String strMessage = Ogre::String(message);
			RakNet::SystemAddress systemAddress = strSystemAddress;
			//An den Empfaenger zuverlaessig (TCP) weiterleiten
			AppStateManager::getSingletonPtr()->getRakNetModule()->getRakPeer()->Send(&bitstream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, systemAddress, false);
		}

		//Diese Funktion funktioniert nicht
		void Server::NotifyReplicaOfMessageDeliveryStatus(RakNet::Packet *packet)
		{

		}

		void Server::updatePackets(Ogre::Real dt)
		{
			if (AppStateManager::getSingletonPtr()->getRakNetModule()->isConnectionFailed())
			{
				return;
			}
			RakNet::Packet* packet;
			for (packet = AppStateManager::getSingletonPtr()->getRakNetModule()->getRakPeer()->Receive(); packet;
				AppStateManager::getSingletonPtr()->getRakNetModule()->getRakPeer()->DeallocatePacket(packet), packet = AppStateManager::getSingletonPtr()->getRakNetModule()->getRakPeer()->Receive())
			{
				// the first byte is the packet id (message-id)
				unsigned char packetID = AppStateManager::getSingletonPtr()->getRakNetModule()->getPacketIdentifier(packet);
				// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[Server] packetID: " + Ogre::StringConverter::toString(packetID));
				switch (packetID)
				{
				case ID_CONNECTION_ATTEMPT_FAILED:
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[Server] ID_CONNECTION_ATTEMPT_FAILED from Client");
					break;
				case ID_NO_FREE_INCOMING_CONNECTIONS:
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[Server] ID_NO_FREE_INCOMING_CONNECTIONS from Client");
					break;
				case ID_CONNECTION_REQUEST_ACCEPTED:
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[Server] ID_CONNECTION_REQUEST_ACCEPTED from Client");
					break;
				case ID_DISCONNECTION_NOTIFICATION:
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[Server] Client ID_DISCONNECTION_NOTIFICATION from Client");
					break;
				case ID_CONNECTION_LOST:
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[Server] ID_CONNECTION_LOST from Client");
					break;
				case ID_NEW_INCOMING_CONNECTION:
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[Server] ID_NEW_INCOMING_CONNECTION from " + Ogre::String(packet->systemAddress.ToString()));
					// send the client data to the client target packet
					AppStateManager::getSingletonPtr()->getRakNetModule()->sendPlayerInformationToClient(packet);
					break;
				case RakNetModule::ID_CLIENT_READY:
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[RakNetModule:Server] got ID_CLIENT_READY from " + Ogre::String(packet->systemAddress.ToString()));
					RakNet::BitStream bitstream;
					bitstream.Write((RakNet::MessageID)RakNetModule::ID_CLIENT_START_REPLICATION);
					AppStateManager::getSingletonPtr()->getRakNetModule()->appReplicaManager.PushConnection(AppStateManager::getSingletonPtr()->getRakNetModule()->appReplicaManager.AllocConnection(packet->systemAddress, packet->guid));
					AppStateManager::getSingletonPtr()->getRakNetModule()->getRakPeer()->Send(&bitstream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, false);
					break;
				}
					//	//Protokoll damit der Server den Spieler bewegt
					//case NOWA::Core::ID_PLAYER_MOVE_CONFIRMATION_PACKET:
					//	this->simulateClientMovement(packet, dt);
					//	break;
					//	//Protokoll, damit der Server den Bewegungstart und -ende registriert
					//case NOWA::Core::ID_PLAYER_MOVEGAMEEVENT_PACKET:
					//	this->simulateClientMoveGameEvent(packet, dt);
					//	break;
					//	//Protokoll, damit der Server den Sprungstart und -ende registriert
					//case NOWA::Core::ID_PLAYER_JUMPGAMEEVENT_PACKET:
					//	this->simulateClientJumpGameEvent(packet);
					//	break;
					//	//Protokoll, damit der Server den Schussstart und -ende registriert
					//case NOWA::Core::ID_PLAYER_SHOOTGAMEEVENT_PACKET:
					//	this->simulateClientShootGameEvent(packet);
					//	break;
					//	//Protokoll um eine Nachricht weiterzuleiten
					//case ID_CHAT_MESSAGE_PACKET:
					//	this->readAndDispatchChatMessage(packet);
					//	break;
					//Reaktion auf Paketverlust, die leider nicht funktioniert in RakNet
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
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[Server] ID_SND_RECEIPT_LOSS from Client");
					////this->NotifyReplicaOfMessageDeliveryStatus(packet);
					break;
				}
					
				case RakNetModule::ID_CLIENT_EXIT:
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[RakNetModule] ID_CLIENT_EXIT from " + Ogre::String(packet->systemAddress.ToString()));
					AppStateManager::getSingletonPtr()->getRakNetModule()->receiveClientExit(packet);
					
					break;
				}
			}
			//Thread nicht schlafen legen
			// RakSleep(0);
		}

		//Serverstatistik herausschreiben
		void Server::writeStatistics(void)
		{
			/*NOWA::Core::getSingletonPtr()->pRakPeer->AttachPlugin(&this->messageHandler);
			this->messageHandler.StartLog("ServerStatistics.txt");
			this->messageHandler.LogHeader();*/
		}

		bool Server::keyPressed(const OIS::KeyEvent &keyEventRef)
		{
			/*switch (keyEventRef.key)
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

		bool Server::keyReleased(const OIS::KeyEvent &keyEventRef)
		{
			return true;
		}

		bool Server::mouseMoved(const OIS::MouseEvent &arg)
		{
			return true;
		}

		bool Server::mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
		{
			return true;
		}

		bool Server::mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
		{
			return true;
		}

		void Server::processUnbufferedKeyInput(Ogre::Real dt)
		{

		}

		void Server::processUnbufferedMouseInput(Ogre::Real dt)
		{
			/*const OIS::MouseState &ms = NOWA::Core::getSingletonPtr()->getMouse()->getMouseState();

			//Drehfaktor abhängig von der Spielgeschwindigkeit aus der aktuellen X-Koordinate der Maus erhalten
			this->cameraRotateScaleX = -ms.X.rel * 30.0f * dt;
			//Drehfaktor aus der aktuellen Y-Koordinate erhalten
			this->cameraRotateScaleY = -ms.Y.rel * 30.0f * dt;
			if (ms.buttonDown(OIS::MB_Left))
			{
			// man muss ts_world yaw und ts_local pitch!
			this->cameraNode->rotate(Ogre::Quaternion(Ogre::Degree(this->cameraRotateScaleX), Ogre::Vector3::UNIT_Y), Ogre::Node::TS_WORLD);
			this->cameraNode->rotate(Ogre::Quaternion(Ogre::Degree(this->cameraRotateScaleY), Ogre::Vector3::UNIT_X), Ogre::Node::TS_LOCAL);
			}*/
		}

		void Server::update(Ogre::Real dt)
		{
			this->updatePackets(dt);
		}

		void Server::updatePhysics(Ogre::Real dt)
		{
			//Server berechnet Physik (extrapolieren)
			//Dabei wird velocity aus der alten und neuen Position berechnet
			//Die neue Position berechnet sich aus der vorigen Position + Geschwindigkeit * dt (interpoliert)
			//Dabei wird die Orientierung interpoliert mittels Quaternion::Slerp(...)
			//Die Ogre-Knoten der Objekte erhalten die neuen Daten
			//Gravitations-, Masse-, (Kraft-?), Daempfung-Vektoren, Traegheits-, Reibungs-, Elastiztaets-, Sanftheits-Koeffizienten, Kollisionsabfragen werden intern von der Newton Game Dynamics Engine berechnet! 
			this->ogreNewt->update(dt);
		}

		void Server::disconnect(void)
		{
			AppStateManager::getSingletonPtr()->getRakNetModule()->destroyContent();
		}

	} // namespace end NET

} // namespace end NOWA
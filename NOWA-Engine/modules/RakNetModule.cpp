#include "NOWAPrecompiled.h"
#include "main/Core.h"
#include "RakNetModule.h"
#include "GetTime.h"
#include "RakSleep.h"
#include "RakAssert.h"
#include "StringTable.h"
#include "BitStream.h"
#include "utilities/rapidxml.hpp"
#include "utilities/XMLConverter.h"
#include "gameobject/PhysicsActiveComponent.h"
#include "main/Events.h"
#include "main/AppStateManager.h"

namespace NOWA
{

	RakNetModule::RakNetModule(const Ogre::String& appStateName)
		: appStateName(appStateName),
		rakPeer(0),
		server(true),
		serverPort(12345),
		serverIP("127.0.0.1"), //locale IP
		playerName("unknown"),
		nextFreePlayerID(1), // starts with 1!
		serverAddress(RakNet::UNASSIGNED_SYSTEM_ADDRESS),
		localAddress(RakNet::UNASSIGNED_SYSTEM_ADDRESS),
		packetSendRate(50), // send by default each 50ms a paket, that means 20 pakets/s
		interpolationTime(100), // 100ms interpolationrange
		areaOfInterest(0), // 60 meters by default
		highestLatencyToServer(50),
		serverName("TestServer"),
		clientConnectionToServer(false),
		serverInConsole(false),
		connectionFailed(true),
		statusActive(false),
		chatTextboxActive(false),
		allowedPlayerCount(0),
		networkSzenario(false)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[RakNetModule] Module created");
	}

	RakNetModule::~RakNetModule()
	{

	}

	void RakNetModule::destroyContent(void)
	{
		if (this->rakPeer)
		{
			this->rakPeer->Shutdown(100, 0);
			RakNet::RakPeerInterface::DestroyInstance(this->rakPeer);
			this->rakPeer = nullptr;
			this->server = true;
			this->serverPort = 12345;
			this->serverIP = "127.0.0.1";
			this->playerName = "unknown";
			this->nextFreePlayerID = 1;
			this->packetSendRate = 100;
			this->interpolationTime = 100;
			this->areaOfInterest = 60;
			this->highestLatencyToServer = 0;
			this->serverName = "";
			this->worldName = "";
			this->projectName = "";
			this->clientConnectionToServer = false;
			this->serverInConsole = false;
			this->connectionFailed = true;
			this->statusActive = false;
			this->chatTextboxActive = false;
			this->networkSzenario = false;

			// here clear? what is if its another level and the clients are the same, a reconnect would be shit
			this->clients.clear();
			this->remotePlayerNames.clear();
			// this->remotePlayerMap.Clear();
		}
	}

	void RakNetModule::createRakNetForClient(void)
	{
		if (!this->rakPeer)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[RakNetModule] Begin: Creating client...");
			this->server = false;
			this->rakPeer = RakNet::RakPeerInterface::GetInstance();

			RakNet::SocketDescriptor socketDeskriptor;
			// set the port to 0 because it will be assigned automatically
			socketDeskriptor.port = 0;

			//this->nextSendTime = 0;
			bool success = this->rakPeer->Startup(1, &socketDeskriptor, 1) == RakNet::RAKNET_STARTED;

			RakAssert(success);

			// occasional pinging needs to be enabled so that RakNet's timestamp negotiation
			// system works correctly, see: <http://www.jenkinssoftware.com/raknet/manual/timestamping.html>
			this->rakPeer->SetOccasionalPing(true);

			this->networkSzenario = true;

			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[RakNetModule] Finished: Creating client...");
		}
	}

	void RakNetModule::createRakNetForServer(const Ogre::String& serverName)
	{
		if (!this->rakPeer)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[RakNetModule] Begin: Creating server...");
			this->serverName = serverName;
			this->server = true;
			this->rakPeer = RakNet::RakPeerInterface::GetInstance();

			char serverData[200];
			//serverData = GameServer1
			strcpy(serverData, this->serverName.c_str());
			//serverData = GameServer1:
			strcat(serverData, ":");
			//serverData = GameServer1:World1
			strcat(serverData, this->projectName.c_str());
			//serverData = GameServer1:World1:
			strcat(serverData, "-");
			//serverData = GameServer1:World1-BoidsWorld
			strcat(serverData, this->worldName.c_str());

			strcat(serverData, ";\0");

			// send server data to client offline
			this->rakPeer->SetOfflinePingResponse(serverData, (const unsigned int)strlen(serverData));

			RakNet::SocketDescriptor socketDescriptor;
			// set serverport
			socketDescriptor.port = this->serverPort;

			// accept connection at port 12345
			// set a max number, how many clients can connect to the server, this value depends on the number of start positions in the 3D-virual environment, that has been set by the level designer
			bool success = this->rakPeer->Startup(this->allowedPlayerCount, &socketDescriptor, 1) == RakNet::RAKNET_STARTED;

			RakAssert(success);
			this->rakPeer->SetMaximumIncomingConnections(this->allowedPlayerCount);
			// this->rakPeer->SetTimeoutTime(30000, RakNet::UNASSIGNED_SYSTEM_ADDRESS);

			// occasional pinging needs to be enabled so that RakNet's timestamp negotiation
			// system works correctly, see: <http://www.jenkinssoftware.com/raknet/manual/timestamping.html>
			this->rakPeer->SetOccasionalPing(true);

			this->serverIP = this->rakPeer->GetLocalIP(0);
			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[RakNetModule] Server IP: " + this->serverIP);

			////Core::getSingletonPtr()->rakPeer->AdvertiseSystem("255.255.255.255", Core::getSingletonPtr()->serverPort, serverData, (int)strlen(serverData) + 1,0);

			//Core::getSingletonPtr()->rakPeer->AdvertiseSystem("255.255.255.255", Core::getSingletonPtr()->serverPort, 0, 0, 0);
			/*for (int i=0; i < 4; i++)
			{
			if (Core::getSingletonPtr()->rakPeer->GetInternalID(RakNet::UNASSIGNED_SYSTEM_ADDRESS,0).GetPort()!=Core::getSingletonPtr()->serverPort+i)
			Core::getSingletonPtr()->rakPeer->AdvertiseSystem("255.255.255.255", Core::getSingletonPtr()->serverPort+i, 0,0,0);
			}*/

			this->localAddress = this->rakPeer->GetInternalID();
			this->guid = this->rakPeer->GetGuidFromSystemAddress(this->localAddress);
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[RakNetModule] local server system address: " + Ogre::String(localAddress.ToString()));

			// important, if the server is created, then connection failed is false
			this->connectionFailed = false;

			this->networkSzenario = true;

			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[RakNetModule] Finished: Creating server...");
		}
	}

	bool RakNetModule::findServerAndCreateClient(void)
	{
		/*Advantages:

		* You can join games automatically on program startup, no GUI or user interaction required
		* Best way to find games on the LAN

		Disadvantages:

		* Won't work for the general internet
		* Not as flexible as the lightweight database plugin*/

		bool serverFound = false;

		// Connecting the client is very simple.  0 means we don't care about
		// a connectionValidationInteger, and false for low priority threads
		// All 255's mean broadcast
		// ping the server, to transmit the ip-address to the client
		this->rakPeer->Ping("255.255.255.255", this->serverPort, false);

		RakNet::TimeMS quitTime;
		RakNet::Packet* packet = nullptr;
		// try for 300ms to ping the server
		quitTime = RakNet::GetTimeMS() + 300;

		// wait for the ip-address of the server, to connect to it
		while (RakNet::GetTimeMS() < quitTime)
		{
			packet = this->rakPeer->Receive();
			if (!packet)
			{
				RakSleep(30);
				continue;
			}
			//uint32_t msgNumber;
			//memcpy(&msgNumber, packet->data /*+5*/, 4);
			//Core::getSingletonPtr()->getLog()->logMessage("Paket Nummer: " + Ogre::StringConverter::toString(msgNumber));
			//[0] is the first byte!
			if (packet->data[0] == ID_UNCONNECTED_PONG)
			{
				RakNet::TimeMS time;

				RakNet::BitStream bsIn(packet->data, packet->length, false);
				bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
				bsIn.Read(time);

				std::string serverIPandPort = std::string(packet->systemAddress.ToString());
				// get the server address
				this->serverAddress = packet->systemAddress;
				size_t pos = serverIPandPort.find("|");    // 192.168.0.163:1163
				this->serverIP = serverIPandPort.substr(0, pos);   //  192.168.0.163

				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[RakNetModule] duration " + Ogre::StringConverter::toString(RakNet::GetTimeMS() - time) + " ms");

				// look at this address (char*)packet->data + sizeof(RakNet::MessageID) + sizeof(RakNet::TimeMS)
				// serverData = GameServer1:World1-BoidsWorld
				std::string serverData = (char*)packet->data + sizeof(RakNet::MessageID) + sizeof(RakNet::TimeMS);

				pos = serverData.find(":");
				// set the servername and the map name for the client
				this->serverName = serverData.substr(0, pos);   //e.g. GameServer1

				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[RakNetModule] server ip: " + this->serverIP + " server name: " + this->serverName);

				size_t pos2 = serverData.find("-");
				this->projectName = serverData.substr(pos + 1, pos2 - pos - 1); //e.g. World1
				//this->foundServerLabel->setCaption("pos: " + Ogre::StringConverter::toString(pos+1) + " pos2: " +  Ogre::StringConverter::toString(pos2));
				size_t pos3 = serverData.find(";");
				this->worldName = serverData.substr(pos2 + 1, pos3 - pos2 - 1); //e.g. BoidsWorld
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[RakNetModule] World: " + this->worldName);

				this->localAddress = this->rakPeer->GetInternalID();
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[RakNetModule] local client system address: " + Ogre::String(this->localAddress.ToString()));

				serverFound = true;

			}
			else
			{
				this->serverName = "";
				this->projectName = "";
				this->worldName = "";
				serverFound = false;
			}
			this->rakPeer->DeallocatePacket(packet);
		}

		return serverFound;
	}

	void RakNetModule::startNetworking(const Ogre::String& windowTitle)
	{
		this->windowTitle = windowTitle;
		Core::getSingletonPtr()->changeWindowTitle(this->windowTitle);

		if (this->server)
		{
			if (this->isServerInConsole())
			{
				Core::getSingletonPtr()->moveWindowToTaskbar();
				//AnimateWindow(handle, 200, AW_BLEND);
			}
		}
		else
		{
			// later use this
			// this->rakPeer->SetTimeoutTime(30000, RakNet::UNASSIGNED_SYSTEM_ADDRESS);
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[RakNetModule] start connecting client to server: "
				+ this->serverIP + "|" + Ogre::StringConverter::toString(this->serverPort));
			RakNet::ConnectionAttemptResult result = this->rakPeer->Connect(this->serverIP.c_str(), this->serverPort, 0, 0, 0);

			if (RakNet::INVALID_PARAMETER == result)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[RakNetModule] Could not connect client to server because of an invalid parameter");
				return;
			}
			else if (RakNet::CANNOT_RESOLVE_DOMAIN_NAME == result)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[RakNetModule] Could not connect client to server because the domain name could not be resolved");
				return;
			}
			else if (RakNet::ALREADY_CONNECTED_TO_ENDPOINT == result)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[RakNetModule] Could not connect client to server because the endpoint is already connected");
				return;
			}
			else if (RakNet::CONNECTION_ATTEMPT_ALREADY_IN_PROGRESS == result)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[RakNetModule] Could not connect client to server because the attempt is already in progress");
				return;
			}
			else if (RakNet::SECURITY_INITIALIZATION_FAILED == result)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[RakNetModule] Could not connect client to server because the security initialisation failed");
				return;
			}
		}

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[RakNetModule] Activating replica manager");
		this->rakPeer->AttachPlugin(&this->appReplicaManager);
		this->appReplicaManager.SetNetworkIDManager(&this->networkIdManager);

		// update with packet send rate
		this->appReplicaManager.SetAutoSerializeInterval(this->packetSendRate);

		// the clients may not be created directly, because its to early and the virtual environment may not be loaded, 
		// so push the connections manually. The client sends ready message to server, the server gets ready for replication and
		// sents back to the client, that the client can start with replication
		this->appReplicaManager.SetAutoManageConnections(false, true);
	
		//this->writeStatistics();
	}

	void RakNetModule::sendPlayerInformationToClient(RakNet::Packet* packet)
	{
		RakNet::BitStream bitstream;

		// check if there is a free id for the next player, that wants to connect
		if (this->clients.size() >= this->allowedPlayerCount)
		{
			// if all spaces are occupied, send no free space
			bitstream.Write((RakNet::MessageID)RakNetModule::ID_NO_FREE_SPACE);
			this->rakPeer->Send(&bitstream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, false);
		}
		else
		{
			// determine the next free player id
			this->nextFreePlayerID = 1;
			for (auto& it = this->clients.cbegin(); it != clients.cend(); ++it)
			{
				int occupiedID = it->first;
				if (this->nextFreePlayerID == occupiedID)
				{
					this->nextFreePlayerID++;
				}

			}
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[RakNetModule:Server] Sending player id: "
				+ Ogre::StringConverter::toString(this->nextFreePlayerID) + " to client: " + Ogre::String(packet->systemAddress.ToString()));

			this->clients.insert(std::make_pair(this->nextFreePlayerID, packet->systemAddress));
			Ogre::String strSystemAddress = packet->systemAddress.ToString();

			// evil, but necessary when "Server" == replicationMethod: If the player should be replicated by the server, the server only knows here, which client wants to connect
			// but he loaded its game objects and world already, so the replication reference must be done here
			// so first get all players
			
			// check if the player for the to be connected client already exists on the server (the case when a client disconnects and reconnects again, to get the other game object player)
			// e.g. C1, C2 connecting to S. C1 gets P1 from S and P2. C2 gets P2 and P1 from S. C2 disconnects. P2 will be deleted on S and C1. C2 reconnects to S.
			// P1 already exists on server and must not be created again, but P2 must be created and also send to C1 and C2.
			auto playersControlledByClient = AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->getGameObjectsControlledByClientId(static_cast<unsigned int>(this->nextFreePlayerID));
			if (playersControlledByClient.empty())
			{
				// trigger the parse game objects event, so that all game objects that are controlled by this player id will be loaded
				// "" means, that not a specific game object will be parsed
				boost::shared_ptr<NOWA::EventDataParseGameObject> parseGameObjectsEvent(new NOWA::EventDataParseGameObject("", static_cast<unsigned int>(this->nextFreePlayerID)));
				NOWA::AppStateManager::getSingletonPtr()->getEventManager(this->appStateName)->triggerEvent(parseGameObjectsEvent);
			}
			playersControlledByClient = AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->getGameObjectsControlledByClientId(static_cast<unsigned int>(this->nextFreePlayerID));
			for (unsigned int i = 0; i < static_cast<unsigned int>(playersControlledByClient.size()); i++)
			{
				auto distributedCompPtr = NOWA::makeStrongPtr(playersControlledByClient[i]->getComponent<DistributedComponent>());
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[RakNetModule] Server references distributed component for client id: "
					+ Ogre::StringConverter::toString(this->nextFreePlayerID) + " and game object name: " + playersControlledByClient[i]->getName()
					+ " system address: " + Ogre::String(packet->systemAddress.ToString()));
				// set replication data
				distributedCompPtr->setClientID(this->nextFreePlayerID);
				distributedCompPtr->systemAddress = packet->systemAddress;
				distributedCompPtr->strSystemAddress = strSystemAddress;
				//if (0 != this->areaOfInterest /*&& 0 != this->gameObjectPtr->getControlledByClientID()*/)
				//{
				//	distributedCompPtr->createSphereSceneQuery();
				//}
				
				// distributedCompPtr->SetNetworkID(distributedPlayers[i]->getId());
				// AppStateManager::getSingletonPtr()->getRakNetModule(this->appStateName)->remoteComponents.Insert(distributedCompPtr.get(), _FILE_AND_LINE_);

				if (!this->remoteComponents.Has(distributedCompPtr->getClientID()))
				{
					DataStructures::List<DistributedComponent*> components;
					components.Insert(distributedCompPtr.get(), _FILE_AND_LINE_);

					this->remoteComponents.SetNew(distributedCompPtr->getClientID(), components);
				}
				else
				{
					auto remoteComponents = this->remoteComponents.Get(distributedCompPtr->getClientID());
					remoteComponents.Insert(distributedCompPtr.get(), _FILE_AND_LINE_);
				}

				AppStateManager::getSingletonPtr()->getRakNetModule()->appReplicaManager.Reference(distributedCompPtr.get());
			}

			bitstream.Write((RakNet::MessageID)RakNetModule::ID_PLAYERASSIGNMENT_PACKET);
			bitstream.Write(this->nextFreePlayerID);
			bitstream.Write(this->areaOfInterest);
			bitstream.Write(this->packetSendRate);
			RakNet::StringTable::Instance()->EncodeString(packet->systemAddress.ToString(), 255, &bitstream);

			// send data with high priority, reliable (TCP), 0, IP + Port, do not broadcast but just send to the client which attempted a connection
			this->rakPeer->Send(&bitstream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, false);
		}
	}

	void RakNetModule::receivePlayerInformation(RakNet::Packet* packet)
	{
		RakNet::BitStream bitstream(packet->data, packet->length, false);

		// ignore ID_GAME_MESSAGE_1
		bitstream.IgnoreBytes(sizeof(RakNet::MessageID));

		int nextFreePlayerID = 0;
		unsigned int areaOfInterest;
		unsigned int packetSendRate;

		// each client receives an unique player id from the server
		bitstream.Read(nextFreePlayerID);
		bitstream.Read(areaOfInterest);
		bitstream.Read(packetSendRate);

		this->nextFreePlayerID = nextFreePlayerID;
		this->areaOfInterest = areaOfInterest;
		this->packetSendRate = packetSendRate;

		char strSystemAddress[255];
		RakNet::StringTable::Instance()->DecodeString(strSystemAddress, 255, &bitstream);

		this->clients.insert(std::make_pair(this->nextFreePlayerID, RakNet::SystemAddress(strSystemAddress)));

		char strID[4];
		sprintf(strID, "%d", this->nextFreePlayerID);
		Ogre::String strTitle = "Client_" + Ogre::String(strID) + "_" + this->playerName;
		// adjust window title in order to see which id which player has
		NOWA::Core::getSingletonPtr()->changeWindowTitle(strTitle.c_str());

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[RakNetModule:Client] receiving player ID: "
			+ Ogre::StringConverter::toString(this->nextFreePlayerID) + " and address" + strSystemAddress + " assigned from server");

		this->connectionFailed = false;
	}

	void RakNetModule::receiveClientExit(RakNet::Packet* packet)
	{
		RakNet::BitStream bitstream(packet->data, packet->length, false);

		// ignore ID_GAME_MESSAGE_1
		bitstream.IgnoreBytes(sizeof(RakNet::MessageID));

		int clientID = -1;
		bitstream.Read(clientID);
		// this->nextFreePlayerID = clientID;
		this->clients.erase(clientID);

		// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[RakNetModule:Server] Getting exit from remote client Id: "
		// 	+ Ogre::StringConverter::toString(this->nextFreePlayerID));

		RakNet::BitStream bitstream2;

		// send to the remaining clients, that one client is no langer available, so that they can remove the client from the list
		for (auto& it = this->clients.cbegin(); it != this->clients.cend(); ++it)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[RakNetModule:Server] send exit for remote client Id: "
				+ Ogre::StringConverter::toString(clientID));

			bitstream2.Write((RakNet::MessageID)RakNetModule::ID_REMOTE_CLIENT_EXIT);
			RakNet::StringTable::Instance()->EncodeString(packet->systemAddress.ToString(), 255, &bitstream2);
			bitstream2.Write(clientID);

			// send data with high priority, reliable (TCP), 0, IP + Port, do not broadcast but just send to the client which attempted a connection
			AppStateManager::getSingletonPtr()->getRakNetModule()->getRakPeer()->Send(&bitstream2, HIGH_PRIORITY, RELIABLE_ORDERED, 0, it->second, false);
		}
	}

	void RakNetModule::update(Ogre::Real dt)
	{
		if (nullptr == this->rakPeer)
		{
			return;
		}

		// Attention: in this function, only packet ids may be received, if the connection progress in a early stage!
		// For custom messages during game use server::update(...) function!
		RakNet::Packet* packet;
		for (packet = this->rakPeer->Receive(); packet; this->rakPeer->DeallocatePacket(packet), packet = this->rakPeer->Receive())
		{
			// the first byte is the packet id (message-id)
			unsigned char packetID = this->getPacketIdentifier(packet);
			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[RakNetModule] packetID: " + Ogre::StringConverter::toString(packetID));
			switch (packetID)
			{
			case ID_CONNECTION_ATTEMPT_FAILED:
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[RakNetModule] ID_CONNECTION_ATTEMPT_FAILED from Client");
				this->connectionFailed = true;
				break;
			case ID_NO_FREE_INCOMING_CONNECTIONS:
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[RakNetModule] ID_NO_FREE_INCOMING_CONNECTIONS from Client");
				this->connectionFailed = true;
				break;
			case ID_CONNECTION_REQUEST_ACCEPTED:
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[RakNetModule] ID_CONNECTION_REQUEST_ACCEPTED from Client");
				this->connectionFailed = false;
				break;
			case ID_DISCONNECTION_NOTIFICATION:
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[RakNetModule] Client ID_DISCONNECTION_NOTIFICATION from Client");
				break;
			case ID_CONNECTION_LOST:
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[RakNetModule] ID_CONNECTION_LOST from Client");
				break;
				//Neue Verbindungsanfrage vom Client, der Server sendet dem Client daraufhin wichtige Daten
			case ID_NEW_INCOMING_CONNECTION:
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[RakNetModule] ID_NEW_INCOMING_CONNECTION from " + Ogre::String(packet->systemAddress.ToString()));
				break;
			case RakNetModule::ID_PLAYERASSIGNMENT_PACKET:
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[RakNetModule] ID_PLAYERASSIGNMENT_PACKET from " + Ogre::String(packet->systemAddress.ToString()));
				if (!this->server)
				{
					this->receivePlayerInformation(packet);
				}
				break;
			case RakNetModule::ID_NO_FREE_SPACE:
				// no free space for the client (map is full)
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[RakNetModule] ID_NO_FREE_SPACE in virtual environment");
				this->connectionFailed = true;
				break;
			case ID_SND_RECEIPT_ACKED:
			case ID_SND_RECEIPT_LOSS:
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[RakNetModule] ID_SND_RECEIPT_LOSS from Client");
				////this->NotifyReplicaOfMessageDeliveryStatus(packet);
				break;
			}
		}
	}

	void RakNetModule::setConnectionFailed(bool connectionFailed)
	{
		this->connectionFailed = connectionFailed;
	}

	bool RakNetModule::isConnectionFailed(void) const
	{
		return this->connectionFailed;
	}

	unsigned char RakNetModule::getPacketIdentifier(RakNet::Packet* packet)
	{
		// Attention: RakNet::GetTime() will be transformed depending on the operating system(32, 64 bit) the time either in 4 (unsigned int) or 8 (unsigned long long) byte
		// RakNet::Time has 8 byte, but RakNet::GetTimeMS() uses always 4 byte
		// Therefore RakNet::Time is used and Replica does not know timestamps via NTP?
		if ((unsigned char)packet->data[0] == ID_TIMESTAMP)
			return (unsigned char)packet->data[sizeof(unsigned char) + sizeof(RakNet::Time)];
		else
			return (unsigned char)packet->data[0];
	}

	unsigned int RakNetModule::connectionCount(void) const
	{
		unsigned int i;
		unsigned int count = 0;

		for (i = 0; i < this->allowedPlayerCount; i++)
		{
			if (this->rakPeer->GetSystemAddressFromIndex(i) != RakNet::UNASSIGNED_SYSTEM_ADDRESS)
			{
				count++;
			}
		}
		return count;
	}

	RakNet::RakPeerInterface* RakNetModule::getRakPeer(void) const
	{
		return this->rakPeer;
	}

	void RakNetModule::setServerIP(const Ogre::String& serverIP)
	{
		this->serverIP = serverIP;
	}

	Ogre::String RakNetModule::getServerIP(void) const
	{
		return this->serverIP;
	}

	unsigned short RakNetModule::getServerPort(void) const
	{
		return this->serverPort;
	}

	void RakNetModule::setServerName(const Ogre::String& serverName)
	{
		this->serverName = serverName;
	}

	Ogre::String RakNetModule::getServerName(void) const
	{
		return this->serverName;
	}

	void RakNetModule::setPlayerName(const Ogre::String& playerName)
	{
		this->playerName = playerName;
	}

	Ogre::String RakNetModule::getPlayerName(void) const
	{
		return this->playerName;
	}

	void RakNetModule::addRemotePlayerName(unsigned int clientID, const Ogre::String& remotePlayerName)
	{
		this->remotePlayerNames.insert(std::make_pair(clientID, remotePlayerName));
	}

	Ogre::String RakNetModule::getRemotePlayerName(unsigned int index)
	{
		return this->remotePlayerNames[index];
	}

	int RakNetModule::getClientID(void) const
	{
		// server has not client id
		if (this->server)
		{
			return -1;
		}
		else
		{
			return this->nextFreePlayerID;
		}
	}

	RakNet::SystemAddress RakNetModule::getServerAddress(void) const
	{
		return this->serverAddress;
	}

	RakNet::SystemAddress RakNetModule::getLocalAddress(void) const
	{
		return this->localAddress;
	}

	void RakNetModule::setPacketSendRateMS(unsigned int packetSendRate)
	{
		this->packetSendRate = packetSendRate;
	}

	unsigned int RakNetModule::getPacketSendRateMS(void) const
	{
		return this->packetSendRate;
	}

	void RakNetModule::setInterpolationTimeMS(unsigned int interpolationTime)
	{
		this->interpolationTime = interpolationTime;
	}

	unsigned int RakNetModule::getInterpolationTime(void) const
	{
		return this->interpolationTime;
	}

	void RakNetModule::setAreaOfInterest(unsigned int areaOfInterest)
	{
		this->areaOfInterest = areaOfInterest;
	}

	unsigned int RakNetModule::getAreaOfInterest(void) const
	{
		return this->areaOfInterest;
	}

	unsigned int RakNetModule::getHighestLatencyToServer(void) const
	{
		return this->highestLatencyToServer;
	}

	Ogre::String RakNetModule::getWindowTitle(void) const
	{
		return this->windowTitle;
	}

	std::map<Ogre::String, Ogre::String> RakNetModule::getWorldNames(void) const
	{
		// <worldName, projectName>
		return this->worldNames;
	}

	void RakNetModule::setProjectName(const Ogre::String& projectName)
	{
		this->projectName = projectName;
	}

	Ogre::String RakNetModule::getProjectName(void) const
	{
		return this->projectName;
	}

	void RakNetModule::setWorldName(const Ogre::String& worldName)
	{
		this->worldName = worldName;
	}

	Ogre::String RakNetModule::getWorldName(void) const
	{
		return this->worldName;
	}

	bool RakNetModule::isServer(void) const
	{
		return this->server;
	}

	bool RakNetModule::hasClientConnectionToServer(void) const
	{
		return this->clientConnectionToServer;
	}

	void RakNetModule::setServerInConsole(bool serverInConsole)
	{
		this->serverInConsole = serverInConsole;
	}

	bool RakNetModule::isServerInConsole(void) const
	{
		return this->serverInConsole;
	}

	int RakNetModule::getClientIDForSystemAddress(const RakNet::SystemAddress& systemAddress)
	{
		int clientID = -1;
		for (auto& it = this->clients.cbegin(); it != this->clients.cend(); ++it)
		{
			if (systemAddress == it->second)
			{
				return it->first;
			}
		}
		return clientID;
	}

	unsigned int RakNetModule::getAllowedPlayerCount(void) const
	{
		return this->allowedPlayerCount;
	}

	bool RakNetModule::isNetworkSzenario(void) const
	{
		return this->networkSzenario;
	}

	RakNet::RakNetGUID RakNetModule::getGuid(void) const
	{
		return this->guid;
	}

	std::map<Ogre::String, Ogre::String> RakNetModule::parseWorlds(const Ogre::String& section, const Ogre::String& resourcesFile)
	{
		// clear everything first
		this->worldName = "";
		this->projectName = "";
		for (std::map<Ogre::String, Ogre::String>::iterator it = this->worldNames.begin(); it != this->worldNames.end(); ++it)
		{
			(*it).second = "";
		}

		//secName = World
		//typeName = FileSystem
		//archName = ../media/BoidsWorld
		Ogre::String secName;
		//Ogre::String typeName;
		Ogre::String archName;

		Ogre::ConfigFile cf;
		// load the resrouces
		cf.load(resourcesFile);
		// go through all sections and subsections
		Ogre::ConfigFile::SectionIterator seci = cf.getSectionIterator();
		int index = 0;
		while (seci.hasMoreElements())
		{
			secName = seci.peekNextKey();
			Ogre::ConfigFile::SettingsMultiMap* pSettings = seci.getNext();

			//if (secName == "World")
			// all ressource groups must start with the postfix "World" (e.g. World_A, World_1, ...)
			if (Ogre::StringUtil::startsWith(secName, section, false))
			{
				Ogre::ConfigFile::SettingsMultiMap::const_iterator i;
				for (i = pSettings->cbegin(); i != pSettings->cend(); ++i)
				{
					++index;

					archName = i->second;
					Ogre::String projectName = Core::getSingletonPtr()->getProjectNameFromPath(archName);
					if (true == projectName.empty() || "media" == projectName)
					{
						continue;
					}
					Ogre::String sceneName;

					std::size_t found = archName.rfind('/');
					if (found != Ogre::String::npos)
					{
						sceneName = archName.substr(found + 1, archName.length() - found);
					}

					///TODO: Due to project1/worldx new system is required
					//e.g. [BoidsWorld (Mapname)] = World1 (Ressourcegroup)

					// Key must not be equal to value (Projects = Projects)
					if (sceneName != secName)
					{
						this->worldNames[sceneName] = projectName;
					}
				}
			}
		}
		return this->worldNames;
	}

	void RakNetModule::preParseWorld(const Ogre::String& resourceGroup, const Ogre::String& projectName, const Ogre::String& worldName)
	{
		this->clients.clear();
		this->allowedPlayerCount = 0;

		// get the xml file from the resourcegroup
		Ogre::DataStreamPtr stream = Ogre::ResourceGroupManager::getSingleton().openResource(projectName + "/" + worldName + ".scene", resourceGroup);

		char* scene = _strdup(stream->getAsString().c_str());

		rapidxml::xml_document<> XMLDoc;
		// parse the xml document
		XMLDoc.parse<0>(scene);

		// go to the node, at which the scene nodes occur
		rapidxml::xml_node<>* XMLRoot = XMLDoc.first_node("scene");
		rapidxml::xml_node<>* nodesElement = XMLRoot->first_node("nodes");
		rapidxml::xml_node<>* nodeElement = nodesElement->first_node("node");
		// go through all nodes
		while (nodeElement)
		{
			// search for the subnode
			rapidxml::xml_node<>* entityElement = nodeElement->first_node("entity");
			if (entityElement)
			{
				rapidxml::xml_node<>* userDataElement = entityElement->first_node("userData");
				if (userDataElement)
				{
					rapidxml::xml_node<>* propertyElement = userDataElement->first_node("property");

					Ogre::String value;

					while (propertyElement && value != "ControlledByClient")
					{
						propertyElement = propertyElement->next_sibling("property");
						value = propertyElement->first_attribute("name")->value();
					}
					// if there is no ControlledByClient property, then all property elements had been visited and the element is still null,
					// so go to the next node
					if (!propertyElement)
					{
						nodeElement = nodeElement->next_sibling("node");
						continue;
					}
					// propertyElement = propertyElement->next_sibling("property");
					// if the attributes: name="Object" data="DistributedGameObject" are found, get the start position
					int controlledByClientID = static_cast<int>(XMLConverter::getAttribReal(propertyElement, "data", 0));
					// if (categoryType == Ogre::String("Player"))
					if (0 != controlledByClientID)
					{
						this->allowedPlayerCount++;
					}
				}
			}
			// go to the next node
			nodeElement = nodeElement->next_sibling("node");
		}
		free(scene);
	}

	std::map<Ogre::String, Ogre::String>::iterator RakNetModule::findProject(const Ogre::String& mapName)
	{
		return this->worldNames.find(mapName);
	}

	/*//Serverstatistik herausschreiben
	void Server::writeStatistics(void)
	{
	NOWA::Core::getSingletonPtr()->rakPeer->AttachPlugin(&this->messageHandler);
	this->messageHandler.StartLog("ServerStatistics.txt");
	this->messageHandler.LogHeader();
	}*/

	/*void RakNetModule::insertLatencyOnServerInClientList(Character* pCurrentCharacter)
	{
	//Rubrik aus Liste erhalten
	Ogre::String strItem = Core::getSingletonPtr()->pParamsPanel->getParamValue(pCurrentCharacter->getClientID());
	//Zeichen suchen: latenz/ und ersetzen durch den Wert latencyToServer und eintragen
	Ogre::String strSubItem = strItem.replace(strItem.find("/", strItem.size() - 12) + 1, strItem.size(), Ogre::StringConverter::toString((unsigned long)pCurrentCharacter->latencyToServer));
	NOWA::Core::getSingletonPtr()->pParamsPanel->setParamValue(pCurrentFlubber->getClientID(), strSubItem);
	//NOWA::Core::getSingletonPtr()->getMyDebugLog()->logMessage("LatencyToServer: " + Ogre::StringConverter::toString(pCurrentFlubber->latencyToServer));
	}*/

} // namespace end
#ifndef RAK_NET_MODULE_H
#define RAK_NET_MODULE_H

//#define __GET_TIME_64BIT 0

#include <defines.h>

// Network
#include <RakPeerInterface.h>
#include <GetTime.h>
#include <StringTable.h>
#include <RakSleep.h>
#include <RakAssert.h>
#include <MessageIdentifiers.h>
#include <ReplicaManager3.h>
#include <NetworkIDManager.h>
#include <DS_MAP.h>

#include "network/AppConnectionRM3.h"

namespace NOWA
{
	class RakAssert;
	class BitStream;
	
	class EXPORTED RakNetModule
	{
	public:
		friend class AppState; // Only AppState may create this class

		void destroyContent(void);

		/**
		 * @brief		Creates the RakNet interface for networking for the client
		 * @return		rakPeer		The RakNet interface
		 */
		void createRakNetForClient(void);

		/**
		 * @brief		Creates the RakNet interface for networking for the server
		 * @param[in]	serverName	The name of the server
		 */
		void createRakNetForServer(const Ogre::String& serverName);

		bool findServerAndCreateClient(void);

		/**
		* @brief		Starts the networking either for the server or for the client
		* @param[in]	windowTitle	The window title to distinguish who the particpant is
		*/
		void startNetworking(const Ogre::String& windowTitle);

		/**
		* @brief		Sends the player information (id etc.) to the client
		* @param[in]	packet	The packet coming from the server that holds the information
		*/
		void sendPlayerInformationToClient(RakNet::Packet* packet);

		/**
		* @brief		Receives the player information (id etc.) from the server
		* @param[in]	packet	The packet coming from the server that holds the information
		*/
		void receivePlayerInformation(RakNet::Packet* packet);

		/**
		* @brief		Updates the module
		* @param[in]	dt	The the delta time in seconds (a simulation running with 60fps, produces a dt of 0.016 seconds each tick)
		*/
		void update(Ogre::Real dt);
		
		/**
		* @brief		Sets wheather the connection to the server has failed.
		* @param[in]	connectionFailed	Wheather the connection has failed or not
		*/
		void setConnectionFailed(bool connectionFailed);

		/**
		* @brief		Gets whether the connection is failed
		* @return		connectionFailed
		*/
		bool isConnectionFailed(void) const;

		/**
		 * @brief		Gets the packet identifier
		 * @param[in]	packet			The packet to get the identifier
		 * @return		identifer		The identifier
		 * @note		If an timestamp has been dispatched, this timestamp gets extracted to get the message id
		 */
		unsigned char getPacketIdentifier(RakNet::Packet* packet);

		/**
		 * @brief		Gets the count, of how many connections are established at the moment.
		 * @return		count	The connection count
		 */
		unsigned int connectionCount(void) const;

		RakNet::RakPeerInterface* getRakPeer(void) const;

		/**
		 * @brief		Sets the server IP manually
		 * @param[in]	serverIP		The server IP
		 * @note		This is the case in a distributed internet scenario
		 */
		void setServerIP(const Ogre::String& serverIP);

		/**
		* @brief		Gets the server ip
		* @return		serverIP	The server ip
		*/
		Ogre::String getServerIP(void) const;

		/**
		* @brief		Gets the server port
		* @return		port	The server port
		*/
		unsigned short getServerPort(void) const;

		/**
		* @brief		Sets the server name
		* @param[in]	serverName		The server name
		*/
		void setServerName(const Ogre::String& serverName);
		
		/**
		* @brief		Gets the server name
		* @return		serverName	The server name
		*/
		Ogre::String getServerName(void) const;

		/**
		* @brief		Sets the player name
		* @param[in]	playerName		The player name
		*/
		void setPlayerName(const Ogre::String& playerName);

		/**
		* @brief		Gets the player name
		* @return		playerName	The player name
		*/
		Ogre::String getPlayerName(void) const;

		void addRemotePlayerName(unsigned int clientID, const Ogre::String& remotePlayerName);

		Ogre::String getRemotePlayerName(unsigned int clientID);

		/**
		* @brief		Gets the player id
		* @return		playerId	The player id
		*/
		int getClientID(void) const;

		/**
		* @brief		Gets the server system address: ip|port
		* @return		serverAddress	The local server address
		*/
		RakNet::SystemAddress getServerAddress(void) const;

		/**
		* @brief		Gets the local system address: ip|port
		* @return		localAddress	The local system address
		*/
		RakNet::SystemAddress getLocalAddress(void) const;

		/**
		* @brief		Sets the paket send rate in ms
		* @param[in]	packetSendRate	The paket send rate
		*/
		void setPacketSendRateMS(unsigned int packetSendRate);

		/**
		* @brief		Gets the paket send rate in ms
		* @return		packetSendRate	The paket send rate
		*/
		unsigned int getPacketSendRateMS(void) const;

		/**
		* @brief		Sets the interpolation time span in ms
		* @param[in]	interpolationTime	The interpolation time
		*/
		void setInterpolationTimeMS(unsigned int interpolationTime);

		/**
		* @brief		Gets the interpolation time span in ms
		* @return		interpolationTime	The interpolation time
		*/
		unsigned int getInterpolationTime(void) const;

		/**
		* @brief		Sets the area of interest in meters. This is the radius, in which remote player can effect the local player.
		*				The local player gets paket updates from all remote player within this area
		* @param[in]	areaOfInterest	The area of interest
		*/
		void setAreaOfInterest(unsigned int areaOfInterest);

		/**
		* @brief		Gets the area of interest in meters
		* @return		areaOfInterest	The area of interest
		*/
		unsigned int getAreaOfInterest(void) const;

		/**
		* @brief		Gets the highest latency to server in ms
		* @return		highestLatencyToServer	The highest latency to server
		*/
		unsigned int getHighestLatencyToServer(void) const;

		/**
		* @brief		Gets the window title
		* @return		windowTitle	The window title
		*/
		Ogre::String getWindowTitle(void) const;

		/**
		* @brief		Gets the scene names
		* @return		sceneNames	The scene names
		* @note			The format of the data structure is: <sceneName, projectName>
		*/
		std::map<Ogre::String, Ogre::String> getSceneNames(void) const;

		/**
		* @brief		Sets the project name
		* @param[in]	projectName	The project to set
		* @note			This can be used to form the project prefix (like Network), combining with the virtual environment name,
		*				so that the client load the map (e.g. Network-Scene1)
		*/
		void setProjectName(const Ogre::String& projectName);

		/**
		* @brief		Gets the project name
		* @return		projectName	The project name to get
		*/
		Ogre::String getProjectName(void) const;

		/**
		* @brief		Sets the scene name
		* @param[in]	sceneName	The scene name to set
		* @note			This can be used to form the project prefix (like Network), combining with the virtual environment name,
		*				so that the client load the map (e.g. Network-Scene1)
		*/
		void setSceneName(const Ogre::String& sceneName);

		/**
		* @brief		Gets the scene name
		* @return		sceneName	The scene name to get
		*/
		Ogre::String getSceneName(void) const;

		/**
		* @brief		Gets whether this application is the server or not. This query can be used, to perform code for a client application or server application
		* @return		server	Whether this is the server or not
		*/
		bool isServer(void) const;

		/**
		* @brief		Gets whether the client has a connection to the server or not
		* @return		hasConnection
		*/
		bool hasClientConnectionToServer(void) const;

		/**
		* @brief		Sets the server to console, that means that the server application will not be rendered, just a console will be shown
		* @param[in]	serverInConsole	The server in console or not
		* @note			This can be used to save performance, when your application is almost finished and you do not need to analyze what the server sees
		*/
		void setServerInConsole(bool serverInConsole);

		/**
		* @brief		Gets whether the server is in console or not
		* @return		isInConsole
		*/
		bool isServerInConsole(void) const;

		/**
		* @brief		Parses all scenes from a section and delivers a map
		* @param[in]	section	The section to parse
		* @return		scenes	The scenes in the format <sceneName, projectName>
		*/
		std::map<Ogre::String, Ogre::String> parseScenes(const Ogre::String& section, const Ogre::String& resourceName = "resources.cfg");

		/**
		* @brief		Pre parses a scene from a resource group and filename to find out, how many clients can participate
		* @param[in]	resourceGroup	The resource group for parsing
		* @param[in]	projectName		The project name
		* @param[in]	sceneName		The scene name
		*/
		void preParseScene(const Ogre::String& resourceGroup, const Ogre::String& projectName, const Ogre::String& sceneName);

		/**
		* @brief		Finds the project from the given scene name
		* @param[in]	sceneName	The scene name to set
		* @return		iterator	The iterator from a scene name
		*/
		std::map<Ogre::String, Ogre::String>::iterator findProject(const Ogre::String& sceneName);

		/**
		* @brief		Gets client start data
		* @return		clientStartData	The client start data structure
		*/
		std::map<unsigned int, RakNet::SystemAddress> RakNetModule::getClients(void) const
		{
			return this->clients;
		}

		int getClientIDForSystemAddress(const RakNet::SystemAddress& systemAddress);

		unsigned int getAllowedPlayerCount(void) const;

		void receiveClientExit(RakNet::Packet* packet);

		bool isNetworkSzenario(void) const;

		RakNet::RakNetGUID getGuid(void) const;
	public:
	// Attention: i need a way, so that the developer can extend this enumeration by its own types and use them!
		enum eGameMessages
		{
			// Attention: many id's are occupied, hence start with the ID_USER_PACKET_ENUM + 1 id
			ID_PLAYERASSIGNMENT_PACKET = ID_USER_PACKET_ENUM + 1,
			ID_PLAYERINFORMATION_PACKET, /// is not used
			ID_NO_FREE_SPACE,
			ID_PLAYER_MOVE_CONFIRMATION_PACKET,
			//ID_PLAYER_MOVESTOP_CONFIRMATION_PACKET,
			ID_SERVER_MOVED_REMOTEPLAYERS_RESULT_PACKET,
			ID_CHAT_MESSAGE_PACKET,
			ID_OBJECT_MOVEGAMEEVENT_PACKET,
			ID_CLIENT_MOVEGAMEEVENT_PACKET,
			ID_CLIENT_JUMPGAMEEVENT_PACKET,
			ID_CLIENT_SHOOTGAMEEVENT_PACKET,
			ID_SERVER_MOVEDGAMEEVENT_PACKET,
			ID_SERVER_JUMPEDGAMEEVENT_PACKET,
			ID_SERVER_SHOTGAMEEVENT_PACKET,
			ID_SERVER_SHOTHITGAMEEVENT_PACKET,
			ID_SERVER_STARTGAME_PACKET,
			ID_SERVER_BALLDATA_PACKET,
			ID_SERVER_GAME_SCORE,
			ID_CLIENT_MOUSE_MOVE_EVENT_PACKET,
			ID_CLIENT_MOUSE_INPUT_EVENT_PACKET,
			ID_CLIENT_EXIT,
			ID_REMOTE_CLIENT_EXIT,
			ID_CLIENT_READY,
			ID_CLIENT_START_REPLICATION
		};
	public:
		NET::AppReplicaManager appReplicaManager;
		
		/*class EXPORTED RemoteComponent
		{
		public:
			DataStructures::List<DistributedComponent* > components;
		};
		DataStructures::List<RemoteComponent> remoteComponents;*/

		DataStructures::Map<int, DataStructures::List<DistributedComponent*>> remoteComponents;

		DataStructures::List<DistributedComponent*> localComponents;
	private:
		RakNetModule(const Ogre::String& appStateName);
		~RakNetModule();
	private:
		Ogre::String appStateName;
		/// RakNet network
		RakNet::RakPeerInterface* rakPeer;
		
		unsigned short serverPort;
		Ogre::String serverIP;
		Ogre::String serverName;
		Ogre::String playerName;
		std::map<unsigned int, Ogre::String> remotePlayerNames;
		unsigned int packetSendRate;
		unsigned int interpolationTime;
		unsigned int areaOfInterest;
		unsigned int highestLatencyToServer;
		int nextFreePlayerID;

		RakNet::SystemAddress serverAddress;
		RakNet::SystemAddress localAddress;
		
		//<sceneName, projectName>
		std::map<Ogre::String, Ogre::String> sceneNames;
		Ogre::String sceneName;
		Ogre::String projectName;
		bool server;
		bool clientConnectionToServer;
		bool serverInConsole;
		bool connectionFailed;

		/// Number of startpositions in the virtual environment (for networking)
		std::map<unsigned int, RakNet::SystemAddress> clients;

		RakNet::NetworkIDManager networkIdManager;
		Ogre::SceneManager*	sceneManager;
		Ogre::Camera* camera;
		Ogre::String windowTitle;

		bool statusActive;

		/*OgreBites::Label* chatMessageLabel;
		OgreBites::Label* latencyLabel;
		OgreBites::TextBox* chatMessageTextBox;*/
		bool chatTextboxActive;
		unsigned int allowedPlayerCount;

		bool networkSzenario;

		RakNet::RakNetGUID guid;
	};

} // namespace end

#endif
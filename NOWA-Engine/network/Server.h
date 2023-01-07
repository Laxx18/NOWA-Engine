#ifndef SERVER_H
#define SERVER_H

#include "ConnectionType.h"

namespace NOWA
{

	namespace NET
	{
		class EXPORTED Server : public ConnectionType
		{
		public:
			Server(Ogre::SceneManager* sceneManager, OgreNewt::World* ogreNewt);
			virtual ~Server();

			virtual void updatePackets(Ogre::Real dt) override;
		
			virtual void update(Ogre::Real dt) override;

			virtual void updatePhysics(Ogre::Real dt) override;

			virtual void disconnect(void) override;

			virtual void setupWidgets(void) override;

			virtual void updateGamestatesOfRemotePlayers(Ogre::Real dt) override;

			virtual void showStatus(bool bShow) override;

			virtual void writeStatistics(void) override;


			virtual bool keyPressed(const OIS::KeyEvent &keyEventRef) override;
			virtual bool keyReleased(const OIS::KeyEvent &keyEventRef) override;

			virtual bool mouseMoved(const OIS::MouseEvent &arg) override;
			virtual bool mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id) override;
			virtual bool mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id) override;

			virtual void processUnbufferedKeyInput(Ogre::Real dt) override;
			virtual void processUnbufferedMouseInput(Ogre::Real dt) override;
		private:
			
			// void sendPlayerInformationToClient(RakNet::Packet* packet);

			void simulateClientMovement(RakNet::Packet* packet, Ogre::Real dt);

			void simulateClientMoveGameEvent(RakNet::Packet* packet, Ogre::Real dt);

			void simulateClientJumpGameEvent(RakNet::Packet* packet);

			void simulateClientShootGameEvent(RakNet::Packet* packet);

			void readAndDispatchChatMessage(RakNet::Packet* packet);

			void NotifyReplicaOfMessageDeliveryStatus(RakNet::Packet* packet);

			//Latenz von Clients beim Server in die Clientliste eintragen
			// void insertLatencyOnServerInClientList(Character* pCurrentCharacter);
		private:
			//SpielerID des momentan behandelnden Spielers
			int	currentPlayerID;

			// OgreBites::SelectMenu* connectedClientsMenu;
			// OgreBites::ParamsPanel* paramsPanel;
		};

	} // namespace end NET

} // namespace end NOWA

#endif

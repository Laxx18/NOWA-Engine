#ifndef CLIENT_H
#define CLIENT_H

#include "ConnectionType.h"

namespace NOWA
{
	namespace NET
	{

		class EXPORTED Client : public ConnectionType
		{
		public:
			Client(Ogre::SceneManager* sceneManager, OgreNewt::World* ogreNewt);
			virtual ~Client();

			virtual void updatePackets(Ogre::Real dt) override;

			virtual void update(Ogre::Real dt) override;

			virtual void disconnect(void) override;

			virtual void updateGamestatesOfRemotePlayers(Ogre::Real dt) override;

			virtual void setupWidgets(void) override;

			virtual void showStatus(bool bShow) override;

			virtual void writeStatistics(void) override;

			void sendClientReadyForReplication(void) override;

			virtual bool keyPressed(const OIS::KeyEvent & keyEventRef) override;
			virtual bool keyReleased(const OIS::KeyEvent & keyEventRef) override;

			virtual bool mouseMoved(const OIS::MouseEvent & arg);
			virtual bool mousePressed(const OIS::MouseEvent & arg, OIS::MouseButtonID id) override;
			virtual bool mouseReleased(const OIS::MouseEvent & arg, OIS::MouseButtonID id) override;

			virtual void processUnbufferedKeyInput(Ogre::Real dt) override;
			virtual void processUnbufferedMouseInput(Ogre::Real dt) override;

			void sendMousePosition(const Ogre::Vector2& position);

			void sendMouseButtonState(unsigned short button, char pressed);
		private:
			
			// void sendChatMessage(Character *pSelectedFlubber);
			void simulateMovedGameEventFromServer(RakNet::Packet* packet);

			void simulateJumpedGameEventFromServer(RakNet::Packet* packet);

			void simulateShotGameEventFromServer(RakNet::Packet* packet);

			void simulateShotHitGameEventFromServer(RakNet::Packet* packet);

			void simulateGamestateFromServer(RakNet::Packet* packet, Ogre::Real dt);

			void readChatMessage(RakNet::Packet* packet);

			void selectPlayer(int mx, int my);
		private:
			/*OgreBites::Label* chatMessageLabel;
			OgreBites::Label* latencyLabel;
			OgreBites::TextBox* chatMessageTextBox;
			bool chatTextboxActive;*/
			Ogre::RaySceneQuery* raySceneQuery;
			// Character* pSelectedCharacter;
		};

	} // namespace end NET

} // namespace end NOWA

#endif

#ifndef CONNECTION_TYPE_H
#define CONNECTION_TYPE_H

#include <NetworkIDManager.h>

#include "defines.h"
#include "modules/RakNetModule.h"

namespace NOWA
{
	namespace NET
	{
		class EXPORTED ConnectionType
		{
		public:
			ConnectionType();
			ConnectionType(Ogre::SceneManager* sceneManager, OgreNewt::World* ogreNewt);

			virtual ~ConnectionType();

			virtual void sendClientReadyForReplication(void) { };

			virtual void updatePackets(Ogre::Real dt) = 0;

			virtual void updatePhysics(Ogre::Real dt) { };

			virtual void updateGamestatesOfRemotePlayers(Ogre::Real dt) = 0;

			virtual void update(Ogre::Real dt) = 0;

			virtual void disconnect(void) = 0;

			virtual void setupWidgets(void);

			virtual void showStatus(bool bShow);

			virtual void setupConnection(void);

			virtual void writeStatistics(void);

			virtual bool keyPressed(const OIS::KeyEvent &keyEventRef);
			virtual bool keyReleased(const OIS::KeyEvent &keyEventRef);

			virtual bool mouseMoved(const OIS::MouseEvent &arg);
			virtual bool mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
			virtual bool mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id);

			virtual void processUnbufferedKeyInput(Ogre::Real dt);
			virtual void processUnbufferedMouseInput(Ogre::Real dt);
		protected:
			bool statusActive;
			// RakNet::PacketFileLogger messageHandler;
			Ogre::SceneManager* sceneManager;
			OgreNewt::World* ogreNewt;
		};

		/****************************************************************/

		class EXPORTED NullConnectionType : public ConnectionType
		{
		public:
			NullConnectionType() : ConnectionType() { }

			virtual ~NullConnectionType() { }
			
			virtual void updatePackets(Ogre::Real dt) { };
			
			virtual void update(Ogre::Real dt) { };

			virtual void disconnect(void) { }
			
			virtual void updateGamestatesOfRemotePlayers(Ogre::Real dt) { };
			
			virtual void setupWidgets(void) { }
			
			virtual void showStatus(bool bShow) { }
			
			virtual void writeStatistics(void) { }

			virtual bool keyPressed(const OIS::KeyEvent & keyEventRef) { return true; }
			virtual bool keyReleased(const OIS::KeyEvent & keyEventRef) { return true; }

			virtual bool mouseMoved(const OIS::MouseEvent & arg) { return true; }
			virtual bool mousePressed(const OIS::MouseEvent & arg, OIS::MouseButtonID id) { return true; }
			virtual bool mouseReleased(const OIS::MouseEvent & arg, OIS::MouseButtonID id) { return true; }

			virtual void processUnbufferedKeyInput(Ogre::Real dt) { }
			virtual void processUnbufferedMouseInput(Ogre::Real dt) { }
		};

	} // namespace end NET

} // namespace end NOWA

#endif


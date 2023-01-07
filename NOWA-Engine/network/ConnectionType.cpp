#include "NOWAPrecompiled.h"
#include "ConnectionType.h"

namespace NOWA
{
	namespace NET
	{

		// AppReplicaManager ConnectionType::appReplicaManager;

		ConnectionType::ConnectionType()
		{

		}

		ConnectionType::ConnectionType(Ogre::SceneManager* sceneManager, OgreNewt::World* ogreNewt)
			: 	sceneManager(sceneManager),
				ogreNewt(ogreNewt),
				statusActive(false)
		{

		}

		ConnectionType::~ConnectionType(void)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ConnectionType] ConnectionType destroyed");
		}

		void ConnectionType::setupConnection(void)
		{

		}

		void ConnectionType::setupWidgets(void)
		{

		}

		void ConnectionType::showStatus(bool bShow)
		{

		}

		void ConnectionType::writeStatistics(void)
		{

		}

		bool ConnectionType::keyPressed(const OIS::KeyEvent &keyEventRef)
		{
			return true;
		}

		bool ConnectionType::keyReleased(const OIS::KeyEvent &keyEventRef)
		{
			return true;
		}

		bool ConnectionType::mouseMoved(const OIS::MouseEvent &arg)
		{
			return true;
		}

		bool ConnectionType::mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
		{
			return true;
		}

		bool ConnectionType::mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
		{
			return true;
		}

		void ConnectionType::processUnbufferedKeyInput(Ogre::Real dt)
		{

		}

		void ConnectionType::processUnbufferedMouseInput(Ogre::Real dt)
		{

		}

	} // namespace end NET

} // namespace end NOWA
//#include "decorators/PhysicsObjectDecorator.h"
//#include "gameobject/PhysicsObject.h"
//
//namespace NOWA
//{
//
//	extern bool decoratorDeleted = false;
//
//	PhysicsObjectDecorator::PhysicsObjectDecorator(PhysicsObject* pPhysicsObject, OgreNewt::World* ogreNewt)
//		:
//	pPhysicsObject	(pPhysicsObject),
//		ogreNewt			(ogreNewt),
//		pRaycastThread		(0)
//	{
//
//	}
//
//	PhysicsObjectDecorator::~PhysicsObjectDecorator()
//	{
//		decoratorDeleted = true;
//		// Delete the Thread
//		if (this->pRaycastThread)
//		{
//			this->pRaycastThread->detach();
//			delete this->pRaycastThread;
//			this->pRaycastThread = NULL;
//		}
//	}
//
//	//void PhysicsObjectDecorator::checkObjectsBelow(GameObject* pGameObject, OgreNewt::World* ogreNewt)
//	void PhysicsObjectDecorator::checkObjectsBelow(void* _pPhysicsObject)
//	{
//		PhysicsObject* pPhysicsObject = (PhysicsObject*)_pPhysicsObject;
//		Ogre::Real nextUpdate = NOWA::Core::getSingletonPtr()->getOgreTimer()->getMilliseconds();
//
//		while (decoratorDeleted == false)
//		{
//			//Ogre::Vector3 playerPos = Ogre::Vector3::ZERO;
//			//Ogre::Quaternion playerOrientation = Ogre::Quaternion::IDENTITY;
//			//pPlayer->pPhysicsBody->getPositionOrientation(playerPos, playerOrientation);
//			//Windows Signale abhandeln, sodass die Simulation jederzeit unterbrochen werden kann, durch Benutzerinteraktion
//#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
//			Ogre::WindowEventUtilities::messagePump();
//#endif
//			if (NOWA::Core::getSingletonPtr()->getOgreTimer()->getMilliseconds() >= nextUpdate)
//			{
//				Ogre::Vector3 pos = Ogre::Vector3::ZERO;
//				if (decoratorDeleted == false)
//				{
//					pos = pPhysicsObject->getPosition();
//					//Anfangsposition
//					Ogre::Vector3 point = Ogre::Vector3(pos.x, pos.y + 0.4f, pos.z);
//					//Strahl von Anfangsposition bis 500 Meter nach unten erzeugen
//					OgreNewt::BasicRaycast ray(pPhysicsObject->getOgreNewt(), point, point + Ogre::Vector3::NEGATIVE_UNIT_Y * 500.0f, true);
//					OgreNewt::BasicRaycast::BasicRaycastInfo contact = ray.getFirstHit();
//					if (contact.getBody())
//					{
//						//* 500 da bei der distanz ein Wert zwischen [0,1] rauskommt also zb. 0.0019
//						//Die Distanz ist relativ zur Länge des Raystrahls
//						//d. h. wenn der Raystrahl nichts mehr trifft wird in diesem Fall
//						//die Distanz > 500, da prozentural, es wird vorher skaliert
//						Ogre::Real height = contact.mDistance  * 500.0f;
//						pPhysicsObject->setHeight(height);
//						if (contact.mBody->getType() == NOWA::Core::getSingletonPtr()->TERRAINTYPE)
//						{
//							//Y
//							//|
//							//|___ Normale
//							//Winkel zwischen Y-Richtung und der normalen eines Objektes errechnen
//							Ogre::Vector3 vec = Ogre::Vector3::UNIT_Y;
//							Ogre::Vector3 normal = contact.mNormal;
//							Ogre::Real rise = Ogre::Math::ACos(vec.dotProduct(normal) / (vec.length() * normal.length())).valueDegrees();
//							pPhysicsObject->setRise(rise);
//						}
//					}
//				}
//
//				//1000/60 = 16 fps, alle 16 Bilder pro Sekunde wird diese Funktion ausgeführt
//				//1000/16.667 = 60, alle 60
//				nextUpdate += 16.667f;
//			}
//		}
//
//		//Es handelt sich hier um eine globale Variable, daher muss diese, nachdem der Thread geloescht wurde, wieder auf den Standartwert gesetzt werden!!
//		//Globare Variablen leben so lange wie die ganze Anwendung!!! also auch ueber die Zustaende hinaus!
//		decoratorDeleted = false;
//	}
//
//	void PhysicsObjectDecorator::executeObjectBelowCheck(void)
//	{
//		if (!this->pRaycastThread)
//		{
//			this->pRaycastThread = new boost::thread(boost::bind(&PhysicsObjectDecorator::checkObjectsBelow, this));
//		} 
//		else 
//		{
//			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsObjectDecorator: A thread for executeObjectBelowCheck() has been already created!");
//		}
//	}
//
//}; // namespace end

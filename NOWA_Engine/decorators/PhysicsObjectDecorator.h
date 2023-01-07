//#ifndef PHYSICS_OBJECT_DECORATOR_H
//#define PHYSICS_OBJECT_DECORATOR_H
//
//#include <OgreNewt.h>
////#include <Ogre.h>
//
//#include "Core.h"
//
//#include "decorators/Decorator.h"
//
//namespace NOWA
//{
//
//class PhysicsObject;
//
//class EXPORTED PhysicsObjectDecorator : public Decorator
//{
//public:
//	PhysicsObjectDecorator(PhysicsObject* pPhysicsObject, OgreNewt::World* ogreNewt);
//	virtual ~PhysicsObjectDecorator();
//	void executeObjectBelowCheck(void);
//private:
//	//Raycast auf den Boden in einem Thread berechnen
//	//static void checkObjectsBelow(GameObject* pGameObject, OgreNewt::World* ogreNewt);
//	static void checkObjectsBelow(void* _pPhysicsObject);
//private:
//	PhysicsObject*		pPhysicsObject;
//	OgreNewt::World*		ogreNewt;
//	//Zeiger auf Raycast Thread
//	boost::thread*			pRaycastThread;
//	//Naechstes Update im Raycast Thread
//	//Ogre::Real				nextUpdate;
//	//Steigung
//	//Ogre::Real				rise;
//	//Ogre::Real				height;
//
//};
//
//}; // namespace end
//
//#endif

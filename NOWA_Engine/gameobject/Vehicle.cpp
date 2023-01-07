#include "NOWAPrecompiled.h"
//#include "Vehicle.h"
//#include "modules/OgreNewtModule.h"
//#include "GameObjectController.h"
//#include "utilities/XMLConverter.h"
//#include "utilities/MathHelper.h"
//
//namespace NOWA
//{
//	Vehicle::Vehicle(OgreNewt::Body* carBody, unsigned int tireCount)
//		: RayCastVehicle(carBody, tireCount)
//
//	{
//
//	}
//
//	Vehicle::~Vehicle()
//	{
//		Ogre::LogManager::getSingletonPtr()->logMessage("#####################vehicle destructor");
//	}
//
//	void Vehicle::setTireTransform(void* tireNodePtr, const Ogre::Matrix4& tireMatrix) const
//	{
//		// cats the user data pointer t0 a scene node;
//		Ogre::Node* tireNode = (Ogre::Node*) tireNodePtr;
//		Ogre::Node* parentNode = tireNode->getParent();
//
//		const Ogre::Matrix4& parentMatrix = parentNode->_getFullTransform();
//
//		// Ogre Matrix inverse really sucks (it use a full Gaussian pivoting when a simple transpose follow by vector rotation will do.
//		const Ogre::Matrix4 tireLocalMatrix(MathHelper::getInstance()->matrixTransposeInverse(parentMatrix) * tireMatrix);
//
//		tireNode->setOrientation(tireLocalMatrix.extractQuaternion());
//		tireNode->setPosition(Ogre::Vector3(tireLocalMatrix[0][3], tireLocalMatrix[1][3], tireLocalMatrix[2][3]));
//	}
//
//}; // namespace end
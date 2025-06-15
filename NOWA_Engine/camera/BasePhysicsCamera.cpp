#include "NOWAPrecompiled.h"
#include "BasePhysicsCamera.h"
#include "gameobject/GameObjectComponent.h"
#include "main/InputDeviceCore.h"
#include "main/AppStateManager.h"
#include "utilities/MathHelper.h"
#include "modules/InputDeviceModule.h"
#include "modules/GraphicsModule.h"

namespace NOWA
{
	BasePhysicsCamera::BasePhysicsCamera(unsigned int id, Ogre::SceneManager* sceneManager, OgreNewt::World* ogreNewt, const Ogre::Vector3& cameraSize,
		Ogre::Real moveSpeed, Ogre::Real rotateSpeed, Ogre::Real smoothValue)
		: sceneManager(sceneManager),
		ogreNewt(ogreNewt),
		cameraSize(cameraSize),
		BaseCamera(id, moveSpeed, rotateSpeed, smoothValue),
		cameraBody(nullptr),
		upVector(nullptr)
	{
		
	}

	BasePhysicsCamera::~BasePhysicsCamera()
	{
		this->onClearData();
	}

	void BasePhysicsCamera::onSetData(void)
	{
		this->firstTimeValueSet = true;
		Ogre::Vector3 resultPosition = this->camera->getPosition();
		Ogre::Quaternion resultOrientation = this->camera->getOrientation();

		ENQUEUE_RENDER_COMMAND_WAIT("BasePhysicsCamera::onSetData",
		{
			this->cameraNode = this->sceneManager->getRootSceneNode(Ogre::SCENE_DYNAMIC)->createChildSceneNode(Ogre::SCENE_DYNAMIC);
			this->cameraNode->setName("PhysicsCameraNode");
			this->cameraNode->setStatic(false);
			this->camera->setStatic(false);
			this->camera->setPosition(Ogre::Vector3::ZERO);
			this->camera->setOrientation(Ogre::Quaternion::IDENTITY);

			// Camera is always attached to root node when created, so must be first detached
			if (true == this->camera->isAttached())
			{
				this->camera->detachFromParent();
			}
			this->cameraNode->attachObject(this->camera);
			// Define it's scale 
			this->cameraNode->setScale(this->cameraSize);
		});

		OgreNewt::CollisionPrimitives::Ellipsoid* col = new OgreNewt::CollisionPrimitives::Ellipsoid(this->ogreNewt, this->cameraSize, 0);
		Ogre::Vector3 inertia;
		Ogre::Vector3 massOrigin;
		col->calculateInertialMatrix(inertia, massOrigin);
		OgreNewt::CollisionPtr collisionPtr = OgreNewt::CollisionPtr(col);

		this->cameraBody = new OgreNewt::Body(this->ogreNewt, this->sceneManager, collisionPtr, Ogre::SCENE_DYNAMIC);
		NOWA::AppStateManager::getSingletonPtr()->getOgreNewtModule()->registerRenderCallbackForBody(this->cameraBody);

		inertia *= 50.0f;
		this->cameraBody->setMassMatrix(50.0f, inertia);

		this->cameraBody->setCustomForceAndTorqueCallback<BasePhysicsCamera>(&BasePhysicsCamera::moveCallback, this);
		this->cameraBody->setLinearDamping(0.0016f);

		// Attach the node to the body. Now, when the body moves the node moves too 
		// But do not update rotation (false) physically because it would mess up a lot of things
		this->cameraBody->attachNode(this->cameraNode, false);

		this->cameraBody->setPositionOrientation(resultPosition, resultOrientation);

		this->upVector = new OgreNewt::UpVector(this->cameraBody, Ogre::Vector3(0.0f, 1.0f, 0.0f));
	}

	void BasePhysicsCamera::onClearData(void)
	{
		ENQUEUE_RENDER_COMMAND_WAIT("BasePhysicsCamera::onClearData",
		{
			if (this->cameraNode->getAttachedObjectIterator().hasMoreElements())
			{
				this->cameraNode->detachObject(this->camera);
			}
			if (nullptr != this->cameraNode)
			{
				NOWA::GraphicsModule::getInstance()->removeTrackedNode(this->cameraNode);
				this->sceneManager->destroySceneNode(this->cameraNode);
				this->cameraNode = nullptr;
			}
		});
		if (nullptr != this->upVector)
		{
			this->upVector->destroyJoint(this->ogreNewt);
			delete this->upVector;
			this->upVector = nullptr;
		}
		if (this->cameraBody)
		{
			this->cameraBody->removeForceAndTorqueCallback();
			this->cameraBody->removeNodeUpdateNotify();
			this->cameraBody->removeDestructorCallback();
			delete this->cameraBody;
			this->cameraBody = nullptr;
		}
	}

	void BasePhysicsCamera::moveCallback(OgreNewt::Body* body, Ogre::Real timeStep, int threadIndex)
	{
		// Do nothing here, since the body should not fall, but move as the camera moves
	}

	void BasePhysicsCamera::moveCamera(Ogre::Real dt)
	{
		Ogre::Vector3 moveValue = Ogre::Vector3::ZERO;
		
		// bad design, the application should consider which key does what
		if (NOWA::InputDeviceCore::getSingletonPtr()->getKeyboard()->isKeyDown(NOWA_K_CAMERA_LEFT))
		{
			moveValue += Ogre::Vector3(-this->moveSpeed * this->moveCameraWeight, 0.0f, 0.0f);
		}
		if (NOWA::InputDeviceCore::getSingletonPtr()->getKeyboard()->isKeyDown(NOWA_K_CAMERA_RIGHT))
		{
			moveValue += Ogre::Vector3(this->moveSpeed * this->moveCameraWeight, 0.0f, 0.0f);
		}
		if (NOWA::InputDeviceCore::getSingletonPtr()->getKeyboard()->isKeyDown(NOWA_K_CAMERA_BACKWARD))
		{
			moveValue += Ogre::Vector3(0.0f, 0.0f, this->moveSpeed * this->moveCameraWeight);
		}
		if (NOWA::InputDeviceCore::getSingletonPtr()->getKeyboard()->isKeyDown(NOWA_K_CAMERA_FORWARD))
		{
			moveValue += Ogre::Vector3(0.0f, 0.0f, -this->moveSpeed * this->moveCameraWeight);
		}
		if (NOWA::InputDeviceCore::getSingletonPtr()->getKeyboard()->isKeyDown(NOWA_K_CAMERA_DOWN))
		{
			moveValue += Ogre::Vector3(0.0f, -this->moveSpeed * this->moveCameraWeight, 0.0f);
		}
		if (NOWA::InputDeviceCore::getSingletonPtr()->getKeyboard()->isKeyDown(NOWA_K_CAMERA_UP))
		{
			moveValue += Ogre::Vector3(0.0f, this->moveSpeed * this->moveCameraWeight, 0.0f);
		}

		if (this->firstTimeMoveValueSet)
		{
			this->lastMoveValue = moveValue;
			this->firstTimeMoveValueSet = false;
		}

		moveValue.x = NOWA::MathHelper::getInstance()->lowPassFilter(moveValue.x, this->lastMoveValue.x, this->smoothValue);
		moveValue.y = NOWA::MathHelper::getInstance()->lowPassFilter(moveValue.y, this->lastMoveValue.y, this->smoothValue);
		moveValue.z = NOWA::MathHelper::getInstance()->lowPassFilter(moveValue.z, this->lastMoveValue.z, this->smoothValue);

		Ogre::Vector3 direction = this->cameraNode->getOrientation() * moveValue;
		this->cameraBody->setVelocity(direction);

		this->lastMoveValue = moveValue;
	}

	void BasePhysicsCamera::rotateCamera(Ogre::Real dt, bool forJoyStick)
	{
		const OIS::MouseState& ms = NOWA::InputDeviceCore::getSingletonPtr()->getMouse()->getMouseState();

		Ogre::Vector2 rotationValue;
		rotationValue.x = -ms.X.rel * this->rotateSpeed * this->rotateCameraWeight;
		rotationValue.y = -ms.Y.rel * this->rotateSpeed * this->rotateCameraWeight;

		if (this->firstTimeValueSet)
		{
			this->lastValue = rotationValue;
			this->firstTimeValueSet = false;
		}

		rotationValue.x = NOWA::MathHelper::getInstance()->lowPassFilter(rotationValue.x, this->lastValue.x, this->smoothValue);
		rotationValue.y = NOWA::MathHelper::getInstance()->lowPassFilter(rotationValue.y, this->lastValue.y, this->smoothValue);
		// Silly shit, this node is as dynamic as can be, but still the assert will throw, so since this node is just for the camera, this expensive method may be called

		auto closureFunction = [this, rotationValue](Ogre::Real weight)
		{
			this->cameraNode->_getFullTransformUpdated();
			this->cameraNode->rotate(Ogre::Quaternion(Ogre::Degree(rotationValue.x), Ogre::Vector3::UNIT_Y), Ogre::Node::TS_WORLD);
			this->cameraNode->rotate(Ogre::Quaternion(Ogre::Degree(rotationValue.y), Ogre::Vector3::UNIT_X), Ogre::Node::TS_LOCAL);
		};
		Ogre::String id = "BasePhysicsCamera::rotateCamera";
		NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction);

		this->lastValue = rotationValue;
	}

	Ogre::Vector3 BasePhysicsCamera::getPosition(void)
	{
		return this->cameraBody->getPosition();
	}

	Ogre::Quaternion BasePhysicsCamera::getOrientation(void)
	{
		return this->cameraNode->getOrientation();
	}

}; //namespace end

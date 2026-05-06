#include "NOWAPrecompiled.h"
#include "FollowCamera2D.h"
#include "utilities/MathHelper.h"
#include "gameobject/GameObject.h"
#include "gameobject/GameObjectController.h"
#include "main/AppStateManager.h"
#include "main/Core.h"
#include "modules/GraphicsModule.h"

namespace NOWA
{
	FollowCamera2D::FollowCamera2D(unsigned int id, Ogre::SceneNode* sceneNode, const Ogre::Vector3& offsetPosition, Ogre::Real smoothValue)
		: BaseCamera(id, 0, 0, smoothValue),
		sceneNode(sceneNode),
		sceneManager(nullptr),
		lastMoveValue(Ogre::Vector3::ZERO),
		firstTimeValueSet(true),
		firstTimeMoveValueSet(true),
		offset(offsetPosition),
		borderOffset(Ogre::Vector3(50.0f, 0.0f, 0.0f)),
		showGameObject(false),
		raySceneQuery(nullptr),
		hiddenSceneNode(nullptr),
		fadeValue(0.0f),
		fadingFinished(true),
		mostRightUp(Ogre::Vector3::ZERO),
		smoothValue(smoothValue),
		minimumBounds(Ogre::Vector3::ZERO),
		maximumBounds(Ogre::Vector3::ZERO),
		pDebugLine(nullptr)
	{
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &FollowCamera2D::handleUpdateBounds), EventDataBoundsUpdated::getStaticEventType());
	}

	FollowCamera2D::~FollowCamera2D()
	{
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &FollowCamera2D::handleUpdateBounds), EventDataBoundsUpdated::getStaticEventType());
		this->sceneNode = nullptr;
		if (this->raySceneQuery)
		{
			ENQUEUE_RENDER_COMMAND("FollowCamera2D::~FollowCamera2D",
			{
				this->sceneManager->destroyQuery(this->raySceneQuery);
			});
		}
	}

	void FollowCamera2D::onSetData(void)
	{
		BaseCamera::onSetData();
		this->firstTimeMoveValueSet = true;
	}

	void FollowCamera2D::setOffset(const Ogre::Vector3& offset)
	{
		this->offset = offset;
		this->firstTimeMoveValueSet = true;
	}
	
	void FollowCamera2D::handleUpdateBounds(NOWA::EventDataPtr eventData)
	{
		// When a new game object has been added to the scene, update the bounds for follow camera 2D
		boost::shared_ptr<NOWA::EventDataBoundsUpdated> castEventData = boost::static_pointer_cast<EventDataBoundsUpdated>(eventData);
		this->setBounds(castEventData->getCalculatedBounds().first, castEventData->getCalculatedBounds().second);
	}

	void FollowCamera2D::setBorderOffset(const Ogre::Vector3& borderOffset)
	{
		this->borderOffset = borderOffset;
		this->firstTimeMoveValueSet = true;
	}

	void FollowCamera2D::setBounds(const Ogre::Vector3& minimumBounds, const Ogre::Vector3& maximumBounds)
	{
		this->minimumBounds = minimumBounds/* + this->borderOffset*/;
		this->maximumBounds = maximumBounds/* - this->borderOffset*/;

		Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[FollowCamera2D] minimum bounds: " + Ogre::StringConverter::toString(this->minimumBounds)
		 + "maximum bounds: " + Ogre::StringConverter::toString(this->maximumBounds));

		if (nullptr == this->camera)
		{
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[FollowCamera2D] Error: Cannot set bounds because the camera does not exist yet. Please call first CameraManager->addCameraBehavior(...)!");
			throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[FollowCamera2D] Error: Cannot set bounds because the camera does not exist yet. Please call first CameraManager->addCameraBehavior(...)!\n", "NOWA");
		}
		// Corner must be calculated here and once, when the camera position is set to its offset to the player
		const Ogre::Vector3* corners = this->camera->getWorldSpaceCorners();

		/*auto fl = this->camera->getFocalLength();
		auto fo = this->camera->getFOVy();
		auto fd = this->camera->getFOVy().valueDegrees();*/

		// Note: 2.5 is exactly the value! Even when fovy is changed, the value is correct
		this->mostRightUp = (this->camera->getViewMatrix(true) * corners[4] / 2.5f) / this->camera->getAspectRatio();
		// this->mostRightUp += this->borderOffset;
		this->mostRightUp -= Ogre::Vector3(this->borderOffset.z, this->borderOffset.z, 0.0f);
		this->firstTimeValueSet = true;
		this->firstTimeMoveValueSet = true;
	}

	void FollowCamera2D::alwaysShowGameObject(bool show, const Ogre::String& category, Ogre::SceneManager* sceneManager)
	{
		ENQUEUE_RENDER_COMMAND_MULTI("FollowCamera2D::alwaysShowGameObject", _3(show, category, sceneManager),
		{
			this->showGameObject = show;
			this->category = category;
			this->sceneManager = sceneManager;
			if (!this->showGameObject)
			{
				if (this->raySceneQuery)
				{
					this->sceneManager->destroyQuery(this->raySceneQuery);
				}
			}
			else
			{
				// Check if the game object should be always shown,
				// if this is the case create ray scene query to throw a ray and check if the player will always be hit
				// hide all game objects that are in front of the player
				this->raySceneQuery = this->sceneManager->createRayQuery(Ogre::Ray());
			}
		});
	}

	void FollowCamera2D::setSceneNode(Ogre::SceneNode * sceneNode)
	{
		this->sceneNode = sceneNode;
	}

	void FollowCamera2D::moveCamera(Ogre::Real dt)
	{
		// Error: No bounds have been set
		if (Ogre::Vector3::ZERO == this->mostRightUp)
		{
			return;
		}

		if (nullptr == this->sceneNode)
		{
			return;
		}

		if (true == this->firstTimeMoveValueSet)
		{
			this->lastMoveValue = Ogre::Vector3::ZERO;

			ENQUEUE_RENDER_COMMAND("FollowCamera2D::moveCamera start",
			{
				// set the camera position to the target one for the first time
				this->camera->setPosition(this->sceneNode->getPosition());
				this->camera->move(this->offset);
			});

			/*this->pDebugLine = this->sceneManager->createManualObject("DebugRayLineFlubber");
			this->pDebugLine->setRenderQueueGroup(Ogre::RENDER_QUEUE_OVERLAY);
			this->pDebugLine->setQueryFlags(0);
			Ogre::SceneNode *pNode = this->sceneManager->getRootSceneNode()->createChildSceneNode("Debugline1FlubberNode1");
			pNode->attachObject(this->pDebugLine);*/

			this->firstTimeMoveValueSet = false;
		}

		const Ogre::Vector3 cameraPosition = this->camera->getPosition();

		//////////////////////////////////////////////////////////////////////////////////////////////
		//// The camera follows the game object as long as it is within the virtual environments bounds
		Ogre::Vector3 velocity = Ogre::Vector3::ZERO;
		// also add the offset, by which the player differentiates from the camera, to get the correct position
		if (this->sceneNode->getPosition().x + this->offset.x - this->mostRightUp.x > this->minimumBounds.x
			&& this->sceneNode->getPosition().x + this->offset.x + this->mostRightUp.x < this->maximumBounds.x)
		{

			velocity.x = this->sceneNode->getPosition().x - cameraPosition.x + this->offset.x;
			// Aufschlageffekt, dies koennte nutzlich sein, wenn ein schwer objekt auf den Boden knallt wie bei Prehistorik lax, damit die Kamera erschuettert wird! wenn smooth value angepasst wird

// Attention: Instead of lowpass filter which osscilates to much, maybe use spring formula:
			/*
			Ogre::Vector3 anchorPosition = predecessorJointSpringCompPtr->getBody()->getPosition() + jointSpringCompPtr->getAnchorOffsetPosition();
				Ogre::Vector3 springPosition = this->getPosition() + jointSpringCompPtr->getSpringOffsetPosition();

				// Calculate spring force
				Ogre::Vector3 dragForce = ((anchorPosition - springPosition) * mass * jointSpringCompPtr->getSpringStrength()) - body->getVelocity();
			*/

			if (Ogre::Math::RealEqual(velocity.x, 0.0f))
			{
				velocity.x = 0.0f;
			}
			if (Ogre::Math::RealEqual(velocity.y, 0.0f))
			{
				velocity.y = 0.0f;
			}
			if (Ogre::Math::RealEqual(velocity.z, 0.0f))
			{
				velocity.z = 0.0f;
			}
		}
		// check the y bounds
		if (this->sceneNode->getPosition().y + this->offset.y - this->mostRightUp.y > this->minimumBounds.y
			&& this->sceneNode->getPosition().y + this->offset.y + this->mostRightUp.y < this->maximumBounds.y)
		{
			velocity.y = this->sceneNode->getPosition().y - cameraPosition.y + this->offset.y;
		}

		velocity.x = NOWA::MathHelper::getInstance()->lowPassFilter(velocity.x, this->lastMoveValue.x, this->smoothValue);
		velocity.y = NOWA::MathHelper::getInstance()->lowPassFilter(velocity.y, this->lastMoveValue.y, this->smoothValue);

		// TODO: cameraUpdate in queue
		// this->camera->move(velocity * this->moveCameraWeight);
		Ogre::Vector3 newMove = this->camera->getPosition() + (velocity * this->moveCameraWeight);
		NOWA::GraphicsModule::getInstance()->updateCameraPosition(this->camera, newMove);

		this->lastMoveValue = velocity;

		//////////////////////////////////////////////////////////////////////////////////////////////
		//// If the camera is out of bounds, constrain its position to remain within bounds
		// x bounds
		Ogre::Real cameraPositionX = cameraPosition.x;
		Ogre::Real mostRightUpX = this->mostRightUp.x;
		Ogre::Real borderXRight = cameraPositionX + mostRightUpX;
		Ogre::Real borderXLeft = cameraPositionX - mostRightUpX;
		if (borderXRight > this->maximumBounds.x/* + this->borderOffset.x*/)
		{
			/*ENQUEUE_RENDER_COMMAND_MULTI_WAIT("FollowCamera2D::moveCamera max x bounds", _1(cameraPosition),
			{
				this->camera->setPosition(this->maximumBounds.x - this->mostRightUp.x, cameraPosition.y, cameraPosition.z);
			});*/

			NOWA::GraphicsModule::getInstance()->updateCameraPosition(this->camera, Ogre::Vector3(this->maximumBounds.x - this->mostRightUp.x, cameraPosition.y, cameraPosition.z));
		}
		else if (borderXLeft < this->minimumBounds.x/* - this->borderOffset.x*/)
		{
			Ogre::Vector3 newPos = Ogre::Vector3(this->minimumBounds.x + this->mostRightUp.x/* + this->borderOffset.x*/, cameraPosition.y, cameraPosition.z);
			/*ENQUEUE_RENDER_COMMAND_MULTI_WAIT("FollowCamera2D::moveCamera min x bounds", _1(newPos),
			{
				this->camera->setPosition(newPos);
			});*/

			NOWA::GraphicsModule::getInstance()->updateCameraPosition(this->camera, newPos);
		}

		// y bounds
		if (this->camera->getPosition().y + this->mostRightUp.y > this->maximumBounds.y/* + this->borderOffset.y*/)
		{
			/*ENQUEUE_RENDER_COMMAND_MULTI_WAIT("FollowCamera2D::moveCamera max y bounds", _1(cameraPosition),
			{
				this->camera->setPosition(cameraPosition.x, this->maximumBounds.y - this->mostRightUp.y, cameraPosition.z);
			});*/

			NOWA::GraphicsModule::getInstance()->updateCameraPosition(this->camera, Ogre::Vector3(cameraPosition.x, this->maximumBounds.y - this->mostRightUp.y, cameraPosition.z));
		}
		else if (this->camera->getPosition().y + this->mostRightUp.y < this->minimumBounds.y/* - this->borderOffset.y*/)
		{
			/*ENQUEUE_RENDER_COMMAND_MULTI_WAIT("FollowCamera2D::moveCamera min y bounds", _1(cameraPosition),
			{
				this->camera->setPosition(cameraPosition.x, this->minimumBounds.y + this->mostRightUp.y, cameraPosition.z);
			});*/

			NOWA::GraphicsModule::getInstance()->updateCameraPosition(this->camera, Ogre::Vector3(cameraPosition.x, this->minimumBounds.y + this->mostRightUp.y, cameraPosition.z));
		}
		// Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[FollowCamera2D] Position: " + Ogre::StringConverter::toString(this->camera->getPosition()));
	}

	void FollowCamera2D::rotateCamera(Ogre::Real dt, bool forJoyStick)
	{
		
	}

	Ogre::Vector3 FollowCamera2D::getPosition(void)
	{
		return this->camera->getPosition();
	}

	Ogre::Quaternion FollowCamera2D::getOrientation(void)
	{
		return this->camera->getOrientation();
	}

	//void FollowCamera2D::followGameObject(const Ogre::Vector3& position, const Ogre::Vector3& offset, const Ogre::Vector3& direction, Ogre::Real dt)
	//{
	//	Ogre::Vector3 cameraPosition = position;

	//	if (this->firstTimeMoveValueSet)
	//	{
	//		this->lastMoveValue = position;
	//		this->camera->moveRelative(offset);
	//		this->firstTimeMoveValueSet = false;
	//	}

	//	cameraPosition.x = NOWA::MathHelper::getInstance()->lowPassFilter(cameraPosition.x, this->lastMoveValue.x, this->smoothValue);
	//	cameraPosition.y = NOWA::MathHelper::getInstance()->lowPassFilter(cameraPosition.y, this->lastMoveValue.y, this->smoothValue);
	//	cameraPosition.z = NOWA::MathHelper::getInstance()->lowPassFilter(cameraPosition.z, this->lastMoveValue.z, this->smoothValue);

	//	Ogre::Vector3 velocity = position - this->lastMoveValue;

	//	this->camera->moveRelative(velocity);
	//	// this->camera->setDirection(direction);

	//	this->lastMoveValue = position;
	//}

}; //namespace end

#include "NOWAPrecompiled.h"
#include "FollowCamera2D.h"
#include "utilities/MathHelper.h"
#include "gameobject/GameObject.h"
#include "gameobject/GameObjectController.h"
#include "main/AppStateManager.h"
#include "main/Core.h"

namespace NOWA
{
	FollowCamera2D::FollowCamera2D(Ogre::SceneNode* sceneNode, const Ogre::Vector3& offsetPosition, Ogre::Real smoothValue)
		: BaseCamera(0, 0, smoothValue),
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
			this->sceneManager->destroyQuery(this->raySceneQuery);
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
		// When a new game object has been added to the world, update the bounds for follow camera 2D
		boost::shared_ptr<NOWA::EventDataBoundsUpdated> castEventData = boost::static_pointer_cast<EventDataBoundsUpdated>(eventData);
		this->setBounds(castEventData->getCalculatedBounds().first, castEventData->getCalculatedBounds().second);
	}

	void FollowCamera2D::setBorderOffset(const Ogre::Vector3& borderOffset)
	{
		this->borderOffset = borderOffset;
		this->firstTimeMoveValueSet = true;
	}

	void FollowCamera2D::setBounds(Ogre::Vector3& minimumBounds, Ogre::Vector3& maximumBounds)
	{
		this->minimumBounds = minimumBounds + this->borderOffset;
		this->maximumBounds = maximumBounds - this->borderOffset;

		// Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[FollowCamera2D] minimumBounds: " + Ogre::StringConverter::toString(this->minimumBounds));

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
		this->mostRightUp += this->borderOffset;

		this->firstTimeValueSet = true;
		this->firstTimeMoveValueSet = true;
	}

	void FollowCamera2D::alwaysShowGameObject(bool show, const Ogre::String& category, Ogre::SceneManager* sceneManager)
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
	}

	void FollowCamera2D::setSceneNode(Ogre::SceneNode * sceneNode)
	{
		this->sceneNode = sceneNode;
	}

#if 0
	void FollowCamera2D::moveCamera(Ogre::Real dt)
	{
		// Error: No bounds have been set
		if (Ogre::Vector3::ZERO == this->mostRightUp)
			return;
		
		if (nullptr == this->sceneNode)
			return;

		Ogre::Vector3 sceneNodePosition = this->sceneNode->_getDerivedPositionUpdated();

		if (this->firstTimeMoveValueSet)
		{
			this->lastMoveValue = Ogre::Vector3::ZERO;
			// Set the camera position to the target one for the first time
			this->camera->setPosition(sceneNodePosition);
			// this->camera->lookAt(this->sceneNode->getPosition());
			this->camera->move(this->offset);

			/*this->pDebugLine = this->sceneManager->createManualObject("DebugRayLineFlubber");
			this->pDebugLine->setRenderQueueGroup(Ogre::RENDER_QUEUE_OVERLAY);
			this->pDebugLine->setQueryFlags(0);
			Ogre::SceneNode *pNode = this->sceneManager->getRootSceneNode()->createChildSceneNode("Debugline1FlubberNode1");
			pNode->attachObject(this->pDebugLine);*/
			
			this->firstTimeMoveValueSet = false;
		}

		//////////////////////////////////////////////////////////////////////////////////////////////
		//// The camera follows the game object as long as it is within the virtual environments bounds
		Ogre::Vector3 velocity = Ogre::Vector3::ZERO;
		// Also add the offset, by which the player differentiates from the camera, to get the correct position
		if (sceneNodePosition.x + this->offset.x - this->mostRightUp.x  > this->minimumBounds.x
			&& sceneNodePosition.x + this->offset.x + this->mostRightUp.x < this->maximumBounds.x)
		{
			
			velocity.x = sceneNodePosition.x - this->camera->getPosition().x + this->offset.x;
			// Aufschlageffekt, dies koennte nutzlich sein, wenn ein schwer objekt auf den Boden knallt wie bei Prehistorik lax, damit die Kamera erschuettert wird! wenn smooth value angepasst wird
			
// Attention: Instead of lowpass filter which osscilates to much, maybe use spring formula:
			/*
			Ogre::Vector3 anchorPosition = predecessorJointSpringCompPtr->getBody()->getPosition() + jointSpringCompPtr->getAnchorOffsetPosition();
				Ogre::Vector3 springPosition = this->getPosition() + jointSpringCompPtr->getSpringOffsetPosition();

				// Calculate spring force
				Ogre::Vector3 dragForce = ((anchorPosition - springPosition) * mass * jointSpringCompPtr->getSpringStrength()) - body->getVelocity();
			*/
			
			
			if (0.0f != this->smoothValue)
			{
				velocity.x = NOWA::MathHelper::getInstance()->lowPassFilter(velocity.x, this->lastMoveValue.x, this->smoothValue);
			}
			//// this->camera->setPosition(NOWA::MathHelper::getInstance()->lowPassFilter(cameraPosition.x, this->lastMoveValue.x, this->smoothValue), cameraPosition).y, cameraPosition.z);

			//// this->camera->setDirection(direction);

			this->lastMoveValue.x = velocity.x;
			// this->camera->setPosition(Ogre::Vector3(this->sceneNode->getPosition().x, cameraPosition.y, cameraPosition.z));

			// currentPosition.x = NOWA::MathHelper::getInstance()->lowPassFilter(currentPosition.x, this->lastMoveValue.x, this->smoothValue);
			/*velocity.x = this->sceneNode->getPosition().x - this->lastMoveValue.x;

			this->lastMoveValue.x = this->sceneNode->getPosition().x;*/
		}
		// check the y bounds
		if (sceneNodePosition.y + this->offset.y - this->mostRightUp.y > this->minimumBounds.y
			&& sceneNodePosition.y + this->offset.y + this->mostRightUp.y < this->maximumBounds.y)
		{
			velocity.y = sceneNodePosition.y - this->camera->getPosition().y + this->offset.y;
			if (0.0f != this->smoothValue)
			{
				velocity.y = NOWA::MathHelper::getInstance()->lowPassFilter(velocity.y, this->lastMoveValue.y, this->smoothValue);
			}

			this->lastMoveValue.y = velocity.y;
			// this->camera->setPosition(Ogre::Vector3(cameraPosition.x, this->sceneNode->getPosition().y + this->offset.y, cameraPosition.z + this->offset.z));

			/*velocity.y = this->sceneNode->getPosition().y - this->lastMoveValue.y;

			this->lastMoveValue.y = this->sceneNode->getPosition().y;*/
		}

		this->camera->moveRelative(velocity * this->moveCameraWeight);

		//////////////////////////////////////////////////////////////////////////////////////////////
		//// If the camera is out of bounds, constrain its position to remain within bounds
		// x bounds
		Ogre::Real cameraPositionX = this->camera->getPosition().x;
		Ogre::Real mostRightUpX = this->mostRightUp.x;
		Ogre::Real borderXRight = cameraPositionX + mostRightUpX;
		Ogre::Real borderXLeft = cameraPositionX - mostRightUpX;
		if (borderXRight > this->maximumBounds.x + this->borderOffset.x)
		{
			Ogre::Vector3 camPos = this->camera->getPosition();
			this->camera->setPosition(this->maximumBounds.x - this->mostRightUp.x, camPos.y, camPos.z);
		}
		else if (borderXLeft < this->minimumBounds.x - this->borderOffset.x)
		{
			Ogre::Vector3 camPos = this->camera->getPosition();
			Ogre::Vector3 newPos = Ogre::Vector3(this->minimumBounds.x + this->mostRightUp.x + this->borderOffset.x, camPos.y, camPos.z);
			this->camera->setPosition(newPos);
		}

		// y bounds
		if (this->camera->getPosition().y + this->mostRightUp.y > this->maximumBounds.y + this->borderOffset.y)
		{
			Ogre::Vector3 camPos = this->camera->getPosition();
			this->camera->setPosition(camPos.x, this->maximumBounds.y - this->mostRightUp.y, camPos.z);
		}
		else if (this->camera->getPosition().y + this->mostRightUp.y < this->minimumBounds.y - this->borderOffset.y)
		{
			Ogre::Vector3 camPos = this->camera->getPosition();
			this->camera->setPosition(camPos.x, this->minimumBounds.y + this->mostRightUp.y, camPos.z);
		}

		Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[FollowCamera2D] Position: " + Ogre::StringConverter::toString(this->camera->getPosition()));

		//////////////////////////////////////////////////////////////////////////////////////////////
		//// Tries to show the game object, no matter what other object tries to hide the game object
#if 0
		if (this->showGameObject)
		{
			/*Ogre::Vector3 direction = this->sceneNode->getPosition() - Ogre::Vector3(this->sceneNode->getPosition().x, this->sceneNode->getPosition().y, cameraPosition.z);
			Ogre::AxisAlignedBox boundingBox = this->sceneNode->getAttachedObject(0)->getBoundingBox();
			Ogre::Vector3 size = boundingBox.getHalfSize() * this->sceneNode->getScale();
			direction.y += size.y;
			direction.normalise();

			Ogre::Ray ray(Ogre::Vector3(this->sceneNode->getPosition().x, this->sceneNode->getPosition().y + size.y, cameraPosition.z), 
				Ogre::Vector3(this->sceneNode->getPosition().x, this->sceneNode->getPosition().y + size.y, this->sceneNode->getPosition().z) - cameraPosition);*/
			Ogre::Vector3 camOffsetPos = cameraPosition /*+ Ogre::Vector3(0.0f, 5.0f, 0.0f)*/;
			Ogre::Vector3 direction = this->sceneNode->getPosition() - camOffsetPos;
			// direction += this->camera->getDirection();
			Ogre::v1::Entity* entity = static_cast<Ogre::v1::Entity*>(this->sceneNode->getAttachedObject(0));
			if (nullptr != entity)
			{
				Ogre::AxisAlignedBox boundingBox = entity->getMesh()->getBounds();
				Ogre::Vector3 size = boundingBox.getHalfSize() * this->sceneNode->getScale();
				direction.y += size.y * 0.5f;
			}
			// direction.normalise();
			
			Ogre::Ray ray(camOffsetPos, direction);

			//this->pDebugLine->clear();

			//this->pDebugLine->begin("RedLine", Ogre::RenderOperation::OT_LINE_LIST);
			//this->pDebugLine->position(cameraPosition);
			////Position + Richtung * länge
			//this->pDebugLine->position(cameraPosition + (direction));
			////this->pDebugLine->position(enemyPos + (quat * Ogre::Vector3::NEGATIVE_UNIT_Z) * 10);
			//this->pDebugLine->end();

			this->raySceneQuery->setRay(ray);
			this->raySceneQuery->setSortByDistance(true);
			Ogre::RaySceneQueryResult& result = this->raySceneQuery->execute();
			Ogre::RaySceneQueryResult::const_iterator it = result.cbegin();
			bool foundPlayer = true;

			for (auto& object : result)
			{
				if (object.movable)
				{
					Ogre::SceneNode* tempNode = object.movable->getParentSceneNode();
					GameObject* gameObject = nullptr;
					try
					{

						gameObject = Ogre::any_cast<GameObject*>(tempNode->getUserAny());
					}
					catch (...)
					{

					}
					if (tempNode == this->hiddenSceneNode)
					{
						break;
					}
					Ogre::v1::Entity* entity = dynamic_cast<Ogre::v1::Entity*>(object.movable);
					// Ogre::LogManager::getSingletonPtr()->logMessage("Hiding object: " + entity->getName());
					if ((gameObject && this->category != gameObject->getCategory()) || !gameObject)
					{
						
						if (entity)
						{
							
							for (unsigned int i = 0; i < entity->getNumSubEntities(); i++)
							{
								// http://www.ogre3d.org/forums/viewtopic.php?t=19405

								Ogre::MaterialPtr mat = entity->getSubEntity(i)->getMaterial();

								Ogre::Pass* pass = mat->getTechnique(0)->getPass(0);
								// Here via macroblock
								// pass->setSceneBlending(Ogre::SceneBlendType::SBT_TRANSPARENT_ALPHA);
								// pass->setDepthWriteEnabled(false);
								Ogre::TextureUnitState* textureUnitState = pass->getTextureUnitState(0);
								textureUnitState->setAlphaOperation(Ogre::LBX_SOURCE1, Ogre::LBS_MANUAL, Ogre::LBS_TEXTURE, 0.25f);

								entity->getSubEntity(i)->setMaterial(mat);

									// http://www.ogre3d.org/forums/viewtopic.php?f=2&t=73259
								   /*if (ShaderModule::LNONE != ShaderModule::getInstance()->getShaderLightParameter().shaderLightTechnique)
								   {
								   Ogre::Pass* pass = mat->getTechnique(1)->getPass(0);
								   pass->setSceneBlending(Ogre::SceneBlendType::SBT_TRANSPARENT_ALPHA);
								   pass->setDepthWriteEnabled(false);
								   Ogre::TextureUnitState* textureUnitState = pass->getTextureUnitState(0);
								   textureUnitState->setAlphaOperation(Ogre::LBX_SOURCE1, Ogre::LBS_MANUAL, Ogre::LBS_TEXTURE, 0.25f);
								   }*/

								this->hiddenSceneNode = tempNode;
							}

							foundPlayer = false;
							break;
						}
					}
					else
					{
						if (this->hiddenSceneNode)
						{
							this->hiddenSceneNode->setVisible(true);

							Ogre::v1::Entity* entity = dynamic_cast<Ogre::v1::Entity*>(this->hiddenSceneNode->getAttachedObject(0));
							if (entity)
							{
								Ogre::LogManager::getSingletonPtr()->logMessage("Showing object: " + entity->getName());
								for (unsigned int i = 0; i < entity->getNumSubEntities(); i++)
								{
									Ogre::MaterialPtr mat = entity->getSubEntity(i)->getMaterial();
									Ogre::Pass* pass = mat->getTechnique(0)->getPass(0);
									// Here via macroblock
									// pass->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
									// pass->setDepthWriteEnabled(true);
									Ogre::TextureUnitState* textureUnitState = pass->getTextureUnitState(0);
									textureUnitState->setAlphaOperation(Ogre::LBX_SOURCE1, Ogre::LBS_MANUAL, Ogre::LBS_TEXTURE, 1.0f);
								}

							}

							this->hiddenSceneNode = nullptr;
						}
						foundPlayer = true;
						break;
					}

				}
			}
		}
#endif
	}

#endif

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
			// set the camera position to the target one for the first time
			this->camera->setPosition(this->sceneNode->getPosition());
			// this->camera->lookAt(this->sceneNode->getPosition());
			// this->camera->moveRelative(this->offset);
			this->camera->move(this->offset);

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

		this->camera->move(velocity * this->moveCameraWeight);

		this->lastMoveValue = velocity;

		//////////////////////////////////////////////////////////////////////////////////////////////
		//// If the camera is out of bounds, constrain its position to remain within bounds
		// x bounds
		Ogre::Real cameraPositionX = cameraPosition.x;
		Ogre::Real mostRightUpX = this->mostRightUp.x;
		Ogre::Real borderXRight = cameraPositionX + mostRightUpX;
		Ogre::Real borderXLeft = cameraPositionX - mostRightUpX;
		if (borderXRight > this->maximumBounds.x + this->borderOffset.x)
		{
			this->camera->setPosition(this->maximumBounds.x - this->mostRightUp.x, cameraPosition.y, cameraPosition.z);
		}
		else if (borderXLeft < this->minimumBounds.x - this->borderOffset.x)
		{
			Ogre::Vector3 newPos = Ogre::Vector3(this->minimumBounds.x + this->mostRightUp.x + this->borderOffset.x, cameraPosition.y, cameraPosition.z);
			this->camera->setPosition(newPos);
		}

		// y bounds
		if (this->camera->getPosition().y + this->mostRightUp.y > this->maximumBounds.y + this->borderOffset.y)
		{
			this->camera->setPosition(cameraPosition.x, this->maximumBounds.y - this->mostRightUp.y, cameraPosition.z);
		}
		else if (this->camera->getPosition().y + this->mostRightUp.y < this->minimumBounds.y - this->borderOffset.y)
		{
			this->camera->setPosition(cameraPosition.x, this->minimumBounds.y + this->mostRightUp.y, cameraPosition.z);
		}

		//////////////////////////////////////////////////////////////////////////////////////////////
		//// Tries to show the game object, no matter what other object tries to hide the game object
#if 0
		if (this->showGameObject)
		{
			/*Ogre::Vector3 direction = this->sceneNode->getPosition() - Ogre::Vector3(this->sceneNode->getPosition().x, this->sceneNode->getPosition().y, cameraPosition.z);
			Ogre::AxisAlignedBox boundingBox = this->sceneNode->getAttachedObject(0)->getBoundingBox();
			Ogre::Vector3 size = boundingBox.getHalfSize() * this->sceneNode->getScale();
			direction.y += size.y;
			direction.normalise();

			Ogre::Ray ray(Ogre::Vector3(this->sceneNode->getPosition().x, this->sceneNode->getPosition().y + size.y, cameraPosition.z),
				Ogre::Vector3(this->sceneNode->getPosition().x, this->sceneNode->getPosition().y + size.y, this->sceneNode->getPosition().z) - cameraPosition);*/
			Ogre::Vector3 camOffsetPos = cameraPosition /*+ Ogre::Vector3(0.0f, 5.0f, 0.0f)*/;
			Ogre::Vector3 direction = this->sceneNode->getPosition() - camOffsetPos;
			// direction += this->camera->getDirection();
			Ogre::v1::Entity* entity = static_cast<Ogre::v1::Entity*>(this->sceneNode->getAttachedObject(0));
			if (nullptr != entity)
			{
				Ogre::AxisAlignedBox boundingBox = entity->getMesh()->getBounds();
				Ogre::Vector3 size = boundingBox.getHalfSize() * this->sceneNode->getScale();
				direction.y += size.y * 0.5f;
			}
			// direction.normalise();

			Ogre::Ray ray(camOffsetPos, direction);

			//this->pDebugLine->clear();

			//this->pDebugLine->begin("RedLine", Ogre::RenderOperation::OT_LINE_LIST);
			//this->pDebugLine->position(cameraPosition);
			////Position + Richtung * länge
			//this->pDebugLine->position(cameraPosition + (direction));
			////this->pDebugLine->position(enemyPos + (quat * Ogre::Vector3::NEGATIVE_UNIT_Z) * 10);
			//this->pDebugLine->end();

			this->raySceneQuery->setRay(ray);
			this->raySceneQuery->setSortByDistance(true);
			Ogre::RaySceneQueryResult& result = this->raySceneQuery->execute();
			Ogre::RaySceneQueryResult::const_iterator it = result.cbegin();
			bool foundPlayer = true;

			for (auto& object : result)
			{
				if (object.movable)
				{
					Ogre::SceneNode* tempNode = object.movable->getParentSceneNode();
					GameObject* gameObject = nullptr;
					try
					{

						gameObject = Ogre::any_cast<GameObject*>(tempNode->getUserAny());
					}
					catch (...)
					{

					}
					if (tempNode == this->hiddenSceneNode)
					{
						break;
					}
					Ogre::v1::Entity* entity = dynamic_cast<Ogre::v1::Entity*>(object.movable);
					// Ogre::LogManager::getSingletonPtr()->logMessage("Hiding object: " + entity->getName());
					if ((gameObject && this->category != gameObject->getCategory()) || !gameObject)
					{

						if (entity)
						{

							for (unsigned int i = 0; i < entity->getNumSubEntities(); i++)
							{
								// http://www.ogre3d.org/forums/viewtopic.php?t=19405

								Ogre::MaterialPtr mat = entity->getSubEntity(i)->getMaterial();

								Ogre::Pass* pass = mat->getTechnique(0)->getPass(0);
								// Here via macroblock
								// pass->setSceneBlending(Ogre::SceneBlendType::SBT_TRANSPARENT_ALPHA);
								// pass->setDepthWriteEnabled(false);
								Ogre::TextureUnitState* textureUnitState = pass->getTextureUnitState(0);
								textureUnitState->setAlphaOperation(Ogre::LBX_SOURCE1, Ogre::LBS_MANUAL, Ogre::LBS_TEXTURE, 0.25f);

								entity->getSubEntity(i)->setMaterial(mat);

								// http://www.ogre3d.org/forums/viewtopic.php?f=2&t=73259
							   /*if (ShaderModule::LNONE != ShaderModule::getInstance()->getShaderLightParameter().shaderLightTechnique)
							   {
							   Ogre::Pass* pass = mat->getTechnique(1)->getPass(0);
							   pass->setSceneBlending(Ogre::SceneBlendType::SBT_TRANSPARENT_ALPHA);
							   pass->setDepthWriteEnabled(false);
							   Ogre::TextureUnitState* textureUnitState = pass->getTextureUnitState(0);
							   textureUnitState->setAlphaOperation(Ogre::LBX_SOURCE1, Ogre::LBS_MANUAL, Ogre::LBS_TEXTURE, 0.25f);
							   }*/

								this->hiddenSceneNode = tempNode;
							}

							foundPlayer = false;
							break;
						}
					}
					else
					{
						if (this->hiddenSceneNode)
						{
							this->hiddenSceneNode->setVisible(true);

							Ogre::v1::Entity* entity = dynamic_cast<Ogre::v1::Entity*>(this->hiddenSceneNode->getAttachedObject(0));
							if (entity)
							{
								Ogre::LogManager::getSingletonPtr()->logMessage("Showing object: " + entity->getName());
								for (unsigned int i = 0; i < entity->getNumSubEntities(); i++)
								{
									Ogre::MaterialPtr mat = entity->getSubEntity(i)->getMaterial();
									Ogre::Pass* pass = mat->getTechnique(0)->getPass(0);
									// Here via macroblock
									// pass->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
									// pass->setDepthWriteEnabled(true);
									Ogre::TextureUnitState* textureUnitState = pass->getTextureUnitState(0);
									textureUnitState->setAlphaOperation(Ogre::LBX_SOURCE1, Ogre::LBS_MANUAL, Ogre::LBS_TEXTURE, 1.0f);
								}

							}

							this->hiddenSceneNode = nullptr;
						}
						foundPlayer = true;
						break;
					}

				}
			}
		}
#endif

		// Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[FollowCamera2D] Position: " + Ogre::StringConverter::toString(this->camera->getPosition()));
	}

#if 0
	void FollowCamera2D::moveCamera(Ogre::Real dt)
	{
		// Error: No bounds have been set
		if (Ogre::Vector3::ZERO == this->mostRightUp)
			return;

		if (nullptr == this->sceneNode)
			return;

		if (this->firstTimeMoveValueSet)
		{
			this->lastMoveValue = Ogre::Vector3::ZERO;
			// set the camera position to the target one for the first time
			this->camera->getParentNode()->_setDerivedPosition(this->sceneNode->getPosition());
			this->camera->getParentNode()->translate(this->offset);

			this->firstTimeMoveValueSet = false;
		}

		const Ogre::Vector3 cameraPosition = this->camera->getParentNode()->_getDerivedPosition();

		//////////////////////////////////////////////////////////////////////////////////////////////
		//// The camera follows the game object as long as it is within the virtual environments bounds
		Ogre::Vector3 velocity = Ogre::Vector3::ZERO;
		// also add the offset, by which the player differentiates from the camera, to get the correct position
		if (this->sceneNode->getPosition().x + this->offset.x - this->mostRightUp.x > this->minimumBounds.x
			&& this->sceneNode->getPosition().x + this->offset.x + this->mostRightUp.x < this->maximumBounds.x)
		{
			velocity.x = this->sceneNode->getPosition().x - cameraPosition.x + this->offset.x;

			if (0.0f != this->smoothValue)
				velocity.x = NOWA::MathHelper::getInstance()->lowPassFilter(velocity.x, this->lastMoveValue.x, this->smoothValue);

			this->lastMoveValue.x = velocity.x;
		}
		// check the y bounds
		if (this->sceneNode->getPosition().y + this->offset.y - this->mostRightUp.y > this->minimumBounds.y
			&& this->sceneNode->getPosition().y + this->offset.y + this->mostRightUp.y < this->maximumBounds.y)
		{
			velocity.y = this->sceneNode->getPosition().y - cameraPosition.y + this->offset.y;
			if (0.0f != this->smoothValue)
				velocity.y = NOWA::MathHelper::getInstance()->lowPassFilter(velocity.y, this->lastMoveValue.y, this->smoothValue);

			this->lastMoveValue.y = velocity.y;
		}

		this->camera->getParentNode()->translate(velocity);

		// Ogre::Vector3 cameraPosition = this->camera->getParentNode()->_getDerivedPosition();

		//////////////////////////////////////////////////////////////////////////////////////////////
		//// If the camera is out of bounds, constrain its position to remain within bounds
		// x bounds
		Ogre::Real cameraPositionX = cameraPosition.x;
		Ogre::Real mostRightUpX = this->mostRightUp.x;
		Ogre::Real borderXRight = cameraPositionX + mostRightUpX;
		Ogre::Real borderXLeft = cameraPositionX - mostRightUpX;
		if (borderXRight > this->maximumBounds.x + this->borderOffset.x)
		{
			Ogre::Vector3 newPos = Ogre::Vector3(this->maximumBounds.x - this->mostRightUp.x, cameraPosition.y, cameraPosition.z);
			this->camera->getParentNode()->_setDerivedPosition(newPos);
		}
		else if (borderXLeft < this->minimumBounds.x - this->borderOffset.x)
		{
			Ogre::Vector3 newPos = Ogre::Vector3(this->minimumBounds.x + this->mostRightUp.x + this->borderOffset.x, cameraPosition.y, cameraPosition.z);
			this->camera->getParentNode()->_setDerivedPosition(newPos);
		}

		// y bounds
		if (cameraPosition.y + this->mostRightUp.y > this->maximumBounds.y + this->borderOffset.y)
		{
			Ogre::Vector3 newPos = Ogre::Vector3(cameraPosition.x, this->maximumBounds.y - this->mostRightUp.y, cameraPosition.z);
			this->camera->getParentNode()->_setDerivedPosition(newPos);
		}
		else if (cameraPosition.y + this->mostRightUp.y < this->minimumBounds.y - this->borderOffset.y)
		{
			Ogre::Vector3 newPos = Ogre::Vector3(cameraPosition.x, this->minimumBounds.y + this->mostRightUp.y, cameraPosition.z);
			this->camera->getParentNode()->_setDerivedPosition(newPos);
		}

		Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[FollowCamera2D] Position: " + Ogre::StringConverter::toString(this->camera->getParentNode()->_getDerivedPosition()));
	}
#endif

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

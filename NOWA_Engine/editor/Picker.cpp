#include "NOWAPrecompiled.h"
#include "Picker.h"
#include "gameobject/JointComponents.h"
#include "main/AppStateManager.h"

namespace NOWA
{
	Picker::PickForceObserver::PickForceObserver()
		: PhysicsActiveComponent::IForceObserver()
	{
		
	}

	void Picker::PickForceObserver::onForceAdd(OgreNewt::Body* body, Ogre::Real timeStep, int threadIndex)
	{
		IPicker* picker = this->getPicker();

		if (nullptr == picker)
		{
			return;
		}

		Ogre::Ray mouseRay = picker->getRayFromMouse();
		// Get the global position our cursor is at
		Ogre::Vector3 cursorPos = mouseRay.getPoint(picker->getDragDistance());

		Ogre::Quaternion bodyOrientation;
		Ogre::Vector3 bodyPos;

		// Now find the global point on the body
		body->getPositionOrientation(bodyPos, bodyOrientation);

		// Find the handle position we are holding the body from
		Ogre::Vector3 dragPos = (bodyOrientation * picker->getDragPoint()) + bodyPos;

		Ogre::Vector3 inertia;
		Ogre::Real mass;

		body->getMassMatrix(mass, inertia);

		Ogre::Vector3 dragForce = Ogre::Vector3::ZERO;

		if (Ogre::Math::RealEqual(picker->getPickForce(), 50.0f))
		{
			body->setPositionOrientation(cursorPos, Ogre::Quaternion::IDENTITY);
			// Annulate gravity
			body->addForce(body->getGravity() * mass);
			picker->destroyLine();
		}
		else
		{
			// Calculate picking spring force
			Ogre::Vector3 dragForce = ((cursorPos - dragPos) * mass * picker->getPickForce()) - body->getVelocity();
			if (picker->getDrawLine())
			{
				picker->drawLine(cursorPos, dragPos);
			}

			// body->addForce(body->getGravity() * mass);

			// Add the picking spring force at the handle
			body->addGlobalForce(dragForce, dragPos);
		}
	}

	/////////////////////////////////////////////////////////////////////////////

	Picker::Picker()
		: active(false),
		dragComponent(nullptr),
		hitBody(nullptr),
		mousePosition(Ogre::Vector2::ZERO),
		renderWindow(nullptr),
		camera(nullptr),
		pickForce(30.0f),
		dragDistance(0.0f),
		maxDistance(100.0f),
		dragPoint(Ogre::Vector3::ZERO),
		dragging(false),
		drawLines(false),
		queryMask(0 << 0),
		gameObjectId(0),
		dragLineNode(nullptr),
		dragLineObject(nullptr)
	{

	}

	Picker::~Picker()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[Picker] Destroying picker");
		this->dragComponent = nullptr;

		if (this->hitBody)
		{               
			this->release();
		}

		if (this->active)
		{
			if (this->drawLines)
			{
				this->destroyLine();
			}
			this->active = false;
		}
		this->detachAndDestroyAllPickObserver();
		AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &Picker::deleteBodyDelegate), EventDataDeleteBody::getStaticEventType());
	}

	void Picker::init(Ogre::SceneManager* sceneManager, Ogre::Camera* camera, Ogre::Real maxDistance, unsigned int queryMask, bool drawLines)
	{
		this->sceneManager = sceneManager;
		this->camera = camera;
		this->maxDistance = maxDistance;
		this->queryMask = queryMask;
		this->drawLines = drawLines;
		this->dragging = false;

		if (true == this->drawLines)
		{
			this->createLine();
		}

		this->active = true;

		AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &Picker::deleteBodyDelegate), EventDataDeleteBody::getStaticEventType());
	}

	void Picker::attachPickObserver(IPickObserver* pickObserver)
	{
		this->pickObservers.emplace_back(pickObserver);
	}

	void Picker::detachAndDestroyPickObserver(IPickObserver* pickObserver)
	{
		auto it = std::find(this->pickObservers.cbegin(), this->pickObservers.cend(), pickObserver);
		if (it != this->pickObservers.end())
		{
			IPickObserver* observer = *it;
			this->pickObservers.erase(it);
			delete observer;
			observer = nullptr;
		}
	}

	void Picker::detachAndDestroyAllPickObserver(void)
	{
		for (auto& it = this->pickObservers.cbegin(); it != this->pickObservers.cend();)
		{
			IPickObserver* observer = *it;
			it = this->pickObservers.erase(it);
			delete observer;
			observer = nullptr;
		}
		this->pickObservers.clear();
	}

	Ogre::Ray Picker::getRayFromMouse(void) const
	{
		// Calculate normalised mouse position [0..1]
		Ogre::Real mx = static_cast<Ogre::Real>(this->mousePosition.x) / static_cast<Ogre::Real>(this->renderWindow->getWidth());
		Ogre::Real my = static_cast<Ogre::Real>(this->mousePosition.y) / static_cast<Ogre::Real>(this->renderWindow->getHeight());

		// Get the global position our cursor is at
		Ogre::Ray ray;
		ENQUEUE_GET_CAMERA_TO_VIEWPORT_RAY(this->camera, mx, my, ray);

		return ray;
	}

	Ogre::Real Picker::getDragDistance(void) const
	{
		return this->dragDistance;
	}

	Ogre::Vector3 Picker::getDragPoint(void) const
	{
		return this->dragPoint;
	}

	bool Picker::getDrawLine(void) const
	{
		return this->drawLines;
	}

	void Picker::createLine(void)
	{
		this->destroyingLine.store(false, std::memory_order_release);

		ENQUEUE_RENDER_COMMAND_WAIT("Picker::createLine",
		{
			this->dragLineNode = this->sceneManager->getRootSceneNode()->createChildSceneNode();
			// this->dragLineObject = new Ogre::v1::ManualObject(0, &this->sceneManager->_getEntityMemoryManager(Ogre::SCENE_DYNAMIC), this->sceneManager);
			this->dragLineObject = this->sceneManager->createManualObject();
			this->dragLineObject->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_MESH);
			this->dragLineObject->setName("PickerDragLines");
			this->dragLineObject->setQueryFlags(0 << 0);
			this->dragLineObject->setCastShadows(false);
			this->dragLineNode->attachObject(this->dragLineObject);
		});
	}

	void Picker::drawLine(const Ogre::Vector3& startPosition, const Ogre::Vector3& endPosition)
	{
		if (this->destroyingLine.load(std::memory_order_acquire))
		{
			return;
		}

		if (this->dragLineNode == nullptr)
		{
			this->createLine();
		}

		auto closureFunction = [this, startPosition, endPosition](Ogre::Real weight)
		{
			if (nullptr == this->dragLineObject)
			{
				return;
			}

			// Draw a 3D line between these points for visual effect
			this->dragLineObject->clear();
			this->dragLineObject->begin("WhiteNoLightingBackground", Ogre::OperationType::OT_LINE_LIST);
			this->dragLineObject->position(startPosition);
			this->dragLineObject->index(0);
			this->dragLineObject->position(endPosition);
			this->dragLineObject->index(1);
			this->dragLineObject->end();
		};
		Ogre::String id = "Picker_drawLine_" + camera->getName();
		NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction, false);
	}

	void Picker::destroyLine()
	{
		if (this->dragLineNode == nullptr)
		{
			return;
		}

		this->destroyingLine.store(true, std::memory_order_release);

		Ogre::String id = "Picker_drawLine_" + camera->getName();
		NOWA::GraphicsModule::getInstance()->removeTrackedClosure(id);

		auto localSceneManager = this->sceneManager;
		auto localDragLineNode = this->dragLineNode;
		auto localDragLineObject = this->dragLineObject;

		ENQUEUE_RENDER_COMMAND_MULTI_WAIT("Picker::destroyLine", _3(localSceneManager, localDragLineNode, localDragLineObject),
		{
			localDragLineObject->clear();
			localDragLineNode->detachAllObjects();
			if (localDragLineObject)
			{
				localSceneManager->destroyManualObject(localDragLineObject);
			}
			if (localDragLineNode->getParentSceneNode())
			{
				localDragLineNode->getParentSceneNode()->removeAndDestroyChild(localDragLineNode);
			}
		});

		this->dragLineObject = nullptr;
		this->dragLineNode = nullptr;
	}

	void Picker::updateVisual()
	{
		if (!this->drawLines || !this->hitBody)
			return;

		Ogre::Ray mouseRay = this->getRayFromMouse();
		Ogre::Vector3 cursorPos = mouseRay.getPoint(this->getDragDistance());

		Ogre::Quaternion q;
		Ogre::Vector3 p;
		this->hitBody->getPositionOrientation(p, q);
		Ogre::Vector3 dragPos = (q * this->dragPoint) + p;

		this->drawLine(cursorPos, dragPos);
	}

	PhysicsComponent* Picker::grab(OgreNewt::World* ogreNewt, const Ogre::Vector2 position, Ogre::Window* renderWindow, Ogre::Real pickForce)
	{
		this->mousePosition = position;
		this->renderWindow = renderWindow;
		this->pickForce = pickForce;
		if (this->pickForce < 1.0f)
		{
			this->pickForce = 1.0f;
		}
		else if (this->pickForce > 50.0f)
		{
			this->pickForce = 50.0f;
		}

		if (false == active || nullptr == this->sceneManager)
		{
			return nullptr;
		}

		this->updateVisual();

		if (false == this->dragging)
		{
			// Get ray origin and end coordinates
			Ogre::Ray mouseRay = getRayFromMouse();

			// Cast a ray between these points and check for first hit
			OgreNewt::BasicRaycast* ray = new OgreNewt::BasicRaycast(ogreNewt, mouseRay.getOrigin(), mouseRay.getPoint(this->maxDistance), true);
			OgreNewt::BasicRaycast::BasicRaycastInfo info = ray->getFirstHit();

			// Found a body in the ray's path
			if (nullptr != info.mBody)
			{
				// check its query mask
				unsigned int type = info.mBody->getType();
				unsigned int finalType = type & this->queryMask;
				if (type == finalType)
				{
					this->hitBody = info.mBody;

					// Calculate hit global and local position (info.mDistance is in the range [0,1])
					Ogre::Vector3 globalPos = mouseRay.getPoint(this->maxDistance * info.mDistance);
					Ogre::Vector3 localPos = Ogre::Vector3::ZERO;
					if (Ogre::Math::RealEqual(this->pickForce, 50.0f))
					{
						this->hitBody->setPositionOrientation(globalPos, this->hitBody->getOrientation());
					}
					else
					{
						Ogre::Vector3 bodyPos;
						Ogre::Quaternion bodyOrientation;

						// Store the body position and orientation
						this->hitBody->getPositionOrientation(bodyPos, bodyOrientation);
						localPos = bodyOrientation.Inverse() * (globalPos - bodyPos);
					}

					// Try to cast to physics component
					this->dragComponent = OgreNewt::any_cast<PhysicsComponent*>(this->hitBody->getUserData());

					if (nullptr != this->dragComponent)
					{
						// if this is a physics component, try to cast to an active one and subscribe, to be called in the force and torque callback of the active component
						// to integrate its force there without disrupting other forces
						PhysicsActiveComponent* physicsActiveComponent = dynamic_cast<PhysicsActiveComponent*>(this->dragComponent);
						if (nullptr != physicsActiveComponent)
						{
							if (this->drawLines)
							{
								this->createLine();
							}
							Picker::PickForceObserver* pickForceObserver = new Picker::PickForceObserver();
							pickForceObserver->setPicker(this);
							pickForceObserver->setName(this->dragComponent->getOwner()->getName() + "_Picker" + Ogre::StringConverter::toString(this->dragComponent->getIndex()));
							physicsActiveComponent->attachForceObserver(pickForceObserver);
						}
					}

					this->dragDistance = (this->maxDistance * info.mDistance);

					if (nullptr != this->dragComponent)
					{
						for (auto& it = this->pickObservers.cbegin(); it != this->pickObservers.cend(); ++it)
						{
							// notify the observer
							(*it)->onPick(this->dragComponent->getOwner().get());
						}
					}
					this->dragging = true;
				}

				delete ray;
			}
		}
		return this->dragComponent;
	}

	void Picker::deleteBodyDelegate(EventDataPtr eventData)
	{
		boost::shared_ptr<EventDataDeleteBody> castEventData = boost::static_pointer_cast<EventDataDeleteBody>(eventData);

		if (castEventData->getBody() == this->hitBody)
		{
			this->release();
		}
	}

	void Picker::release(void)
	{
		if (this->hitBody)
		{
			for (auto& it = this->pickObservers.cbegin(); it != this->pickObservers.cend(); ++it)
			{
				// notify the observer
				(*it)->onRelease();
			}

			if (nullptr != this->dragComponent)
			{
				PhysicsActiveComponent* physicsActiveComponent = dynamic_cast<PhysicsActiveComponent*>(this->dragComponent);
				if (nullptr != physicsActiveComponent)
				{
					physicsActiveComponent->detachAndDestroyForceObserver(this->dragComponent->getOwner()->getName() + "_Picker" + Ogre::StringConverter::toString(this->dragComponent->getIndex()));
				}
			}
		}

		if (this->getDrawLine())
		{
			this->destroyLine();
		}
		// Reinitialise dragging information
		this->dragComponent = nullptr;
		if (0 == this->gameObjectId)
		{
			this->hitBody = nullptr;
		}
		this->dragPoint = Ogre::Vector3::ZERO;
		this->dragDistance = 0.0f;
		this->dragging = false;
	}

	void Picker::updateQueryMask(unsigned int newQueryMask)
	{
		this->queryMask = newQueryMask;
	}

	Ogre::Real Picker::getPickForce(void) const
	{
		return this->pickForce;
	}

	bool Picker::getIsDragging(void) const
	{
		return this->dragging;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GameObjectPicker::PickForceObserver::PickForceObserver()
		: PhysicsActiveComponent::IForceObserver()
	{

	}

	void GameObjectPicker::PickForceObserver::onForceAdd(OgreNewt::Body* body, Ogre::Real timeStep, int threadIndex)
	{
		IPicker* picker = this->getPicker();

		if (nullptr == picker)
		{
			return;
		}

		Ogre::Ray mouseRay = picker->getRayFromMouse();
		// Get the global position our cursor is at
		Ogre::Vector3 cursorPos = mouseRay.getPoint(picker->getDragDistance());

		Ogre::Quaternion bodyOrientation;
		Ogre::Vector3 bodyPos;

		// Now find the global point on the body
		body->getPositionOrientation(bodyPos, bodyOrientation);

		// Find the handle position we are holding the body from
		Ogre::Vector3 dragPos = (bodyOrientation * picker->getDragPoint()) + (bodyPos + static_cast<GameObjectPicker*>(picker)->offsetPosition);

		Ogre::Vector3 inertia;
		Ogre::Real mass;

		body->getMassMatrix(mass, inertia);

		// Calculate picking spring force
		Ogre::Vector3 dragForce = Ogre::Vector3::ZERO;

		if (Ogre::Math::RealEqual(picker->getPickForce(), 50.0f))
		{
			body->setPositionOrientation(cursorPos, Ogre::Quaternion::IDENTITY);
			// Annulate gravity
			body->addForce(body->getGravity() * mass);
			picker->destroyLine();
		}
		else
		{
			Ogre::Real length = (cursorPos - dragPos).length();
			// Ogre::LogManager::getSingletonPtr()->logMessage("length: " + Ogre::StringConverter::toString(this->dragDistance));
			if (length < static_cast<GameObjectPicker*>(picker)->dragAffectDistance)
			{
				Ogre::Vector3 dragForce = ((cursorPos - dragPos) * mass * (static_cast<Ogre::Real>(picker->getPickForce())))/* - body->getVelocity()*/;
				if (picker->getDrawLine())
				{
					picker->drawLine(cursorPos, dragPos);
				}
				// Add the picking spring force at the handle
				body->addGlobalForce(dragForce, dragPos);
			}
			else
			{
				if (picker->getDrawLine())
				{
					picker->destroyLine();
				}
			}
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GameObjectPicker::GameObjectPicker()
		: active(false),
		dragComponent(nullptr),
		hitBody(nullptr),
		jointId(0),
		mousePosition(Ogre::Vector2::ZERO),
		renderWindow(nullptr),
		camera(nullptr),
		pickForce(30.0f),
		dragDistance(0.0f),
		maxDistance(100.0f),
		dragPoint(Ogre::Vector3::ZERO),
		dragging(false),
		drawLines(false),
		queryMask(0 << 0),
		gameObjectId(0),
		dragLineNode(nullptr),
		dragLineObject(nullptr),
		dragAffectDistance(0.0f),
		offsetPosition(Ogre::Vector3::ZERO),
		dragPos(Ogre::Vector3::ZERO),
		cursorPos(Ogre::Vector3::ZERO)
	{
		AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &GameObjectPicker::deleteJointDelegate), EventDataDeleteJoint::getStaticEventType());
		AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &GameObjectPicker::deleteBodyDelegate), EventDataDeleteBody::getStaticEventType());
	}

	GameObjectPicker::~GameObjectPicker()
	{
		AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &GameObjectPicker::deleteJointDelegate), EventDataDeleteJoint::getStaticEventType());
		AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &GameObjectPicker::deleteJointDelegate), EventDataDeleteBody::getStaticEventType());

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[GameObjectPicker] Destroying game object picker");
		this->dragComponent = nullptr;
		if (this->active)
		{
			if (this->drawLines)
			{
				this->destroyLine();
			}
			this->active = false;
		}
	}

	void GameObjectPicker::init(Ogre::SceneManager* sceneManager, Ogre::Camera* camera, Ogre::Real maxDistance, unsigned long gameObjectId)
	{
		this->sceneManager = sceneManager;
		this->camera = camera;
		this->maxDistance = maxDistance;
		this->gameObjectId = gameObjectId;
		this->dragging = false;

		auto gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(gameObjectId);
		if (nullptr != gameObjectPtr)
		{
			auto physicsCompPtr = NOWA::makeStrongPtr(gameObjectPtr->getComponent<PhysicsComponent>());
			if (nullptr != physicsCompPtr)
			{
				this->hitBody = physicsCompPtr->getBody();
			}
		}

		this->active = true;
	}

	void GameObjectPicker::actualizeData(Ogre::Camera* camera, unsigned long gameObjectId, bool drawLines)
	{
		this->camera = camera;
		this->gameObjectId = gameObjectId;
		this->jointId = 0;
		this->drawLines = drawLines;
		this->hitBody = nullptr;

		auto gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(gameObjectId);
		if (nullptr != gameObjectPtr)
		{
			auto physicsCompPtr = NOWA::makeStrongPtr(gameObjectPtr->getComponent<PhysicsComponent>());
			if (nullptr != physicsCompPtr)
			{
				this->hitBody = physicsCompPtr->getBody();
			}
		}

		if (true == this->drawLines)
		{
			this->createLine();
		}
	}

	void GameObjectPicker::actualizeData2(Ogre::Camera* camera, unsigned long jointId, bool drawLines)
	{
		this->camera = camera;
		this->gameObjectId = 0;
		this->drawLines = drawLines;
		this->jointId = jointId;
		this->hitBody = nullptr;
		auto jointCompPtr = NOWA::makeStrongPtr(AppStateManager::getSingletonPtr()->getGameObjectController()->getJointComponent(jointId));
		if (nullptr != jointCompPtr)
		{
			this->hitBody = jointCompPtr->getBody();
		}
	}

	void GameObjectPicker::actualizeData3(Ogre::Camera* camera, OgreNewt::Body* body, bool drawLines)
	{
		this->camera = camera;
		this->gameObjectId = 0;
		this->hitBody = body;
		this->drawLines = drawLines;
		this->jointId = 0;
	}

	bool GameObjectPicker::getIsDragging(void) const
	{
		return this->dragging;
	}

	Ogre::Ray GameObjectPicker::getRayFromMouse(void) const
	{
		// Calculate normalised mouse position [0..1]
		Ogre::Real mx = static_cast<Ogre::Real>(this->mousePosition.x) / static_cast<Ogre::Real>(this->renderWindow->getWidth());
		Ogre::Real my = static_cast<Ogre::Real>(this->mousePosition.y) / static_cast<Ogre::Real>(this->renderWindow->getHeight());

		// Get the global position our cursor is at
		Ogre::Ray ray;
		ENQUEUE_GET_CAMERA_TO_VIEWPORT_RAY(this->camera, mx, my, ray);

		return ray;
	}

	Ogre::Real GameObjectPicker::getDragDistance(void) const
	{
		return this->dragDistance;
	}

	Ogre::Vector3 GameObjectPicker::getDragPoint(void) const
	{
		return this->dragPoint;
	}

	bool GameObjectPicker::getDrawLine(void) const
	{
		return this->drawLines;
	}

	void GameObjectPicker::createLine(void)
	{
		this->destroyingLine.store(false, std::memory_order_release);

		ENQUEUE_RENDER_COMMAND_WAIT("GameObjectPicker::createLine",
		{
			this->dragLineNode = this->sceneManager->getRootSceneNode()->createChildSceneNode();
			// this->dragLineObject = new Ogre::v1::ManualObject(0, &this->sceneManager->_getEntityMemoryManager(Ogre::SCENE_DYNAMIC), this->sceneManager);
			this->dragLineObject = this->sceneManager->createManualObject();
			this->dragLineObject->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_MESH);
			this->dragLineObject->setName("PickerDragLines");
			this->dragLineObject->setQueryFlags(0 << 0);
			this->dragLineObject->setCastShadows(false);
			this->dragLineNode->attachObject(this->dragLineObject);
		});
	}

	void GameObjectPicker::drawLine(const Ogre::Vector3& startPosition, const Ogre::Vector3& endPosition)
	{
		if (this->destroyingLine.load(std::memory_order_acquire))
		{
			return;
		}

		if (this->dragLineNode == nullptr)
		{
			this->createLine();
		}

		auto closureFunction = [this, startPosition, endPosition](Ogre::Real weight)
		{
			if (nullptr == this->dragLineObject)
			{
				return;
			}
			// Draw a 3D line between these points for visual effect
			this->dragLineObject->clear();
			this->dragLineObject->begin("WhiteNoLightingBackground", Ogre::OperationType::OT_LINE_LIST);
			this->dragLineObject->position(startPosition);
			this->dragLineObject->index(0);
			this->dragLineObject->position(endPosition);
			this->dragLineObject->index(1);
			this->dragLineObject->end();
		};
		Ogre::String id = "GameObjectPicker_drawLine_" + camera->getName();
		NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction, false);
	}

	void GameObjectPicker::destroyLine()
	{
		if (this->dragLineNode == nullptr)
		{
			return;
		}

		this->destroyingLine.store(true, std::memory_order_release);

		Ogre::String id = "GameObjectPicker_drawLine_" + camera->getName();
		NOWA::GraphicsModule::getInstance()->removeTrackedClosure(id);

		auto localSceneManager = this->sceneManager;
		auto localDragLineNode = this->dragLineNode;
		auto localDragLineObject = this->dragLineObject;

		ENQUEUE_RENDER_COMMAND_MULTI_WAIT("GameObjectPicker::destroyLine", _3(localSceneManager, localDragLineNode, localDragLineObject),
		{
			localDragLineObject->clear();
			localDragLineNode->detachAllObjects();
			if (localDragLineObject)
			{
				localSceneManager->destroyManualObject(localDragLineObject);
			}
			if (localDragLineNode->getParentSceneNode())
			{
				localDragLineNode->getParentSceneNode()->removeAndDestroyChild(localDragLineNode);
			}
		});

		this->dragLineObject = nullptr;
		this->dragLineNode = nullptr;
	}

	void GameObjectPicker::deleteJointDelegate(EventDataPtr eventData)
	{
		boost::shared_ptr<EventDataDeleteJoint> castEventData = boost::static_pointer_cast<EventDataDeleteJoint>(eventData);

		unsigned long id = castEventData->getJointId();
		if (id == this->jointId)
		{
			this->jointId = 0;
			this->release(true);
		}
	}

	void GameObjectPicker::deleteBodyDelegate(EventDataPtr eventData)
	{
		boost::shared_ptr<EventDataDeleteBody> castEventData = boost::static_pointer_cast<EventDataDeleteBody>(eventData);

		if (castEventData->getBody() == this->hitBody)
		{
			this->release(true);
		}
	}

	void GameObjectPicker::updateVisual()
	{
		if (!this->drawLines || !this->hitBody)
			return;

		Ogre::Ray mouseRay = this->getRayFromMouse();
		Ogre::Vector3 cursorPos = mouseRay.getPoint(this->getDragDistance());

		Ogre::Quaternion q;
		Ogre::Vector3 p;
		this->hitBody->getPositionOrientation(p, q);
		Ogre::Vector3 dragPos = (q * this->dragPoint) + p;

		this->drawLine(cursorPos, dragPos);
	}

	void GameObjectPicker::grabGameObject(OgreNewt::World* ogreNewt, const Ogre::Vector2 position, Ogre::Window* renderWindow, Ogre::Real pickForce, Ogre::Real dragAffectDistance, const Ogre::Vector3& offsetPosition)
	{
		this->mousePosition = position;
		this->renderWindow = renderWindow;
		this->pickForce = pickForce;
		this->dragAffectDistance = dragAffectDistance;
		this->offsetPosition = offsetPosition;

		if (this->pickForce < 1.0f)
		{
			this->pickForce = 1.0f;
		}
		else if (this->pickForce > 50.0f)
		{
			this->pickForce = 50.0f;
		}

		if (false == active || nullptr == this->sceneManager)
		{
			return;
		}

		this->updateVisual();

		if (false == this->dragging)
		{
			// Get ray origin and end coordinates
			Ogre::Ray mouseRay = getRayFromMouse();

			// Found a body in the ray's path
			if (nullptr != this->hitBody)
			{
				// Calculate hit global and local position (info.mDistance is in the range [0,1])
				Ogre::Plane plGround = Ogre::Plane(camera->getDerivedDirection(), -(this->hitBody->getPosition() + this->offsetPosition) * camera->getDerivedDirection());

				std::pair<bool, Ogre::Real> intersectionDistance = mouseRay.intersects(plGround);

				Ogre::Real length = 0.0f;

				if (intersectionDistance.first)
				{
					length = intersectionDistance.second;
				}

				Ogre::Vector3 globalPos = Ogre::Vector3::ZERO;

				if (Ogre::Math::RealEqual(this->pickForce, 50.0f))
				{
					this->hitBody->setPositionOrientation(globalPos, this->hitBody->getOrientation());
				}
				else
				{
					Ogre::Quaternion bodyOrientation;

					// Store the body position and orientation
					this->hitBody->getPositionOrientation(globalPos, bodyOrientation);
				}

				// Try to cast to physics component
				this->dragComponent = OgreNewt::any_cast<PhysicsComponent*>(this->hitBody->getUserData());

				if (nullptr != this->dragComponent)
				{
					// if this is a physics component, try to cast to an active one and subscribe, to be called in the force and torque callback of the active component
					// to integrate its force there without disrupting other forces
					PhysicsActiveComponent* physicsActiveComponent = dynamic_cast<PhysicsActiveComponent*>(this->dragComponent);
					if (nullptr != physicsActiveComponent)
					{
						if (this->drawLines)
						{
							this->createLine();
						}
						GameObjectPicker::PickForceObserver* pickForceObserver = new GameObjectPicker::PickForceObserver();
						pickForceObserver->setPicker(this);
						pickForceObserver->setName(this->dragComponent->getOwner()->getName() + "_GameObjectPicker" + Ogre::StringConverter::toString(this->dragComponent->getIndex()));
						physicsActiveComponent->attachForceObserver(pickForceObserver);
					}
				}

				Ogre::Vector3 currentMousePoint = mouseRay.getOrigin(); /*mouseRay.getPoint(length);*/
				this->dragDistance = ((globalPos + this->offsetPosition) - currentMousePoint).length();

				// this->dragPoint = localPos;
				this->dragPoint = Ogre::Vector3::ZERO;

				this->dragging = true;
			}
		}
		else
		{
			if (true == this->drawLines)
			{
				this->drawLine(this->cursorPos, this->dragPos);
			}
		}
	}

	void GameObjectPicker::release(bool resetBody)
	{
		if (this->hitBody)
		{
			if (nullptr != this->dragComponent)
			{
				PhysicsActiveComponent* physicsActiveComponent = dynamic_cast<PhysicsActiveComponent*>(this->dragComponent);
				if (nullptr != physicsActiveComponent)
				{
					physicsActiveComponent->detachAndDestroyForceObserver(this->dragComponent->getOwner()->getName() + "_GameObjectPicker" + Ogre::StringConverter::toString(this->dragComponent->getIndex()));
				}
			}
		}

		if (this->getDrawLine())
		{
			this->destroyLine();
		}
		// Reinitialise dragging information
		this->dragComponent = nullptr;
		if (0 == this->gameObjectId && 0 == this->jointId && true == resetBody)
		{
			this->hitBody = nullptr;
		}
		this->dragPoint = Ogre::Vector3::ZERO;
		this->dragDistance = 0.0f;
		this->dragging = false;
	}

	void GameObjectPicker::updateQueryMask(unsigned int newQueryMask)
	{
		this->queryMask = newQueryMask;
	}

	Ogre::Real GameObjectPicker::getPickForce(void) const
	{
		return this->pickForce;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	KinematicPicker::KinematicPicker()
		: sceneManager(nullptr),
		camera(nullptr),
		renderWindow(nullptr),
		dragNormal(Ogre::Vector3::ZERO),
		dragPoint(Ogre::Vector3::ZERO),
		dragDistance(1.0f),
		maxDistance(100.0f),
		queryMask(0 << 0),
		hitBody(nullptr),
		mousePosition(Ogre::Vector2::ZERO),
		dragComponent(nullptr),
		kinematicController(nullptr),
		active(false),
		dragging(false)
	{
	}

	KinematicPicker::~KinematicPicker()
	{
	}

	void KinematicPicker::init(Ogre::SceneManager* sceneManager, Ogre::Camera* camera, Ogre::Real maxDistance, unsigned int queryMask)
	{
		this->sceneManager = sceneManager;
		this->camera = camera;
		this->maxDistance = maxDistance;
		this->queryMask = queryMask;

		this->active = true;
	}

	PhysicsComponent* KinematicPicker::grab(OgreNewt::World* ogreNewt, const Ogre::Vector2 position, Ogre::Window* renderWindow)
	{
		this->mousePosition = position;
		this->renderWindow = renderWindow;
		
		if (false == this->active || nullptr == this->sceneManager)
		{
			return nullptr;
		}

		// Get ray origin and end coordinates
		Ogre::Ray mouseRay = getRayFromMouse();

		if (false == this->dragging)
		{
			// Cast a ray between these points and check for first hit
			OgreNewt::BasicRaycast* ray = new OgreNewt::BasicRaycast(ogreNewt, mouseRay.getOrigin(), mouseRay.getPoint(this->maxDistance), true);
			OgreNewt::BasicRaycast::BasicRaycastInfo info = ray->getFirstHit();

			// Found a body in the ray's path
			if (nullptr != info.mBody)
			{
				this->dragDistance = this->maxDistance * info.mDistance;
				this->dragNormal = info.mNormal;
				
				// check its query mask
				unsigned int type = info.mBody->getType();
				unsigned int finalType = type & this->queryMask;
				if (type == finalType)
				{
					this->hitBody = info.mBody;
					this->dragNormal = info.mNormal;

					Ogre::Vector3 bodyPos;
					Ogre::Quaternion bodyOrientation;

					// Store the body position and orientation
					this->hitBody->getPositionOrientation(bodyPos, bodyOrientation);

					// Try to cast to physics component
					this->dragComponent = OgreNewt::any_cast<PhysicsComponent*>(this->hitBody->getUserData());

					if (nullptr != this->dragComponent)
					{
						if (nullptr != this->kinematicController)
						{
							delete this->kinematicController;
							this->kinematicController = nullptr;
						}

						Ogre::Vector3 gravity = this->hitBody->getGravity();
						if (Ogre::Vector3::ZERO == gravity)
						{
							gravity = Ogre::Vector3(0.0f, -19.8f, 0.0f);
						}

						// Change this to make the grabbing stronger or weaker
				        //const dFloat angularFritionAccel = 10.0f;
						const Ogre::Real angularFrictionAccel = 5.0f;
						const Ogre::Real linearFrictionAccel = 400.0f * gravity.y;

						Ogre::Vector3 hitBodyInertia = this->hitBody->getInertia();

						const Ogre::Real inertia = std::max(hitBodyInertia.z, std::max(hitBodyInertia.x, hitBodyInertia.y));

						this->kinematicController = new OgreNewt::KinematicController(AppStateManager::getSingletonPtr()->getOgreNewtModule()->getOgreNewt(), this->hitBody, bodyPos);
						this->kinematicController->setPickingMode(/*linearPlusAngularFriction*/ 4);

						this->kinematicController->setMaxLinearFriction(hitBody->getMass() * linearFrictionAccel);
						this->kinematicController->setMaxAngularFriction(inertia * angularFrictionAccel);
					}

					this->dragging = true;
				}

				delete ray;
			}
		}
		else
		{
			if (nullptr != this->kinematicController)
			{
				this->dragPoint = mouseRay.getPoint(this->dragDistance);
				this->kinematicController->setTargetPosit(this->dragPoint);
			}
		}

		return this->dragComponent;
	}

	void KinematicPicker::release(void)
	{
		this->dragComponent = nullptr;

		if (nullptr != this->kinematicController)
		{
			delete this->kinematicController;
			this->kinematicController = nullptr;
		}

		this->hitBody = nullptr;
		this->dragDistance = 0.0f;
		this->dragNormal = Ogre::Vector3::ZERO;
		this->dragging = false;
	}

	void KinematicPicker::updateQueryMask(unsigned int newQueryMask)
	{
		this->queryMask = newQueryMask;
	}

	Ogre::Ray KinematicPicker::getRayFromMouse(void) const
	{
		// Calculate normalised mouse position [0..1]
		Ogre::Real mx = static_cast<Ogre::Real>(this->mousePosition.x) / static_cast<Ogre::Real>(this->renderWindow->getWidth());
		Ogre::Real my = static_cast<Ogre::Real>(this->mousePosition.y) / static_cast<Ogre::Real>(this->renderWindow->getHeight());

		// Get the global position our cursor is at
		Ogre::Ray ray;
		ENQUEUE_GET_CAMERA_TO_VIEWPORT_RAY(this->camera, mx, my, ray);

		return ray;
	}

	Ogre::Real KinematicPicker::getDragDistance(void) const
	{
		return this->dragDistance;
	}

	Ogre::Vector3 KinematicPicker::getDragNormal(void) const
	{
		return this->dragNormal;
	}

	Ogre::Vector3 KinematicPicker::getDragPoint(void) const
	{
		return this->dragPoint;
	}

}; // namespace end
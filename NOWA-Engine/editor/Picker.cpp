#include "NOWAPrecompiled.h"
#include "Picker.h"
#include "gameobject/JointComponents.h"

namespace NOWA
{
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
		dragLineNode(nullptr),
		dragLineObject(nullptr),
		hasMoveCallback(false),
		oldForceTorqueCallback(nullptr)
	{

	}

	Picker::~Picker()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[Picker] Destroying picker");
		this->dragComponent = nullptr;
		if (this->active)
		{
			if (this->drawLines)
			{
				this->destroyLine();
			}
			this->active = false;
		}
		this->detachAndDestroyAllPickObserver();
	}

	void Picker::init(Ogre::SceneManager* sceneManager, Ogre::Camera* camera, Ogre::Real maxDistance, unsigned int queryMask, bool drawLines)
	{
		this->sceneManager = sceneManager;
		this->camera = camera;
		this->maxDistance = maxDistance;
		this->queryMask = queryMask;
		this->drawLines = drawLines;
		this->dragging = false;

		if (this->drawLines)
		{
			this->createLine();
		}

		this->active = true;
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
		return std::move(this->camera->getCameraToViewportRay(mx, my));
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
		this->dragLineNode = this->sceneManager->getRootSceneNode()->createChildSceneNode();
		// this->dragLineObject = new Ogre::v1::ManualObject(0, &this->sceneManager->_getEntityMemoryManager(Ogre::SCENE_DYNAMIC), this->sceneManager);
		this->dragLineObject = this->sceneManager->createManualObject();
		this->dragLineObject->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_MESH);
		this->dragLineObject->setName("PickerDragLines");
		this->dragLineObject->setQueryFlags(0 << 0);
		this->dragLineObject->setCastShadows(false);
		this->dragLineNode->attachObject(this->dragLineObject);
	}

	void Picker::drawLine(const Ogre::Vector3& startPosition, const Ogre::Vector3& endPosition)
	{
		if (this->dragLineNode == nullptr)
		{
			this->createLine();
		}
		// Draw a 3D line between these points for visual effect
		this->dragLineObject->clear();
		this->dragLineObject->begin("WhiteNoLightingBackground", Ogre::OperationType::OT_LINE_LIST);
		this->dragLineObject->position(startPosition);
		this->dragLineObject->index(0);
		this->dragLineObject->position(endPosition);
		this->dragLineObject->index(1);
		this->dragLineObject->end();
	}

	void Picker::destroyLine()
	{
		if (this->dragLineNode != nullptr)
		{
			this->dragLineNode->detachAllObjects();
			if (this->dragLineObject != nullptr)
			{
				this->sceneManager->destroyManualObject(this->dragLineObject);
				// delete this->dragLineObject;
				this->dragLineObject = nullptr;
			}
			this->dragLineNode->getParentSceneNode()->removeAndDestroyChild(this->dragLineNode);
			this->dragLineNode = nullptr;
		}
	}

	PhysicsComponent* Picker::grab(OgreNewt::World* ogreNewt, const Ogre::Vector2 position, Ogre::Window* renderWindow, Ogre::Real pickForce)
	{
		this->mousePosition = position;
		this->renderWindow = renderWindow;
		this->pickForce = pickForce;
		if (this->pickForce < 1.0f)
			this->pickForce = 1.0f;
		else if (this->pickForce > 50.0f)
			this->pickForce = 50.0f;

		if (!active || !this->sceneManager)
		{
			return nullptr;
		}

		if (!this->dragging)
		{
			// Get ray origin and end coordinates
			Ogre::Ray mouseRay = getRayFromMouse();

			// Cast a ray between these points and check for first hit
			OgreNewt::BasicRaycast* ray = new OgreNewt::BasicRaycast(ogreNewt, mouseRay.getOrigin(), mouseRay.getPoint(this->maxDistance), true);
			OgreNewt::BasicRaycast::BasicRaycastInfo info = ray->getFirstHit();

			// Found a body in the ray's path
			if (info.mBody)
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
							Picker::PickForceObserver* pickForceObserver = new Picker::PickForceObserver(this);
							pickForceObserver->setName(this->dragComponent->getOwner()->getName() + "_Picker");
							physicsActiveComponent->attachForceObserver(pickForceObserver);
						}
					}
					else
					{
						// If its no component, but just the body, apply the drag callback with standard functionality
						// Change the force callback from the standard one to the one that applies the spring picking force
						// Store the old force torque callback
						this->oldForceTorqueCallback = info.mBody->getForceTorqueCallback();

						info.mBody->setCustomForceAndTorqueCallback<Picker>(&Picker::dragCallback, this);
						this->hasMoveCallback = true;
					}

					this->dragDistance = (this->maxDistance * info.mDistance);
					this->dragPoint = localPos;

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

	void Picker::dragCallback(OgreNewt::Body* body, Ogre::Real timeStep, int threadIndex)
	{
		Ogre::Ray mouseRay = this->getRayFromMouse();
		// Get the global position our cursor is at
		Ogre::Vector3 cursorPos = mouseRay.getPoint(this->dragDistance);

		Ogre::Quaternion bodyOrientation;
		Ogre::Vector3 bodyPos;

		// Now find the global point on the body
		body->getPositionOrientation(bodyPos, bodyOrientation);

		// Find the handle position we are holding the body from
		Ogre::Vector3 dragPos = (bodyOrientation * dragPoint) + bodyPos;

		Ogre::Vector3 inertia;
		Ogre::Real mass;

		body->getMassMatrix(mass, inertia);

		// Calculate picking spring force
		Ogre::Vector3 dragForce = Ogre::Vector3::ZERO;

		if (Ogre::Math::RealEqual(this->pickForce, 50.0f))
		{
			body->setPositionOrientation(cursorPos, Ogre::Quaternion::IDENTITY);
			// Annulate gravity
			body->addForce(body->getGravity() * mass);
			this->destroyLine();
		}
		else
		{
			Ogre::Vector3 dragForce = ((cursorPos - dragPos) * mass * (static_cast<Ogre::Real>(this->pickForce)))/* - body->getVelocity()*/;
			if (this->drawLines)
			{
				this->drawLine(cursorPos, dragPos);
			}

			// body->addForce(body->getGravity() * mass);
			// Add the picking spring force at the handle
			body->addGlobalForce(dragForce, dragPos);
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
					physicsActiveComponent->detachAndDestroyForceObserver(this->dragComponent->getOwner()->getName() + "_Picker");
				}
			}
		}

		if (this->getDrawLine())
		{
			this->destroyLine();
		}
		// Reinitialise dragging information
		this->dragComponent = nullptr;
		if (true == this->hasMoveCallback)
		{
			// this->hitBody->removeForceAndTorqueCallback();
			this->hitBody->setCustomForceAndTorqueCallback(this->oldForceTorqueCallback);
			this->hasMoveCallback = false;
		}
		this->hitBody = nullptr;
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

						this->kinematicController = new OgreNewt::KinematicController(this->hitBody, bodyPos);
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
		return std::move(this->camera->getCameraToViewportRay(mx, my));
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
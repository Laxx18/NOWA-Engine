#include "NOWAPrecompiled.h"
#include "CameraComponent.h"
#include "GameObjectController.h"
#include "WorkspaceComponents.h"
#include "utilities/XMLConverter.h"
#include "modules/WorkspaceModule.h"
#include "camera/CameraManager.h"
#include "utilities/MathHelper.h"
#include "main/AppStateManager.h"
#include "main/Core.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	bool CameraComponent::justCreated = false;

	CameraComponent::CameraComponent()
		: GameObjectComponent(),
		camera(nullptr),
		dummyItem(nullptr),
		baseCamera(nullptr),
		timeSinceLastUpdate(0.0f),
		workspaceBaseComponent(nullptr),
		eyeId(-1),
		active(new Variant(CameraComponent::AttrActive(), false, this->attributes)),
        position(new Variant(CameraComponent::AttrPosition(), Ogre::Vector3(0.0f, 5.0f, -2.0f), this->attributes)),
		orientation(new Variant(CameraComponent::AttrOrientation(), Ogre::Vector3::ZERO, this->attributes)),
		nearClipDistance(new Variant(CameraComponent::AttrNearClipDistance(), 0.1f, this->attributes)),
		farClipDistance(new Variant(CameraComponent::AttrFarClipDistance(), 500.0f, this->attributes)),
		fovy(new Variant(CameraComponent::AttrFovy(), 90.0f, this->attributes)),
		orthographic(new Variant(CameraComponent::AttrOrthographic(), false, this->attributes)),
		orthoWindowSize(new Variant(CameraComponent::AttrOrthoWindowSize(), Ogre::Vector2(10.0f, 10.0f), this->attributes)),
		fixedYawAxis(new Variant(CameraComponent::AttrFixedYawAxis(), true, this->attributes)),
		showDummyEntity(new Variant(CameraComponent::AttrShowDummyItem(), false, this->attributes))
	{
		this->orthographic->addUserData(GameObject::AttrActionNeedRefresh());
		this->orthoWindowSize->setVisible(false);

		this->active->setDescription("In order to change the workspace or sky box or color, first deactivate and then activate the camera again.");
		this->fovy->setDescription("Field Of View (FOV) is the angle made between the frustum's position, and the edges "
			"of the 'screen' onto which the scene is projected.High values(90 + degrees) result in a wide - angle, "
			"fish - eye kind of view, low values(30 - degrees) in a stretched, telescopic kind of view.Typical values "
			"are between 45 and 60 degrees.");

		this->fixedYawAxis->setDescription("Tells the camera whether to yaw around it's own local Y axis or a "
			"fixed axis of choice. This method allows you to change the yaw behaviour of the camera "
			"- by default, the camera yaws around a fixed Y axis.This is "
			"often what you want - for example if you're making a first-person "
			"shooter, you really don't want the yaw axis to reflect the local "
			"camera Y, because this would mean a different yaw axis if the "
			"player is looking upwards rather than when they are looking "
			"straight ahead.You can change this behaviour by calling this "
			"method, which you will want to do if you are making a completely "
			"free camera like the kind used in a flight simulator. ");

		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &CameraComponent::handleSwitchCamera), EventDataSwitchCamera::getStaticEventType());
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &CameraComponent::handleRemoveCamera), EventDataRemoveCamera::getStaticEventType());
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &CameraComponent::handleRemoveCameraBehavior), EventDataRemoveCameraBehavior::getStaticEventType());
	}

	CameraComponent::~CameraComponent()
	{
		
	}

	void CameraComponent::handleSwitchCamera(EventDataPtr eventData)
	{
		boost::shared_ptr<EventDataSwitchCamera> castEventData = boost::static_pointer_cast<EventDataSwitchCamera>(eventData);
		unsigned long id = std::get<0>(castEventData->getCameraGameObjectData());
		unsigned int index = std::get<1>(castEventData->getCameraGameObjectData());
		bool active = std::get<2>(castEventData->getCameraGameObjectData());

		// if a camera has been set as active, go through all game objects and set all camera components as active false
		if (true == active)
		{
			auto gameObjects = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjects();
			for (auto it = gameObjects->begin(); it != gameObjects->end(); ++it)
			{
				GameObject* gameObject = it->second.get();
				if (id != gameObject->getId())
				{
					auto cameraComponent = NOWA::makeStrongPtr(gameObject->getComponent<CameraComponent>());
					if (nullptr != cameraComponent)
					{
						// Do not call: setActivated(false), because internally this event is sent, so a event flooding would occur!
						cameraComponent->setActivatedFlag(false);
					}
				}
			}
		}
	}

	void CameraComponent::handleRemoveCamera(EventDataPtr eventData)
	{
		boost::shared_ptr<EventDataRemoveCamera> castEventData = boost::static_pointer_cast<EventDataRemoveCamera>(eventData);
		bool cameraWasActive = castEventData->getCameraWasActive();
		
		if (true == cameraWasActive)
		{
			auto gameObjects = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjects();
			// if the removed camera component had an active camera a successeres must be determined
			for (auto it = gameObjects->begin(); it != gameObjects->end(); ++it)
			{
				GameObject* gameObject = it->second.get();

				auto cameraComponent = NOWA::makeStrongPtr(gameObject->getComponent<CameraComponent>());
				if (nullptr != cameraComponent)
				{
					// Do not call: setActivated(true), because internally this event is sent, so a event flooding would occur!
					cameraComponent->setActivatedFlag(true);
					break;
				}
			}
		}
	}

	void CameraComponent::handleRemoveCameraBehavior(EventDataPtr eventData)
	{
		boost::shared_ptr<EventDataRemoveCameraBehavior> castEventData = boost::static_pointer_cast<EventDataRemoveCameraBehavior>(eventData);

		// If camera has been removed by the CameraManager, then its behavior is also have been deleted, so reset the pointer
		if (this->camera == castEventData->getCamera())
		{
			this->baseCamera = nullptr;
		}
	}

	bool CameraComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Active")
		{
			// this->setActivated(XMLConverter::getAttribBool(propertyElement, "data", false)); // Commented out, because else workspace is created to early
			this->active->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CameraPosition")
		{
			// ONLY set the variant during init; don't touch Ogre yet
			this->position->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CameraOrientation")
		{
			this->orientation->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "NearClipDistance")
		{
			this->setNearClipDistance(XMLConverter::getAttribReal(propertyElement, "data", 0.01f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FarClipDistance")
		{
			this->setFarClipDistance(XMLConverter::getAttribReal(propertyElement, "data", 100.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Fovy")
		{
			this->setFovy(Ogre::Degree(XMLConverter::getAttribReal(propertyElement, "data", 90.0f)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Orthographic")
		{
			this->setOrthographic(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OrthoWindowSize")
		{
			this->setOrthoWindowSize(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FixedYawAxis")
		{
			this->fixedYawAxis->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ShowDummyEntity")
		{
			this->showDummyEntity->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		
		return true;
	}

	GameObjectCompPtr CameraComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		CameraCompPtr clonedCompPtr(boost::make_shared<CameraComponent>());
		
		clonedCompPtr->setActivated(this->active->getBool());

		clonedCompPtr->setCameraPosition(this->position->getVector3());
		clonedCompPtr->setCameraDegreeOrientation(this->orientation->getVector3());
		clonedCompPtr->setNearClipDistance(this->nearClipDistance->getReal());
		clonedCompPtr->setFarClipDistance(this->farClipDistance->getReal());
		clonedCompPtr->setFovy(Ogre::Degree(this->fovy->getReal()));
		clonedCompPtr->setOrthographic(this->orthographic->getBool());
		clonedCompPtr->setOrthoWindowSize(this->orthoWindowSize->getVector2());
		clonedCompPtr->setFixedYawAxis(this->fixedYawAxis->getBool());
		
		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		clonedCompPtr->setShowDummyEntity(this->showDummyEntity->getBool());

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool CameraComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CameraComponent] Init camera component for game object: " + this->gameObjectPtr->getName());

		// Hide transform values for game object, since it is controlled here via camera component
		this->gameObjectPtr->getAttribute(GameObject::AttrPosition())->setVisible(false);
		this->gameObjectPtr->getAttribute(GameObject::AttrOrientation())->setVisible(false);
		this->gameObjectPtr->getAttribute(GameObject::AttrScale())->setVisible(false);

		this->gameObjectPtr->setDynamic(false);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);

		this->createCamera();

		return true;
	}

	bool CameraComponent::connect(void)
	{
		GameObjectComponent::connect();

		if (nullptr != this->dummyItem)
		{
            auto item = this->dummyItem;
			bool visible = this->showDummyEntity->getBool() && this->gameObjectPtr->isVisible();
            NOWA::GraphicsModule::RenderCommand renderCommand = [this, item, visible]
            {
                item->setVisible(visible);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "CameraComponent::connect");
		}

		return true;
	}

	bool CameraComponent::disconnect(void)
	{
		GameObjectComponent::disconnect();

		if (nullptr != this->dummyItem)
		{
			Ogre::String name = this->camera->getName();
			if (this->camera == AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera() || this->gameObjectPtr->getId() == GameObjectController::MAIN_CAMERA_ID)
			{
				GraphicsModule::RenderCommand renderCommand = [this]()
                {
                    if (this->dummyItem)
                    {
                        this->dummyItem->setVisible(false);
                    }
                };
                NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "CameraComponent::disconnect1");
			}
			else
			{
                bool visible = this->gameObjectPtr->isVisible();
                NOWA::GraphicsModule::RenderCommand renderCommand = [this, visible]
                {
                    this->dummyItem->setVisible(visible);
                };
                NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "CameraComponent::disconnect2");
			}
		}

		return true;
	}

	void CameraComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();

		// Remove event listeners immediately (assumed thread-safe)
		auto eventManager = NOWA::AppStateManager::getSingletonPtr()->getEventManager();
		eventManager->removeListener(fastdelegate::MakeDelegate(this, &CameraComponent::handleSwitchCamera), EventDataSwitchCamera::getStaticEventType());
		eventManager->removeListener(fastdelegate::MakeDelegate(this, &CameraComponent::handleRemoveCamera), EventDataRemoveCamera::getStaticEventType());
		eventManager->removeListener(fastdelegate::MakeDelegate(this, &CameraComponent::handleRemoveCameraBehavior), EventDataRemoveCameraBehavior::getStaticEventType());

		NOWA::GraphicsModule::getInstance()->removeTrackedCamera(this->camera);

		// Copy pointers for deferred destruction
		auto cameraCopy = this->camera;
		auto dummyItemCopy = this->dummyItem;
		auto gameObjectCopy = this->gameObjectPtr;
		auto sceneManagerCopy = (gameObjectCopy) ? gameObjectCopy->getSceneManager() : nullptr;
		auto cameraManager = NOWA::AppStateManager::getSingletonPtr()->getCameraManager();
		auto workspaceBaseComponentCopy = this->workspaceBaseComponent;
		auto active = this->active->getBool();

		// Nullify members immediately
		this->camera = nullptr;
		this->dummyItem = nullptr;
		this->gameObjectPtr = nullptr;

		// Enqueue destruction command on render thread
		NOWA::GraphicsModule::RenderCommand renderCommand = [this, cameraCopy, dummyItemCopy, sceneManagerCopy, cameraManager, gameObjectCopy, workspaceBaseComponentCopy, active]()
		{
			if (cameraCopy)
			{
				// Show transform values for game object when camera component has been removed
				gameObjectCopy->getAttribute(GameObject::AttrPosition())->setVisible(true);
				gameObjectCopy->getAttribute(GameObject::AttrOrientation())->setVisible(true);
				gameObjectCopy->getAttribute(GameObject::AttrScale())->setVisible(true);

				// If it was an active one, send event
				if (true == active && false == AppStateManager::getSingletonPtr()->bShutdown)
				{
					boost::shared_ptr<EventDataRemoveCamera> eventDataRemoveCamera(new EventDataRemoveCamera(active, cameraCopy));
					NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataRemoveCamera);
				}

				if (nullptr != workspaceBaseComponentCopy && false == AppStateManager::getSingletonPtr()->bShutdown)
				{
					workspaceBaseComponentCopy->setUseReflection(false);
				}

				WorkspaceModule::getInstance()->removeCamera(cameraCopy);
			
				if (cameraManager)
					cameraManager->removeCamera(cameraCopy);

				if (cameraCopy->getParentSceneNode())
					cameraCopy->getParentSceneNode()->detachObject(cameraCopy);

				if (sceneManagerCopy)
					sceneManagerCopy->destroyMovableObject(cameraCopy);
			}
		};
		NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "CameraComponent::onRemoveComponent");
	}

	void CameraComponent::update(Ogre::Real dt, bool notSimulating)
    {
        if (!notSimulating)
        {
            return; // do nothing while simulating
        }

        if (this->timeSinceLastUpdate > 0.0f)
        {
            this->timeSinceLastUpdate -= dt;
            return;
        }

        if (!this->gameObjectPtr)
        {
            return;
        }
        if (!this->camera)
        {
            return;
        }

        // ---------------------------------------------------------------------
        // ACTIVE camera (the current editor viewport camera, or in-game the
        // currently rendering one): it is being driven directly by its own
        // camera behavior (BaseCamera free-fly navigation, FirstPersonCamera,
        // orbit cam, etc.) via camera->setPosition()/setOrientation() calls
        // that never touch the GameObject's scene node at all. Forcing the
        // camera to follow the (static, unrelated) node here would fight that
        // behavior every debounce tick, snapping the camera back mid-navigation
        // - this is what caused the gizmo/viewport jitter. So for the active
        // camera we ONLY read its current world transform into the Variants,
        // exactly like before this sync logic existed; we never write back to
        // either the camera or the node.
        // ---------------------------------------------------------------------
        if (true == this->active->getBool())
        {
            Ogre::Vector3 worldPos = this->camera->getDerivedPosition();
            Ogre::Quaternion worldOri = this->camera->getDerivedOrientation();

            if (!MathHelper::getInstance()->vector3Equals(this->position->getVector3(), worldPos, 0.001f))
            {
                this->position->setValue(worldPos);
            }

            const Ogre::Vector3 worldDeg = MathHelper::getInstance()->quatToDegrees(worldOri);
            if (!MathHelper::getInstance()->vector3Equals(this->orientation->getVector3(), worldDeg, 0.001f))
            {
                this->orientation->setValue(worldDeg);
            }

            this->timeSinceLastUpdate = 0.05f;
            return;
        }

        // -----------------------------------------------------------------
        // INACTIVE camera: nothing drives this camera's own transform except
        // the gizmo, which moves the GameObject's scene node (the same node
        // the dummy item is attached to). The camera itself stays on its own
        // separate default node (see createCamera() - required so existing
        // camera behaviors keep working in world space), so it never follows
        // the node automatically. Push the node's current transform onto the
        // camera here so a gizmo drag actually moves the real Ogre::Camera,
        // not just the visible dummy mesh.
        // -----------------------------------------------------------------
        Ogre::Vector3 nodePos = this->gameObjectPtr->getSceneNode()->_getDerivedPositionUpdated();
        Ogre::Quaternion nodeOri = this->gameObjectPtr->getSceneNode()->_getDerivedOrientationUpdated();

        bool posDiffers = !MathHelper::getInstance()->vector3Equals(this->camera->getPosition(), nodePos, 0.001f);
        bool oriDiffers = !this->camera->getOrientation().equals(nodeOri, Ogre::Radian(0.001f));

        if (true == posDiffers || true == oriDiffers)
        {
            NOWA::GraphicsModule::RenderCommand renderCommand = [this, nodePos, nodeOri]()
            {
                this->camera->setPosition(nodePos);
                this->camera->setOrientation(nodeOri);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "CameraComponent::update_syncCameraToNode");
        }

        Ogre::Vector3 worldPos = this->camera->getDerivedPosition();
        Ogre::Quaternion worldOri = this->camera->getDerivedOrientation();

        if (!MathHelper::getInstance()->vector3Equals(this->position->getVector3(), worldPos, 0.001f))
        {
            this->position->setValue(worldPos);
        }

        const Ogre::Vector3 worldDeg = MathHelper::getInstance()->quatToDegrees(worldOri);
        if (!MathHelper::getInstance()->vector3Equals(this->orientation->getVector3(), worldDeg, 0.001f))
        {
            this->orientation->setValue(worldDeg);
        }

        this->timeSinceLastUpdate = 0.05f;
    }

	void CameraComponent::createCamera(void)
    {
        if (nullptr == this->camera)
        {
            bool applyStoredTransformToNode = CameraComponent::justCreated;

            NOWA::GraphicsModule::RenderCommand renderCommand = [this, applyStoredTransformToNode]()
            {
                this->camera = this->gameObjectPtr->getSceneManager()->createCamera(this->gameObjectPtr->getName());

                this->camera->setFixedYawAxis(this->fixedYawAxis->getBool());
                this->camera->setFOVy(Ogre::Degree(this->fovy->getReal()));
                this->camera->setNearClipDistance(this->nearClipDistance->getReal());
                this->camera->setFarClipDistance(this->farClipDistance->getReal());
                this->camera->setQueryFlags(0 << 0);
                this->setOrthographic(this->orthographic->getBool());

                Ogre::Real windowWidth = Core::getSingletonPtr()->getOgreRenderWindow()->getWidth();
                Ogre::Real windowHeight = Core::getSingletonPtr()->getOgreRenderWindow()->getHeight();
                Ogre::Real aspectRatio = windowWidth / windowHeight;
                this->camera->setAspectRatio(aspectRatio);

                // IMPORTANT: the camera is intentionally kept on its OWN
                // Ogre-Next auto-created default node (sitting at world
                // identity) rather than being attached to the GameObject's
                // node. All existing camera behaviors (FirstPersonCamera etc.)
                // call camera->setPosition()/setOrientation() directly and
                // treat those values as WORLD space - that only holds true
                // because the camera's own node never moves. Re-parenting the
                // camera to the GameObject node would turn every one of those
                // calls into a LOCAL offset instead and break every behavior.
                //
                // The GameObject's node is only used here, once, to give a
                // freshly-placed camera its initial world transform. Ongoing
                // synchronization between "gizmo moved the node" and "the
                // actual unparented camera" happens continuously in update()
                // (design-mode tick) - see there for why a one-shot alignment
                // here is not enough on its own.
                //
                // Only push our stored position/orientation onto the node when
                // the component was freshly added in the editor (no meaningful
                // node transform exists yet) - never on the load path, where
                // DotSceneImportModule::processNode() already applied the
                // correct <node> transform before this ran; overwriting it here
                // with a stale CameraPosition/CameraOrientation XML value was
                // the original bug.
                if (true == applyStoredTransformToNode)
                {
                    this->gameObjectPtr->getSceneNode()->setPosition(this->position->getVector3());
                    this->gameObjectPtr->getSceneNode()->setOrientation(MathHelper::getInstance()->degreesToQuat(this->orientation->getVector3()));
                }

                const Ogre::Vector3 worldPos = this->gameObjectPtr->getSceneNode()->getPosition();
                const Ogre::Quaternion worldOri = this->gameObjectPtr->getSceneNode()->getOrientation();

                if (this->camera->getParentSceneNode())
                {
                    this->camera->setPosition(this->camera->getParentSceneNode()->convertWorldToLocalPositionUpdated(worldPos));
                    this->camera->setOrientation(this->camera->getParentSceneNode()->convertWorldToLocalOrientationUpdated(worldOri));
                }

                // Keep the Variants in sync with the node's actual transform right
                // away, instead of waiting for the next editor-idle update() tick.
                this->position->setValue(worldPos);
                this->orientation->setValue(MathHelper::getInstance()->quatToDegrees(worldOri));

                // Borrow the entity from the game object
                this->dummyItem = this->gameObjectPtr->getMovableObject<Ogre::Item>();
                if (nullptr != this->dummyItem)
                {
                    this->dummyItem->setName("DummyItem");
                    this->dummyItem->setCastShadows(false);
                }

                // Register camera
                if (this->gameObjectPtr->getId() == GameObjectController::MAIN_CAMERA_ID)
                {
                    Ogre::Camera* previousCamera = NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
                    if (nullptr != previousCamera)
                    {
                        NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->removeCamera(previousCamera);
                        NOWA::GraphicsModule::getInstance()->removeTrackedCamera(previousCamera);
                        this->gameObjectPtr->getSceneManager()->destroyCamera(previousCamera);
                    }

                    if (nullptr == this->baseCamera)
                    {
                        this->baseCamera = new NOWA::BaseCamera(NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->getCameraBehaviorId(), 10.0f, 1.0f, 0.01f);
                    }

                    AppStateManager::getSingletonPtr()->getCameraManager()->addCameraBehavior(this->camera, this->baseCamera);
                    AppStateManager::getSingletonPtr()->getCameraManager()->addCamera(this->camera, true);
                }

                this->setActivated(this->active->getBool());
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "CameraComponent::createCamera");

            CameraComponent::justCreated = false;
        }
    }

	void CameraComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (CameraComponent::AttrActive() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (CameraComponent::AttrPosition() == attribute->getName())
		{
			this->setCameraPosition(attribute->getVector3());
            if (this->gameObjectPtr)
            {
                this->gameObjectPtr->setAttributePosition(attribute->getVector3());
            }
		}
		else if (CameraComponent::AttrOrientation() == attribute->getName())
		{
			this->setCameraDegreeOrientation(attribute->getVector3());
            if (this->gameObjectPtr)
            {
                this->gameObjectPtr->setAttributeOrientation(MathHelper::getInstance()->degreesToQuat(attribute->getVector3()));
            }
		}
		else if (CameraComponent::AttrNearClipDistance() == attribute->getName())
		{
			this->setNearClipDistance(attribute->getReal());
		}
		else if (CameraComponent::AttrFarClipDistance() == attribute->getName())
		{
			this->setFarClipDistance(attribute->getReal());
		}
		else if (CameraComponent::AttrFovy() == attribute->getName())
		{
			this->setFovy(Ogre::Degree(attribute->getReal()));
		}
		else if (CameraComponent::AttrOrthographic() == attribute->getName())
		{
			this->setOrthographic(attribute->getBool());
		}
		else if (CameraComponent::AttrOrthoWindowSize() == attribute->getName())
		{
			this->setOrthoWindowSize(attribute->getVector2());
		}
		else if (CameraComponent::AttrFixedYawAxis() == attribute->getName())
		{
			this->setFixedYawAxis(attribute->getBool());
		}
		else if (CameraComponent::AttrShowDummyItem() == attribute->getName())
		{
			this->setShowDummyEntity(attribute->getBool());
		}
	}

	void CameraComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		GameObjectComponent::writeXML(propertiesXML, doc);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Active"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->active->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "CameraPosition"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->position->getVector3())));
		propertiesXML->append_node(propertyXML);

		// this->setCameraPosition(this->gameObjectPtr->getSceneNode()->_getDerivedPositionUpdated());

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "CameraOrientation"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->orientation->getVector3())));
		propertiesXML->append_node(propertyXML);

		// this->setCameraOrientation(this->gameObjectPtr->getSceneNode()->_getDerivedOrientationUpdated());

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "NearClipDistance"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->nearClipDistance->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "FarClipDistance"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->farClipDistance->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Fovy"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->fovy->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Orthographic"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->orthographic->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OrthoWindowSize"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->orthoWindowSize->getVector2())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "FixedYawAxis"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->fixedYawAxis->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ShowDummyEntity"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->showDummyEntity->getBool())));
		propertiesXML->append_node(propertyXML);
	}

	void CameraComponent::setActivated(bool activated)
    {
        Ogre::String name = this->camera->getName();

        if (true == Core::getSingletonPtr()->getIsSceneBeingDestroyed())
        {
            return;
        }

        this->active->setValue(activated);

        // -----------------------------------------------------------------
        // Dummy item visibility rules
        //
        // The dummy item is the placeholder mesh representing a camera's
        // physical body in the scene, so it can be seen/selected/positioned
        // like a regular GameObject.
        //
        // 1. ACTIVE camera (the one currently rendering the view) NEVER shows
        //    its own dummy - you can't look at your own camera body from
        //    inside it. Applies regardless of offline/simulation or
        //    showDummyEntity. The main camera is just a special case of this:
        //    since it is active by default, its dummy ends up always hidden
        //    without any extra check.
        //
        // 2. INACTIVE camera, OFFLINE mode (bConnected == false, editor/design
        //    time): dummy is ALWAYS visible, so the designer can see and
        //    place every camera in the level. showDummyEntity is ignored here
        //    - that flag is a gameplay-only concept.
        //
        // 3. INACTIVE camera, SIMULATION mode (bConnected == true): dummy
        //    visibility follows the designer-controlled showDummyEntity flag
        //    (e.g. to show/hide a security-camera mesh in-game).
        //
        // IMPORTANT: Switching the active camera affects TWO components: the
        // newly activated one AND the previously active one. The camera
        // manager must call setActivated() on BOTH so the now-inactive
        // camera's dummy gets re-evaluated (rule 2/3), not just left hidden
        // from when it used to be active.
        //
        // ALSO IMPORTANT: Toggling bConnected alone (entering/leaving
        // simulation) does NOT change 'activated' for any camera, but it DOES
        // change which branch (2 vs 3) applies for every inactive camera.
        // Whatever code flips bConnected (simulation start/stop) must
        // re-trigger this evaluation for all inactive cameras in the scene
        // (e.g. by calling setActivated(this->active->getBool()) again on
        // each of them), otherwise their dummy visibility goes stale.
        // -----------------------------------------------------------------
        NOWA::GraphicsModule::RenderCommand renderCommand = [this, activated]
        {
            this->dummyItem = this->gameObjectPtr->getMovableObject<Ogre::Item>();
            if (nullptr == this->dummyItem)
            {
                return;
            }

            bool dummyVisible = false;

            if (false == activated)
            {
                if (false == this->bConnected)
                {
                    // Rule 2: offline mode, inactive camera -> always show.
                    dummyVisible = true;
                }
                else
                {
                    // Rule 3: simulation mode, inactive camera -> designer choice.
                    dummyVisible = this->showDummyEntity->getBool();
                }
            }
            // else: Rule 1, activated == true -> dummyVisible stays false.

            dummyVisible = dummyVisible && this->gameObjectPtr->isVisible();

            this->dummyItem->setVisible(dummyVisible);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "CameraComponent::setActivated");

        if (true == this->active->getBool())
        {
            // Hide entity for active camera
            if (nullptr != this->camera)
            {
                // REMOVED: previously wrote this->position/this->orientation
                // back onto the GameObject's node here
                // (gameObjectPtr->setAttributePosition/setAttributeOrientation).
                // Now that the camera is rigidly attached to that node with a
                // zero local offset (see createCamera()), the node's transform
                // IS the camera's transform at all times - there is nothing to
                // "restore" or push back. Keeping this risked snapping the node
                // back to a stale, not-yet-debounced Variant value right after a
                // gizmo drag (this->timeSinceLastUpdate can be up to 0.05s old).

                auto workspaceBaseCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<WorkspaceBaseComponent>());
                if (nullptr != workspaceBaseCompPtr)
                {
                    this->baseCamera = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCameraBehavior(this->camera);
                    // ATTENTION: Since even there is another camera behavior, the base camera behavior would be used sometimes, hence this changes here
                    // So that only a base camera is created, if there no other behavior for this game object
                    if (nullptr == this->baseCamera)
                    {
                        this->baseCamera = new NOWA::BaseCamera(NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->getCameraBehaviorId());
                        AppStateManager::getSingletonPtr()->getCameraManager()->addCameraBehavior(this->camera, this->baseCamera);
                        AppStateManager::getSingletonPtr()->getCameraManager()->addCamera(this->camera, true);
                    }
                    /*else
                    {
                        AppStateManager::getSingletonPtr()->getCameraManager()->activateCamera(this->camera);
                    }*/

                    // Create and switch workspace -- must run BEFORE setPrimaryWorkspace below, so that
                    // workspaceBaseCompPtr->getWorkspace() (read inside setPrimaryWorkspace) returns the
                    // actual, just-created workspace instead of a stale/null one.
                    workspaceBaseCompPtr->createWorkspace();

                    WorkspaceModule::getInstance()->setPrimaryWorkspace(this->gameObjectPtr->getSceneManager(), this->camera, workspaceBaseCompPtr.get());
                }
                else
                {
                    this->active->setValue(false);
                    Ogre::String message = "[CameraComponent] Could not switch workspace, because this camera component has no corresponding workspace component! Affected game object: " + this->gameObjectPtr->getName();
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, message);
                    boost::shared_ptr<EventDataFeedback> eventDataFeedback(new EventDataFeedback(false, message));
                    NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataFeedback);
                    return;
                }
            }
        }
        else
        {
            bool stillActiveOne = false;
            auto gameObjects = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjects();

            for (auto it = gameObjects->begin(); it != gameObjects->end(); ++it)
            {
                GameObject* gameObject = it->second.get();

                auto cameraComponent = NOWA::makeStrongPtr(gameObject->getComponent<CameraComponent>());
                if (nullptr != cameraComponent)
                {
                    if (true == cameraComponent->isActivated())
                    {
                        stillActiveOne = true;
                        break;
                    }
                }
            }
            // If there is no camera active, at least this one must remain active
            if (false == stillActiveOne)
            {
                this->active->setValue(true);
            }

            if (false == this->active->getBool())
            { 
                WorkspaceModule::getInstance()->removeCamera(this->camera);
                AppStateManager::getSingletonPtr()->getCameraManager()->removeCamera(this->camera);
            }
        }

        if (nullptr != this->gameObjectPtr)
        {
            Ogre::String name = this->gameObjectPtr->getName();
            // Send out event, whether is camera has been activated or not, to camera manager and other camera components, to adapt their state
            boost::shared_ptr<EventDataSwitchCamera> eventDataSwitchCamera(new EventDataSwitchCamera(this->gameObjectPtr->getId(), this->gameObjectPtr->getIndexFromComponent(this), activated));
            NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataSwitchCamera);
        }
    }

	void CameraComponent::setActivatedFlag(bool activated)
	{
        Ogre::String name = this->gameObjectPtr->getName();

		this->active->setValue(activated);
		if (nullptr != this->camera)
		{
			NOWA::GraphicsModule::RenderCommand renderCommand = [this, activated]()
			{
                if (nullptr != this->dummyItem)
                {
                    if (true == this->bConnected)
                    {
                        this->dummyItem->setVisible(this->showDummyEntity->getBool());
                    }
                    else
                    {
                        this->dummyItem->setVisible(true);
                    }

                    if (this->camera == AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera())
                    {
                        this->dummyItem->setVisible(false);
                    }
                }

				if (false == activated)
				{
					// if (true == WorkspaceModule::getInstance()->hasMoreThanOneWorkspace())
					{
						auto workspaceBaseCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<WorkspaceBaseComponent>());
						if (nullptr != workspaceBaseCompPtr)
						{
							// Create and switch workspace
							workspaceBaseCompPtr->removeWorkspace();
						}
						WorkspaceModule::getInstance()->removeCamera(this->camera);
					}
					AppStateManager::getSingletonPtr()->getCameraManager()->removeCamera(this->camera);
				}
			};
			NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "CameraComponent::setActivatedFlag");
		}
	}

	Ogre::String CameraComponent::getClassName(void) const
	{
		return "CameraComponent";
	}

	Ogre::String CameraComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	bool CameraComponent::isActivated(void) const
	{
		return this->active->getBool();
	}

	void CameraComponent::applySplitScreen(bool useSplitScreen, int eyeId)
	{
		if (nullptr != this->camera && this->gameObjectPtr->getId() != GameObjectController::MAIN_CAMERA_ID)
		{
			if (true == useSplitScreen)
			{
				this->eyeId = eyeId;
			}
			else
			{
				this->eyeId = -1;
			}
		}
	}

	void CameraComponent::setCameraPosition(const Ogre::Vector3& position)
	{
		this->position->setValue(position);
		/*if (this->gameObjectPtr)
		{
			this->gameObjectPtr->setAttributePosition(position);
		}*/

		if (this->camera)
		{
			NOWA::GraphicsModule::getInstance()->setCameraPosition(this->camera, position);
		}
	}

	Ogre::Vector3 CameraComponent::getCameraPosition(void) const
	{
		this->position->setValue(this->gameObjectPtr->getSceneNode()->getPosition());
		return this->position->getVector3();
	}

	void CameraComponent::setCameraDegreeOrientation(const Ogre::Vector3& orientationDeg)
	{
		this->orientation->setValue(orientationDeg);
		/*if (this->gameObjectPtr)
		{
			this->gameObjectPtr->setAttributeOrientation(MathHelper::getInstance()->degreesToQuat(orientationDeg));
		}*/

		if (this->camera)
		{
			NOWA::GraphicsModule::getInstance()->setCameraOrientation(this->camera, MathHelper::getInstance()->degreesToQuat(orientationDeg));
		}
	}

	void CameraComponent::setCameraOrientation(const Ogre::Quaternion& orientation)
	{
		this->orientation->setValue(MathHelper::getInstance()->quatToDegrees(orientation));
		/*if (nullptr != this->gameObjectPtr)
		{
			this->gameObjectPtr->setAttributeOrientation(orientation);
		}*/
		if (this->camera)
		{
			NOWA::GraphicsModule::getInstance()->setCameraOrientation(this->camera, orientation);
		}
	}

	Ogre::Vector3 CameraComponent::getCameraDegreeOrientation(void) const
	{
		this->orientation->setValue(MathHelper::getInstance()->quatToDegreesRounded(this->gameObjectPtr->getSceneNode()->getOrientation()));
		return this->orientation->getVector3();
	}

	void CameraComponent::setNearClipDistance(Ogre::Real nearClipDistance)
	{
		this->nearClipDistance->setValue(nearClipDistance);
		if (nullptr != this->camera)
		{
			GraphicsModule::RenderCommand renderCommand = [this, nearClipDistance]()
            {
                this->camera->setNearClipDistance(nearClipDistance);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "CameraComponent::setNearClipDistance");
		}
	}

	Ogre::Real CameraComponent::getNearClipDistance(void) const
	{
		return this->nearClipDistance->getReal();
	}

	void CameraComponent::setFarClipDistance(Ogre::Real farClipDistance)
	{
		this->farClipDistance->setValue(farClipDistance);
		if (nullptr != this->camera)
		{
			GraphicsModule::RenderCommand renderCommand = [this, farClipDistance]()
            {
                this->camera->setFarClipDistance(farClipDistance);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "CameraComponent::setFarClipDistance");
		}
	}

	Ogre::Real CameraComponent::getFarClipDistance(void) const
	{
		return this->farClipDistance->getReal();
	}

	void CameraComponent::setFovy(const Ogre::Degree& fovy)
	{
		this->fovy->setValue(fovy.valueDegrees());
		if (nullptr != this->camera)
		{
			GraphicsModule::RenderCommand renderCommand = [this, fovy]()
            {
                this->camera->setFOVy(fovy);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "CameraComponent::setFovy");
		}
	}

	Ogre::Degree CameraComponent::getFovy(void) const
	{
		return Ogre::Degree(this->fovy->getReal());
	}

	void CameraComponent::setOrthographic(bool orthographic)
	{
		this->orthographic->setValue(orthographic);
		if (nullptr != this->camera)
		{
			this->orthoWindowSize->setVisible(this->orthographic->getBool());

			GraphicsModule::RenderCommand renderCommand = [this]()
            {
                this->camera->setProjectionType(static_cast<Ogre::ProjectionType>(this->orthographic->getBool() == true ? 0 : 1));
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "CameraComponent::setOrthographic");

			this->setOrthoWindowSize(this->orthoWindowSize->getVector2());
		}
	}

	bool CameraComponent::getIsOrthographic(void) const
	{
		return this->orthographic->getBool();
	}

	void CameraComponent::setOrthoWindowSize(const Ogre::Vector2& orthoWindowSize)
	{
		this->orthoWindowSize->setValue(orthoWindowSize);
		if (nullptr != this->camera)
		{
			if (true == this->orthographic->getBool())
			{
				GraphicsModule::RenderCommand renderCommand = [this, orthoWindowSize]()
                {
                    this->camera->setOrthoWindow(orthoWindowSize.x, orthoWindowSize.y);
                };
                NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "CameraComponent::setOrthoWindowSize");
			}
		}
	}

	Ogre::Vector2 CameraComponent::getOrthoWindowSize(void) const
	{
		return this->orthoWindowSize->getVector2();
	}

	void CameraComponent::setShowDummyEntity(bool showDummyEntity)
	{
		this->showDummyEntity->setValue(showDummyEntity);
	}

	bool CameraComponent::getShowDummyEntity(void) const
	{
		return 	this->showDummyEntity->getBool();
	}

	void CameraComponent::setAspectRatio(Ogre::Real aspectRatio)
	{
		if (nullptr != this->camera)
		{
			GraphicsModule::RenderCommand renderCommand = [this, aspectRatio]()
            {
                this->camera->setAspectRatio(aspectRatio);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "CameraComponent::setAspectRatio");
		}
	}

	Ogre::Camera* CameraComponent::getCamera(void) const
	{
		return this->camera;
	}

	Ogre::uint8 CameraComponent::getEyeId(void) const
	{
		return this->eyeId;
	}

	void CameraComponent::setJustCreated(bool justCreated)
	{
		CameraComponent::justCreated = justCreated;
	}

	void CameraComponent::setFixedYawAxis(bool fixedYawAxis)
	{
		this->fixedYawAxis->setValue(fixedYawAxis);
		if (nullptr != this->camera)
		{
			GraphicsModule::RenderCommand renderCommand = [this, fixedYawAxis]()
            {
                this->camera->setFixedYawAxis(fixedYawAxis);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "CameraComponent::setFixedYawAxis");
		}
	}

	bool CameraComponent::getFixedYawAxis(void) const
	{
		return this->fixedYawAxis->getBool();
	}

	std::optional<NOWA::GameObjectTypeDescriptor> CameraComponent::getStaticTypeDescriptor()
    {
        NOWA::GameObjectTypeDescriptor desc;
        desc.type = NOWA::CAMERA;
        desc.displayName = "Camera";
        desc.meshToDisplay = "Camera.mesh";
        desc.needsMeshItem = true;
        desc.autoComponents = {CameraComponent::getStaticClassName(), WorkspacePbsComponent::getStaticClassName()};
        desc.preComponentsCallback = []()
        {
            CameraComponent::setJustCreated(true);
        };
        desc.postComponentsCallback = []()
        {
            CameraComponent::setJustCreated(true);
        };
        return desc;
    }

}; // namespace end
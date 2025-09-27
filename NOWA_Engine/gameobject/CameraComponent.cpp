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
		dummyEntity(nullptr),
		baseCamera(nullptr),
		timeSinceLastUpdate(0.0f),
		workspaceBaseComponent(nullptr),
		eyeId(-1),
		active(new Variant(CameraComponent::AttrActive(), false, this->attributes)),
		position(new Variant(CameraComponent::AttrPosition(), Ogre::Vector3::ZERO, this->attributes)),
		orientation(new Variant(CameraComponent::AttrOrientation(), Ogre::Vector3::ZERO, this->attributes)),
		nearClipDistance(new Variant(CameraComponent::AttrNearClipDistance(), 0.1f, this->attributes)),
		farClipDistance(new Variant(CameraComponent::AttrFarClipDistance(), 500.0f, this->attributes)),
		fovy(new Variant(CameraComponent::AttrFovy(), 90.0f, this->attributes)),
		orthographic(new Variant(CameraComponent::AttrOrthographic(), false, this->attributes)),
		orthoWindowSize(new Variant(CameraComponent::AttrOrthoWindowSize(), Ogre::Vector2(10.0f, 10.0f), this->attributes)),
		fixedYawAxis(new Variant(CameraComponent::AttrFixedYawAxis(), true, this->attributes)),
		showDummyEntity(new Variant(CameraComponent::AttrShowDummyEntity(), false, this->attributes)),
		excludeRenderCategories(new Variant(CameraComponent::AttrExcludeRenderCategories(), Ogre::String("All"), this->attributes))
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

		this->excludeRenderCategories->setDescription("Sets the render categories, which shall be excluded from this camera rendering. By default nothing will be excluded. Possible is also: 'All-LeftCamera-Enemy'. "
			"For this case e.g. all render categories but the game object which do belong to the left camera and the enemy would not be rendered by this camera.");

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
			for (auto& it = gameObjects->begin(); it != gameObjects->end(); ++it)
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
			for (auto& it = gameObjects->begin(); it != gameObjects->end(); ++it)
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
			this->setCameraPosition(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CameraOrientation")
		{
			this->setCameraDegreeOrientation(XMLConverter::getAttribVector3(propertyElement, "data"));
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
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ExcludeRenderCategories")
		{
			this->excludeRenderCategories->setValue(XMLConverter::getAttrib(propertyElement, "data", ""));
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
		clonedCompPtr->setExcludeRenderCategories(this->excludeRenderCategories->getString());
		
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

		if (nullptr != this->dummyEntity)
		{
			bool visible = this->showDummyEntity->getBool();
			ENQUEUE_RENDER_COMMAND_MULTI("CameraComponent::connect", _1(visible),
			{
				if (this->dummyEntity)
					this->dummyEntity->setVisible(visible);
			});
		}

		return true;
	}

	bool CameraComponent::disconnect(void)
	{
		GameObjectComponent::disconnect();

		if (nullptr != this->dummyEntity)
		{
			Ogre::String name = this->camera->getName();
			if (this->camera == AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera() || this->gameObjectPtr->getId() == GameObjectController::MAIN_CAMERA_ID)
			{
				ENQUEUE_RENDER_COMMAND("CameraComponent::disconnect1",
				{
					if (this->dummyEntity)
						this->dummyEntity->setVisible(false);
				});
			}
			else
			{
				ENQUEUE_RENDER_COMMAND("CameraComponent::disconnect2",
				{
					if (this->dummyEntity)
						this->dummyEntity->setVisible(true);
				});
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
		auto dummyEntityCopy = this->dummyEntity;
		auto gameObjectCopy = this->gameObjectPtr;
		auto sceneManagerCopy = (gameObjectCopy) ? gameObjectCopy->getSceneManager() : nullptr;
		auto cameraManager = NOWA::AppStateManager::getSingletonPtr()->getCameraManager();
		auto workspaceBaseComponentCopy = this->workspaceBaseComponent;
		auto active = this->active->getBool();

		// Nullify members immediately
		this->camera = nullptr;
		this->dummyEntity = nullptr;
		this->gameObjectPtr = nullptr;

		// Enqueue destruction command on render thread
		NOWA::GraphicsModule::RenderCommand renderCommand = [this, cameraCopy, dummyEntityCopy, sceneManagerCopy, cameraManager, gameObjectCopy, workspaceBaseComponentCopy, active]()
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
					NOWA::AppStateManager::getSingletonPtr()->getEventManager()->threadSafeQueueEvent(eventDataRemoveCamera);
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
		if (false == notSimulating)
		{
			// Do NOT update component attributes during simulation.
			// During simulation, the camera's movement is driven by game logic (e.g., a process, physics, or another component).
			return;
		}

		// Update camera values
		if (this->timeSinceLastUpdate >= 0.0f)
		{
			this->timeSinceLastUpdate -= dt;
		}
		else
		{
			if (nullptr != this->gameObjectPtr)
			{
				// Ogre::Vector3 cameraPosition = this->camera->getDerivedPosition();
				Ogre::Vector3 cameraPosition = this->gameObjectPtr->getSceneNode()->_getDerivedPositionUpdated();

				if (false == MathHelper::getInstance()->vector3Equals(this->position->getVector3(), cameraPosition, 0.001f))
				{
					this->setCameraPosition(cameraPosition);
				}

				// Ogre::Quaternion cameraOrientation = this->camera->getDerivedOrientation();
				Ogre::Quaternion cameraOrientation = this->gameObjectPtr->getSceneNode()->_getDerivedOrientationUpdated();
				// this->gameObjectPtr->getSceneNode()->setOrientation(MathHelper::getInstance()->degreesToQuat(this->orientation->getVector3()));
				// 
				Ogre::Vector3 orientationDegree = MathHelper::getInstance()->quatToDegrees(cameraOrientation);

				if (false == MathHelper::getInstance()->vector3Equals(this->orientation->getVector3(), orientationDegree, 0.001f))
				{
					this->setCameraOrientation(cameraOrientation);
				}

				this->timeSinceLastUpdate = 2.0f;
			}
		}
	}

	void CameraComponent::createCamera(void)
	{
		if (nullptr == this->camera)
		{
			ENQUEUE_RENDER_COMMAND_WAIT("CameraComponent::createCamera",
			{
				this->camera = this->gameObjectPtr->getSceneManager()->createCamera(this->gameObjectPtr->getName());

				// As attribute? because it should be set to false, when several viewports are used
				// this->camera->setAutoAspectRatio(true);

				this->camera->setFixedYawAxis(this->fixedYawAxis->getBool());
				this->camera->setFOVy(Ogre::Degree(this->fovy->getReal()));
				this->camera->setNearClipDistance(this->nearClipDistance->getReal());
				this->camera->setFarClipDistance(this->farClipDistance->getReal());
				this->camera->setQueryFlags(0 << 0);
				this->setOrthographic(this->orthographic->getBool());

				// This is required, because when a camera is created via the editor, it must be placed where placenode has been when the user clicked the mouse button
				// But when a camera is loaded from scene, it must not have an orientation, else there are ugly side effects
				if (true == CameraComponent::justCreated)
				{
					// this->gameObjectPtr->getSceneNode()->setOrientation(MathHelper::getInstance()->degreesToQuat(this->orientation->getVector3()));
					this->position->setValue(this->gameObjectPtr->getSceneNode()->getPosition());
					this->orientation->setValue(MathHelper::getInstance()->quatToDegreesRounded(this->gameObjectPtr->getSceneNode()->getOrientation()));

					CameraComponent::justCreated = false;
				}

				this->gameObjectPtr->getSceneNode()->setPosition(this->position->getVector3());
				this->gameObjectPtr->getSceneNode()->setOrientation(MathHelper::getInstance()->degreesToQuat(this->orientation->getVector3()));

				Ogre::Vector3 position = this->position->getVector3();
				Ogre::Quaternion orientation = MathHelper::getInstance()->degreesToQuat(this->orientation->getVector3());

				this->camera->setPosition(this->camera->getParentSceneNode()->convertWorldToLocalPositionUpdated(position));
				this->camera->setOrientation(this->camera->getParentSceneNode()->convertWorldToLocalOrientationUpdated(orientation));

				// Borrow the entity from the game object
				this->dummyEntity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
				if (nullptr != this->dummyEntity)
				{
					this->dummyEntity->setCastShadows(false);
				}

				// Special treatment: A camera must be created at an early stage, because other game objects and components are relying on this
				// So if its the main camera add the camera to the camera manager to have public available
				if (this->gameObjectPtr->getId() == GameObjectController::MAIN_CAMERA_ID)
				{
					// Delete camera that existed, before a project was loaded
					Ogre::Camera* previousCamera = NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
					if (nullptr != previousCamera)
					{
						NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->removeCamera(previousCamera);
						NOWA::GraphicsModule::getInstance()->removeTrackedCamera(previousCamera);
						this->gameObjectPtr->getSceneManager()->destroyCamera(previousCamera);
						previousCamera = nullptr;
					}

					this->dummyEntity->setVisible(false);

					if (nullptr == this->baseCamera)
					{
						this->baseCamera = new NOWA::BaseCamera(NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->getCameraBehaviorId(), 10.0f, 1.0f, 0.01f);
					}
					AppStateManager::getSingletonPtr()->getCameraManager()->addCameraBehavior(this->camera, this->baseCamera);
					AppStateManager::getSingletonPtr()->getCameraManager()->addCamera(this->camera, true);
				}

				// Call activation, because e.g. new workspace must be set
				this->setActivated(this->active->getBool());
			});
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
		}
		else if (CameraComponent::AttrOrientation() == attribute->getName())
		{
			this->setCameraDegreeOrientation(attribute->getVector3());
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
		else if (CameraComponent::AttrShowDummyEntity() == attribute->getName())
		{
			this->setShowDummyEntity(attribute->getBool());
		}
		else if (CameraComponent::AttrExcludeRenderCategories() == attribute->getName())
		{
			this->setExcludeRenderCategories(attribute->getString());
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
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->camera->getPosition())));
		propertiesXML->append_node(propertyXML);

		// this->setCameraPosition(this->gameObjectPtr->getSceneNode()->_getDerivedPositionUpdated());

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "CameraOrientation"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, MathHelper::getInstance()->quatToDegrees(this->camera->getOrientation()))));
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

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ExcludeRenderCategories"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->excludeRenderCategories->getString())));
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

		if (true == this->active->getBool())
		{
			// Hide entity for active camera
			if (nullptr != this->camera)
			{
				Ogre::Vector3 desiredWorldPosition = this->position->getVector3();
				Ogre::Quaternion desiredWorldOrientation = MathHelper::getInstance()->degreesToQuat(this->orientation->getVector3());

				this->gameObjectPtr->setAttributePosition(desiredWorldPosition);
				this->gameObjectPtr->setAttributeOrientation(desiredWorldOrientation);

				this->setCameraPosition(this->camera->getParentSceneNode()->convertWorldToLocalPositionUpdated(desiredWorldPosition));
				this->setCameraOrientation(this->camera->getParentSceneNode()->convertWorldToLocalOrientationUpdated(desiredWorldOrientation));

				auto& workspaceBaseCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<WorkspaceBaseComponent>());
				if (nullptr != workspaceBaseCompPtr)
				{
					// if ("MainCamera" == this->camera->getName())
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

						WorkspaceModule::getInstance()->setPrimaryWorkspace(this->gameObjectPtr->getSceneManager(), this->camera, workspaceBaseCompPtr.get());
					}

					// Create and switch workspace
					workspaceBaseCompPtr->createWorkspace();
				}
				else
				{
					this->active->setValue(false);
					Ogre::String message = "[CameraComponent] Could not switch workspace, because this camera component has no corresponding workspace component! Affected game object: " + this->gameObjectPtr->getName();
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, message);
					boost::shared_ptr<EventDataFeedback> eventDataFeedback(new EventDataFeedback(false, message));
					NOWA::AppStateManager::getSingletonPtr()->getEventManager()->threadSafeQueueEvent(eventDataFeedback);
					return;
				}
			}
		}
		else
		{
			bool stillActiveOne = false;
			auto gameObjects = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjects();

			for (auto& it = gameObjects->begin(); it != gameObjects->end(); ++it)
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
			// Send out event, whether is camera has been activated or not, to camera manager and other camera components, to adapt their state
			boost::shared_ptr<EventDataSwitchCamera> eventDataSwitchCamera(new EventDataSwitchCamera(this->gameObjectPtr->getId(), this->gameObjectPtr->getIndexFromComponent(this), this->active->getBool()));
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->threadSafeQueueEvent(eventDataSwitchCamera);
		}
	}

	void CameraComponent::setActivatedFlag(bool activated)
	{
		this->active->setValue(activated);
		if (nullptr != this->camera)
		{
			// NOWA::GraphicsModule::RenderCommand renderCommand = [this, activated]()
			auto closureFunction = [this, activated](Ogre::Real weight)
			{
				if (true == this->bConnected)
				{
					this->dummyEntity->setVisible(this->showDummyEntity->getBool());
				}
				else
				{
					this->dummyEntity->setVisible(true);
				}

				if (this->camera == AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera())
				{
					this->dummyEntity->setVisible(false);
				}

				if (false == activated)
				{
					// if (true == WorkspaceModule::getInstance()->hasMoreThanOneWorkspace())
					{
						auto& workspaceBaseCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<WorkspaceBaseComponent>());
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
			// NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "CameraComponent::setActivatedFlag");
			Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::setActivatedFlag" + Ogre::StringConverter::toString(this->index);
			NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction);
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
		/*if (true == this->bConnected)
		{
			return;
		}*/

		this->position->setValue(position);
		if (nullptr != this->gameObjectPtr)
		{
			this->gameObjectPtr->setAttributePosition(this->position->getVector3());
			NOWA::GraphicsModule::getInstance()->updateCameraPosition(this->camera, camera->getParentSceneNode()->convertWorldToLocalPositionUpdated(position));
		}
	}

	Ogre::Vector3 CameraComponent::getCameraPosition(void) const
	{
		this->position->setValue(this->gameObjectPtr->getSceneNode()->getPosition());
		return this->position->getVector3();
	}

	void CameraComponent::setCameraDegreeOrientation(const Ogre::Vector3& orientation)
	{
		/*if (true == this->bConnected)
		{
			return;
		}*/

		this->orientation->setValue(orientation);
		if (nullptr != this->gameObjectPtr)
		{
			this->gameObjectPtr->setAttributeOrientation(MathHelper::getInstance()->degreesToQuat(orientation));
			/*auto camera = this->camera;
			ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("CameraComponent::setCameraDegreeOrientation", _2(camera, orientation),
			{
				camera->setOrientation(camera->getParentSceneNode()->convertWorldToLocalOrientationUpdated(MathHelper::getInstance()->degreesToQuat(orientation)));
			});*/

			NOWA::GraphicsModule::getInstance()->updateCameraOrientation(this->camera, camera->getParentSceneNode()->convertWorldToLocalOrientationUpdated(MathHelper::getInstance()->degreesToQuat(orientation)));
		}
	}

	void CameraComponent::setCameraOrientation(const Ogre::Quaternion& orientation)
	{
		/*if (true == this->bConnected)
		{
			return;
		}*/

		this->orientation->setValue(MathHelper::getInstance()->quatToDegrees(orientation));
		if (nullptr != this->gameObjectPtr)
		{
			this->gameObjectPtr->setAttributeOrientation(orientation);
			/*auto camera = this->camera;
			ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("CameraComponent::setCameraOrientation", _2(camera, orientation),
			{
				camera->setOrientation(camera->getParentSceneNode()->convertWorldToLocalOrientationUpdated(orientation));
			});*/

			NOWA::GraphicsModule::getInstance()->updateCameraOrientation(this->camera, camera->getParentSceneNode()->convertWorldToLocalOrientationUpdated(orientation));
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
			ENQUEUE_RENDER_COMMAND_MULTI("CameraComponent::setNearClipDistance", _1(nearClipDistance),
			{
				this->camera->setNearClipDistance(nearClipDistance);
			});
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
			ENQUEUE_RENDER_COMMAND_MULTI("CameraComponent::setFarClipDistance", _1(farClipDistance),
			{
				this->camera->setFarClipDistance(farClipDistance);
			});
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
			ENQUEUE_RENDER_COMMAND_MULTI("CameraComponent::setFovy", _1(fovy),
			{
				this->camera->setFOVy(fovy);
			});
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

			ENQUEUE_RENDER_COMMAND("CameraComponent::setOrthographic",
			{
				this->camera->setProjectionType(static_cast<Ogre::ProjectionType>(this->orthographic->getBool() == true ? 0 : 1));
			});
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
				ENQUEUE_RENDER_COMMAND_MULTI("CameraComponent::setOrthoWindowSize", _1(orthoWindowSize),
				{
					this->camera->setOrthoWindow(orthoWindowSize.x, orthoWindowSize.y);
				});
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

	void CameraComponent::setExcludeRenderCategories(const Ogre::String& excludeRenderCategories)
	{
		this->excludeRenderCategories->setValue(excludeRenderCategories);
	}

	void CameraComponent::setAspectRatio(Ogre::Real aspectRatio)
	{
		if (nullptr != this->camera)
		{
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("CameraComponent::setAspectRatio", _1(aspectRatio),
			{
				this->camera->setAspectRatio(aspectRatio);
			});
		}
	}

	Ogre::String CameraComponent::getExcludeRenderCategories(void) const
	{
		return this->excludeRenderCategories->getString();
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
			ENQUEUE_RENDER_COMMAND_MULTI("CameraComponent::setFixedYawAxis", _1(fixedYawAxis),
			{
				this->camera->setFixedYawAxis(fixedYawAxis);
			});
		}
	}

	bool CameraComponent::getFixedYawAxis(void) const
	{
		return this->fixedYawAxis->getBool();
	}

}; // namespace end
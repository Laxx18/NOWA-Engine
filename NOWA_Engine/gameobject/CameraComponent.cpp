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
		hideEntity(true),
		timeSinceLastUpdate(5.0f),
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
		showDummyEntity(new Variant(CameraComponent::AttrShowDummyEntity(), false, this->attributes))
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
	}

	CameraComponent::~CameraComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CameraComponent] Destructor camera component for game object: " + this->gameObjectPtr->getName());

		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &CameraComponent::handleSwitchCamera), EventDataSwitchCamera::getStaticEventType());
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &CameraComponent::handleRemoveCamera), EventDataRemoveCamera::getStaticEventType());

		// If it was an active one, send event
		if (true == this->active->getBool())
		{
			boost::shared_ptr<EventDataRemoveCamera> eventDataRemoveCamera(new EventDataRemoveCamera(this->active->getBool(), this->camera));
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataRemoveCamera);
		}

		if (nullptr != this->camera)
		{
			AppStateManager::getSingletonPtr()->getCameraManager()->removeCamera(this->camera);
			// this->gameObjectPtr->getSceneNode()->detachObject(this->camera);
			this->camera->getParentSceneNode()->detachObject(this->camera);
			this->gameObjectPtr->getSceneManager()->destroyMovableObject(this->camera);
			this->camera = nullptr;
			this->dummyEntity = nullptr;
		}
	}

	void CameraComponent::handleSwitchCamera(NOWA::EventDataPtr eventData)
	{
		boost::shared_ptr<NOWA::EventDataSwitchCamera> castEventData = boost::static_pointer_cast<EventDataSwitchCamera>(eventData);
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

	void CameraComponent::handleRemoveCamera(NOWA::EventDataPtr eventData)
	{
		boost::shared_ptr<NOWA::EventDataRemoveCamera> castEventData = boost::static_pointer_cast<EventDataRemoveCamera>(eventData);
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

	bool CameraComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

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
		return true;
	}

	void CameraComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();
		// Show transform values for game object when camera component has been removed
		this->gameObjectPtr->getAttribute(GameObject::AttrPosition())->setVisible(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrOrientation())->setVisible(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrScale())->setVisible(true);

#if 0
		bool foundAnyOtherCamera = false;
		auto gameObjects = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjects();
		// if the removed camera component had an active camera a successeres must be determined
		for (auto& it = gameObjects->begin(); it != gameObjects->end(); ++it)
		{
			GameObject* gameObject = it->second.get();

			if (gameObject->getId() != this->gameObjectPtr->getId())
			{
				auto cameraComponent = NOWA::makeStrongPtr(gameObject->getComponent<CameraComponent>());
				if (nullptr != cameraComponent)
				{
					foundAnyOtherCamera = true;
					break;
				}
			}
		}

		// A camera must exist!
		if (false == foundAnyOtherCamera && false == AppStateManager::getSingletonPtr()->getIsShutdown())
		{
			Ogre::Camera* camera = this->gameObjectPtr->getSceneManager()->createCamera("GamePlayCamera");
			camera->setFOVy(this->camera->getFOVy());
			camera->setNearClipDistance(this->camera->getNearClipDistance());
			camera->setFarClipDistance(this->camera->getFarClipDistance());
			camera->setQueryFlags(0 << 0);
			camera->setPosition(this->camera->getPosition());

			AppStateManager::getSingletonPtr()->getCameraManager()->init("CameraManager1", camera);
			auto baseCamera = new BaseCamera(AppStateManager::getSingletonPtr()->getCameraManager()->getCameraBehaviorId());
			AppStateManager::getSingletonPtr()->getCameraManager()->addCameraBehavior(baseCamera);
			AppStateManager::getSingletonPtr()->getCameraManager()->setActiveCameraBehavior(cameraType->getBehaviorType());
			// Create dummy workspace

			if (false == WorkspaceModule::getInstance()->getUseSplitScreen())
			{
				WorkspaceModule::getInstance()->setPrimaryWorkspace(this->gameObjectPtr->getSceneManager(), camera, nullptr);
			}
			else
			{
				if (nullptr == WorkspaceModule::getInstance()->getPrimaryWorkspaceComponent())
				{
					WorkspaceModule::getInstance()->setPrimaryWorkspace(this->gameObjectPtr->getSceneManager(), camera, nullptr);
				}
				else
				{
					WorkspaceModule::getInstance()->setNthWorkspace(this->gameObjectPtr->getSceneManager(), camera, nullptr);
				}
			}
		}

		WorkspaceModule::getInstance()->removeCamera(this->camera);
		AppStateManager::getSingletonPtr()->getCameraManager()->removeCamera(this->camera);

		// Send out event, that main camera has changed! so that EditorManager can react and set different camera! but what is with all other components??
#endif

		if (nullptr != this->workspaceBaseComponent && false == AppStateManager::getSingletonPtr()->getIsShutdown())
		{
			this->workspaceBaseComponent->setUseReflection(false);
		}

		WorkspaceModule::getInstance()->removeCamera(this->camera);
	}

	void CameraComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (nullptr != dummyEntity && true == this->hideEntity)
		{
			if (true == this->dummyEntity->isVisible() && false == this->showDummyEntity->getBool())
			{
				this->dummyEntity->setVisible(false);
			}
			else if (true == this->showDummyEntity->getBool())
			{
				if (true == this->dummyEntity->isVisible() && true == this->active->getBool())
				{
					this->dummyEntity->setVisible(false);
				}
				else
				{
					// If its not the active camera and not in split screen
					if (false == this->dummyEntity->isVisible() && false == this->active->getBool() && -1 == this->eyeId)
					{
						this->dummyEntity->setVisible(true);
					}
				}
			}
		}

		/*if (false == this->active->getBool())
		{
			return;
		}*/

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
					this->position->setValue(cameraPosition);
				}

				// Ogre::Quaternion cameraOrientation = this->camera->getDerivedOrientation();

				// this->gameObjectPtr->getSceneNode()->setOrientation(MathHelper::getInstance()->degreesToQuat(this->orientation->getVector3()));
				Ogre::Quaternion cameraOrientation = this->gameObjectPtr->getSceneNode()->_getDerivedOrientationUpdated();
				Ogre::Vector3 orientationDegree = MathHelper::getInstance()->quatToDegrees(cameraOrientation);

				if (false == MathHelper::getInstance()->vector3Equals(this->orientation->getVector3(), orientationDegree, 0.001f))
				{
					this->orientation->setValue(orientationDegree);
				}

				this->timeSinceLastUpdate = 2.0f;
			}
		}
	}

	void CameraComponent::createCamera(void)
	{
		if (nullptr == this->camera)
		{
			this->camera = this->gameObjectPtr->getSceneManager()->createCamera(this->gameObjectPtr->getName());

			// this->camera->detachFromParent();
			// this->gameObjectPtr->getSceneNode()->attachObject(this->camera);

			// As attribute? because it should be set to false, when several viewports are used
			// this->camera->setAutoAspectRatio(true);
#if 1
			this->camera->setFixedYawAxis(this->fixedYawAxis->getBool());
			this->camera->setFOVy(Ogre::Degree(this->fovy->getReal()));
			this->camera->setNearClipDistance(this->nearClipDistance->getReal());
			this->camera->setFarClipDistance(this->farClipDistance->getReal());
			this->camera->setQueryFlags(0 << 0);
			this->setOrthographic(this->orthographic->getBool());
#endif
			// This is required, because when a camera is created via the editor, it must be placed where placenode has been when the user clicked the mouse button
			// But when a camera is loaded from world, it must not have an orientation, else there are ugly side effects
			if (true == CameraComponent::justCreated)
			{
				// this->gameObjectPtr->getSceneNode()->setOrientation(MathHelper::getInstance()->degreesToQuat(this->orientation->getVector3()));
				this->position->setValue(this->gameObjectPtr->getSceneNode()->getPosition());
				this->orientation->setValue(MathHelper::getInstance()->quatToDegreesRounded(this->gameObjectPtr->getSceneNode()->getOrientation()));
			}

			this->gameObjectPtr->getSceneNode()->setPosition(this->position->getVector3());
			this->gameObjectPtr->getSceneNode()->setOrientation(MathHelper::getInstance()->degreesToQuat(this->orientation->getVector3()));

			// Really necessary, so that camera is always up to date with its parent node!

			Ogre::Vector3 position = this->gameObjectPtr->getSceneNode()->getPosition();
			Ogre::Quaternion orientation = this->gameObjectPtr->getSceneNode()->getOrientation();

			this->camera->setPosition(this->camera->getParentSceneNode()->convertWorldToLocalPositionUpdated(position));
			this->camera->setOrientation(this->camera->getParentSceneNode()->convertWorldToLocalOrientationUpdated(orientation));

			// Special treatment: A camera must be created at an early stage, because other game objects and components are relying on this
			// So if its the main camera add the camera to the camera manager to have public available
			if ("MainCamera" == this->camera->getName())
			{
				// Delete camera that existed, before a project was loaded
				Ogre::Camera* previousCamera = NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
				if (nullptr != previousCamera)
				{
					NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->removeCamera(previousCamera);
					this->gameObjectPtr->getSceneManager()->destroyCamera(previousCamera);
					previousCamera = nullptr;
				}

				AppStateManager::getSingletonPtr()->getCameraManager()->addCamera(this->camera, true);
			}

			// AppStateManager::getSingletonPtr()->getCameraManager()->addCamera(this->camera, this->active->getBool());

			// Borrow the entity from the game object
			this->dummyEntity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
			if (nullptr != this->dummyEntity)
			{
				this->dummyEntity->setCastShadows(false);
			
				if ("Camera.mesh" == this->dummyEntity->getMesh()->getName())
				{
					this->hideEntity = true;
				}
			}
			// Call activation, because e.g. new workspace must be set
			this->setActivated(this->active->getBool());
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
	}

	void CameraComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		GameObjectComponent::writeXML(propertiesXML, doc, filePath);

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

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "CameraOrientation"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->orientation->getVector3())));
		propertiesXML->append_node(propertyXML);

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
		if (true == Core::getSingletonPtr()->getIsWorldBeingDestroyed())
		{
			return;
		}

		this->active->setValue(activated);
		bool success = true;

		if (true == this->active->getBool())
		{
			// Hide entity for active camera
			if (nullptr != this->camera)
			{
				auto& workspaceBaseCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<WorkspaceBaseComponent>());
				if (nullptr != workspaceBaseCompPtr)
				{
					this->hideEntity = true;
					this->dummyEntity->setVisible(false);

					// if ("MainCamera" == this->camera->getName())
					{
						AppStateManager::getSingletonPtr()->getCameraManager()->addCamera(this->camera, true);
#if 1
						WorkspaceModule::getInstance()->setPrimaryWorkspace(this->gameObjectPtr->getSceneManager(), this->camera, workspaceBaseCompPtr.get());
#else
						if (false == WorkspaceModule::getInstance()->getUseSplitScreen())
						{
							WorkspaceModule::getInstance()->setPrimaryWorkspace(this->gameObjectPtr->getSceneManager(), this->camera, workspaceBaseCompPtr.get());
						}
						else
						{
							auto primaryWorkSpaceComponent = WorkspaceModule::getInstance()->getPrimaryWorkspaceComponent();
							if (workspaceBaseCompPtr.get() == primaryWorkSpaceComponent)
							{
								WorkspaceModule::getInstance()->setPrimaryWorkspace(this->gameObjectPtr->getSceneManager(), this->camera, workspaceBaseCompPtr.get());
							}
							else
							{
								WorkspaceModule::getInstance()->addNthWorkspace(this->gameObjectPtr->getSceneManager(), this->camera, workspaceBaseCompPtr.get());
							}
						}
#endif
					}

					// Create and switch workspace
					workspaceBaseCompPtr->createWorkspace();
				}
				else
				{
					// Illegal case
					success = false;
					this->active->setValue(false);
					Ogre::String message =  "[CameraComponent] Could not switch workspace, because this camera component has no corresponding workspace component! Affected game object: " + this->gameObjectPtr->getName();
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, message);
					boost::shared_ptr<EventDataFeedback> eventDataFeedback(new EventDataFeedback(false, message));
					NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataFeedback);
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

			if (nullptr != this->dummyEntity)
			{
				this->dummyEntity->setVisible(true);
			}
		}

		if (true == success && nullptr != this->gameObjectPtr)
		{
			// Send out event, whether is camera has been activated or not, to camera manager and other camera components, to adapt their state
			boost::shared_ptr<EventDataSwitchCamera> eventDataSwitchCamera(new EventDataSwitchCamera(this->gameObjectPtr->getId(), this->gameObjectPtr->getIndexFromComponent(this), this->active->getBool()));
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataSwitchCamera);
		}
	}

	void CameraComponent::setActivatedFlag(bool activated)
	{
		this->active->setValue(activated);
		if (nullptr != this->camera)
		{
			// AppStateManager::getSingletonPtr()->getCameraManager()->addCamera(this->camera, activated);
			if (false == activated)
			{
				this->hideEntity = false;
				this->dummyEntity->setVisible(true);

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
				this->hideEntity = true;
				this->dummyEntity->setVisible(false);
			}
			else
			{
				this->eyeId = -1;
				this->hideEntity = false;
				this->dummyEntity->setVisible(true);
			}
		}
	}

	void CameraComponent::setCameraPosition(const Ogre::Vector3& position)
	{
		this->position->setValue(position);
		if (nullptr != this->gameObjectPtr)
		{
			this->gameObjectPtr->getSceneNode()->setPosition(this->position->getVector3());
			this->camera->setPosition(this->camera->getParentSceneNode()->convertWorldToLocalPositionUpdated(position));
		}
	}

	Ogre::Vector3 CameraComponent::getCameraPosition(void) const
	{
		this->position->setValue(this->gameObjectPtr->getSceneNode()->getPosition());
		return this->position->getVector3();
	}

	void CameraComponent::setCameraDegreeOrientation(const Ogre::Vector3& orientation)
	{
		this->orientation->setValue(orientation);
		if (nullptr != this->gameObjectPtr)
		{
			this->gameObjectPtr->getSceneNode()->setOrientation(MathHelper::getInstance()->degreesToQuat(orientation));
			this->camera->setOrientation(this->camera->getParentSceneNode()->convertWorldToLocalOrientationUpdated(MathHelper::getInstance()->degreesToQuat(orientation)));
		}
	}

	void CameraComponent::setCameraOrientation(const Ogre::Quaternion& orientation)
	{
		this->orientation->setValue(MathHelper::getInstance()->quatToDegrees(orientation));
		if (nullptr != this->gameObjectPtr)
		{
			this->gameObjectPtr->getSceneNode()->setOrientation(orientation);
			this->camera->setOrientation(this->camera->getParentSceneNode()->convertWorldToLocalOrientationUpdated(orientation));
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
			this->camera->setNearClipDistance(nearClipDistance);
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
			this->camera->setFarClipDistance(farClipDistance);
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
			this->camera->setFOVy(fovy);
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
			this->camera->setProjectionType(static_cast<Ogre::ProjectionType>(this->orthographic->getBool() == true ? 0 : 1));
			this->orthoWindowSize->setVisible(this->orthographic->getBool());
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
				this->camera->setOrthoWindow(orthoWindowSize.x, orthoWindowSize.y);
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
			this->camera->setFixedYawAxis(fixedYawAxis);
		}
	}

	bool CameraComponent::getFixedYawAxis(void) const
	{
		return this->fixedYawAxis->getBool();
	}

}; // namespace end
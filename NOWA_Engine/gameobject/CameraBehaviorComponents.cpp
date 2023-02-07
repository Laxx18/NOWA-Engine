#include "NOWAPrecompiled.h"
#include "CameraBehaviorComponents.h"
#include "PhysicsActiveComponent.h"
#include "WorkspaceComponents.h"

#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"

#include "camera/BasePhysicsCamera.h"
#include "camera/FirstPersonCamera.h"
#include "camera/ThirdPersonCamera.h"
#include "camera/FollowCamera2D.h"
#include "camera/ZoomCamera.h"
#include "main/AppStateManager.h"
#include "modules/WorkspaceModule.h"

#include "main/Core.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	CameraBehaviorComponent::CameraBehaviorComponent()
		: GameObjectComponent(),
		baseCamera(nullptr),
		oldPosition(Ogre::Vector3::ZERO),
		oldOrientation(Ogre::Quaternion::IDENTITY),
		activated(new Variant(CameraBehaviorComponent::AttrActivated(), false, this->attributes))
	{
		this->activated->setVisible(false);
	}

	CameraBehaviorComponent::~CameraBehaviorComponent()
	{
		if (nullptr != this->baseCamera)
		{
			delete this->baseCamera;
			this->baseCamera = nullptr;
		}
	}

	bool CameraBehaviorComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	bool CameraBehaviorComponent::postInit(void)
	{
		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);

		return true;
	}

	bool CameraBehaviorComponent::connect(void)
	{
		
		return true;
	}

	bool CameraBehaviorComponent::disconnect(void)
	{
		if (nullptr != this->baseCamera)
		{
			AppStateManager::getSingletonPtr()->getCameraManager()->removeCameraBehavior(this->baseCamera->getBehaviorType());
			this->baseCamera = nullptr;
		}
		return true;
	}

	void CameraBehaviorComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (CameraBehaviorComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
	}

	void CameraBehaviorComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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
		propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->activated->getBool())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String CameraBehaviorComponent::getClassName(void) const
	{
		return "CameraBehaviorComponent";
	}

	Ogre::String CameraBehaviorComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void CameraBehaviorComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
		if (nullptr != this->baseCamera)
		{
			if (true == activated)
			{
				
				AppStateManager::getSingletonPtr()->getCameraManager()->addCameraBehavior(this->baseCamera);

				this->oldPosition = this->baseCamera->getCamera()->getParentSceneNode()->convertLocalToWorldPosition(this->baseCamera->getCamera()->getPosition());
				this->oldOrientation = this->baseCamera->getCamera()->getParentSceneNode()->convertLocalToWorldOrientation(this->baseCamera->getCamera()->getOrientation());

				AppStateManager::getSingletonPtr()->getCameraManager()->setActiveCameraBehavior(this->baseCamera->getBehaviorType());
			}
			else
			{
				if (nullptr != this->baseCamera)
				{
					this->baseCamera->getCamera()->setPosition(this->baseCamera->getCamera()->getParentSceneNode()->convertWorldToLocalPositionUpdated(this->oldPosition));
					this->baseCamera->getCamera()->setOrientation(this->baseCamera->getCamera()->getParentSceneNode()->convertWorldToLocalOrientationUpdated(this->oldOrientation));

					AppStateManager::getSingletonPtr()->getCameraManager()->removeCameraBehavior(this->baseCamera->getBehaviorType());

					this->baseCamera = nullptr;
				}
			}
		}
	}

	bool CameraBehaviorComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}
	
	BaseCamera* CameraBehaviorComponent::getBaseCamera(void) const
	{
		return this->baseCamera;
	}

	Ogre::Camera* CameraBehaviorComponent::getCamera(void) const
	{
		return this->baseCamera->getCamera();
	}

	void CameraBehaviorComponent::setCameraControlLocked(bool cameraControlLocked)
	{
		this->baseCamera->setCameraControlLocked(cameraControlLocked);
	}

	bool CameraBehaviorComponent::getCameraControlLocked(void) const
	{
		return this->baseCamera->getCameraControlLocked();
	}
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	CameraBehaviorBaseComponent::CameraBehaviorBaseComponent()
		: CameraBehaviorComponent(),
		moveSpeed(new Variant(CameraBehaviorBaseComponent::AttrMoveSpeed(), 20.0f, this->attributes)),
		rotationSpeed(new Variant(CameraBehaviorBaseComponent::AttrRotationSpeed(), 20.0f, this->attributes)),
		smoothValue(new Variant(CameraBehaviorBaseComponent::AttrSmoothValue(), 0.1f, this->attributes))
	{
		
	}

	CameraBehaviorBaseComponent::~CameraBehaviorBaseComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CameraBehaviorBaseComponent] Destructor camera behavior base component for game object: " + this->gameObjectPtr->getName());
	}

	bool CameraBehaviorBaseComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		bool success = CameraBehaviorComponent::init(propertyElement, filename);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MoveSpeed")
		{
			this->moveSpeed->setValue(XMLConverter::getAttribReal(propertyElement, "data", 20.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RotationSpeed")
		{
			this->rotationSpeed->setValue(XMLConverter::getAttribReal(propertyElement, "data", 20.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SmoothValue")
		{
			this->smoothValue->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.1f));
			propertyElement = propertyElement->next_sibling("property");
		}

		return success;
	}

	GameObjectCompPtr CameraBehaviorBaseComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		CameraBehaviorBaseCompPtr clonedCompPtr(boost::make_shared<CameraBehaviorBaseComponent>());

		
		// Do not clone activated, since its no visible and switched manually in game object controller
		// clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setMoveSpeed(this->moveSpeed->getReal());
		clonedCompPtr->setRotationSpeed(this->rotationSpeed->getReal());
		clonedCompPtr->setSmoothValue(this->smoothValue->getReal());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool CameraBehaviorBaseComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AiMoveComponent] Init camera behavior base component for game object: " + this->gameObjectPtr->getName());

		return CameraBehaviorComponent::postInit();
	}

	bool CameraBehaviorBaseComponent::connect(void)
	{
		bool success = CameraBehaviorComponent::connect();

		return success;
	}

	bool CameraBehaviorBaseComponent::disconnect(void)
	{
		return CameraBehaviorComponent::disconnect();
	}

	void CameraBehaviorBaseComponent::actualizeValue(Variant* attribute)
	{
		CameraBehaviorComponent::actualizeValue(attribute);
		
		if (CameraBehaviorBaseComponent::AttrMoveSpeed() == attribute->getName())
		{
			this->setMoveSpeed(attribute->getReal());
		}
		else if (CameraBehaviorBaseComponent::AttrRotationSpeed() == attribute->getName())
		{
			this->setRotationSpeed(attribute->getReal());
		}
		else if (CameraBehaviorBaseComponent::AttrSmoothValue() == attribute->getName())
		{
			this->setSmoothValue(attribute->getReal());
		}
	}

	void CameraBehaviorBaseComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		CameraBehaviorComponent::writeXML(propertiesXML, doc, filePath);
		
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MoveSpeed"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->moveSpeed->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RotationSpeed"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->rotationSpeed->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SmoothValue"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->smoothValue->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	void CameraBehaviorBaseComponent::setActivated(bool activated)
	{
		if (true == activated && nullptr == this->baseCamera)
		{
			this->baseCamera = new BaseCamera(this->moveSpeed->getReal(), this->rotationSpeed->getReal(), this->smoothValue->getReal());
		}
		CameraBehaviorComponent::setActivated(activated);
	}

	Ogre::String CameraBehaviorBaseComponent::getClassName(void) const
	{
		return "CameraBehaviorBaseComponent";
	}

	Ogre::String CameraBehaviorBaseComponent::getParentClassName(void) const
	{
		return "CameraBehaviorComponent";
	}
	
	void CameraBehaviorBaseComponent::setMoveSpeed(Ogre::Real moveSpeed)
	{
		this->moveSpeed->setValue(moveSpeed);
	}

	Ogre::Real CameraBehaviorBaseComponent::getMoveSpeed(void) const
	{
		return moveSpeed->getReal();
	}
	
	void CameraBehaviorBaseComponent::setRotationSpeed(Ogre::Real rotationSpeed)
	{
		this->rotationSpeed->setValue(rotationSpeed);
	}

	Ogre::Real CameraBehaviorBaseComponent::getRotationSpeed(void) const
	{
		return rotationSpeed->getReal();
	}
	
	void CameraBehaviorBaseComponent::setSmoothValue(Ogre::Real smoothValue)
	{
		this->smoothValue->setValue(smoothValue);
	}

	Ogre::Real CameraBehaviorBaseComponent::getSmoothValue(void) const
	{
		return smoothValue->getReal();
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	CameraBehaviorFirstPersonComponent::CameraBehaviorFirstPersonComponent()
		: CameraBehaviorComponent(),
		smoothValue(new Variant(CameraBehaviorFirstPersonComponent::AttrSmoothValue(), 0.3f, this->attributes)),
		rotationSpeed(new Variant(CameraBehaviorFirstPersonComponent::AttrRotationSpeed(), 0.5f, this->attributes)),
		offsetPosition(new Variant(CameraBehaviorFirstPersonComponent::AttrOffsetPosition(), Ogre::Vector3(0.4f, 0.8f, -0.9f), this->attributes))
	{
		
	}

	CameraBehaviorFirstPersonComponent::~CameraBehaviorFirstPersonComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CameraBehaviorBaseComponent] Destructor camera behavior first person component for game object: " + this->gameObjectPtr->getName());
	}

	bool CameraBehaviorFirstPersonComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		bool success = CameraBehaviorComponent::init(propertyElement, filename);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SmoothValue")
		{
			this->smoothValue->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.3f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RotationSpeed")
		{
			this->rotationSpeed->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.5f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OffsetPosition")
		{
			this->offsetPosition->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3(0.4f, 0.8f, -0.9f)));
			propertyElement = propertyElement->next_sibling("property");
		}

		return success;
	}

	GameObjectCompPtr CameraBehaviorFirstPersonComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		CameraBehaviorFirstPersonCompPtr clonedCompPtr(boost::make_shared<CameraBehaviorFirstPersonComponent>());

		
		// Do not clone activated, since its no visible and switched manually in game object controller
		// clonedCompPtr->setActivated(this->activated->getBool());
		
		clonedCompPtr->setSmoothValue(this->smoothValue->getReal());
		clonedCompPtr->setRotationSpeed(this->rotationSpeed->getReal());
		clonedCompPtr->setOffsetPosition(this->offsetPosition->getVector3());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool CameraBehaviorFirstPersonComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CameraBehaviorFirstPersonComponent] Init camera behavior first person component for game object: " + this->gameObjectPtr->getName());

		return CameraBehaviorComponent::postInit();
	}

	bool CameraBehaviorFirstPersonComponent::connect(void)
	{
		bool success = CameraBehaviorComponent::connect();

		return success;
	}

	bool CameraBehaviorFirstPersonComponent::disconnect(void)
	{
		return CameraBehaviorComponent::disconnect();
	}

	void CameraBehaviorFirstPersonComponent::actualizeValue(Variant* attribute)
	{
		CameraBehaviorComponent::actualizeValue(attribute);
		
		if (CameraBehaviorFirstPersonComponent::AttrSmoothValue() == attribute->getName())
		{
			this->setSmoothValue(attribute->getReal());
		}
		else if (CameraBehaviorFirstPersonComponent::AttrRotationSpeed() == attribute->getName())
		{
			this->setRotationSpeed(attribute->getReal());
		}
		else if (CameraBehaviorFirstPersonComponent::AttrOffsetPosition() == attribute->getName())
		{
			this->setOffsetPosition(attribute->getVector3());
		}
	}

	void CameraBehaviorFirstPersonComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		CameraBehaviorComponent::writeXML(propertiesXML, doc, filePath);
		
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SmoothValue"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->smoothValue->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RotationSpeed"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->rotationSpeed->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OffsetPosition"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->offsetPosition->getVector3())));
		propertiesXML->append_node(propertyXML);
	}

	void CameraBehaviorFirstPersonComponent::setActivated(bool activated)
	{
		if (true == activated && nullptr == this->baseCamera)
		{
			this->baseCamera = new FirstPersonCamera(this->gameObjectPtr->getSceneNode(), this->gameObjectPtr->getDefaultDirection(), this->smoothValue->getReal(),
				this->rotationSpeed->getReal(), this->offsetPosition->getVector3());
		}
		CameraBehaviorComponent::setActivated(activated);
	}

	Ogre::String CameraBehaviorFirstPersonComponent::getClassName(void) const
	{
		return "CameraBehaviorFirstPersonComponent";
	}

	Ogre::String CameraBehaviorFirstPersonComponent::getParentClassName(void) const
	{
		return "CameraBehaviorComponent";
	}
	
	void CameraBehaviorFirstPersonComponent::setSmoothValue(Ogre::Real smoothValue)
	{
		this->smoothValue->setValue(smoothValue);
	}

	Ogre::Real CameraBehaviorFirstPersonComponent::getSmoothValue(void) const
	{
		return smoothValue->getReal();
	}
	
	void CameraBehaviorFirstPersonComponent::setRotationSpeed(Ogre::Real rotationSpeed)
	{
		this->rotationSpeed->setValue(rotationSpeed);
	}

	Ogre::Real CameraBehaviorFirstPersonComponent::getRotationSpeed(void) const
	{
		return rotationSpeed->getReal();
	}
	
	void CameraBehaviorFirstPersonComponent::setOffsetPosition(const Ogre::Vector3& offsetPosition)
	{
		this->offsetPosition->setValue(offsetPosition);
	}

	Ogre::Vector3 CameraBehaviorFirstPersonComponent::getOffsetPosition(void) const
	{
		return offsetPosition->getVector3();
	}
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	CameraBehaviorThirdPersonComponent::CameraBehaviorThirdPersonComponent()
		: CameraBehaviorComponent(),
		yOffset(new Variant(CameraBehaviorThirdPersonComponent::AttrYOffset(), 2.0f, this->attributes)),
		lookAtOffset(new Variant(CameraBehaviorThirdPersonComponent::AttrLookAtOffset(), Ogre::Vector3::ZERO, this->attributes)),
		springForce(new Variant(CameraBehaviorThirdPersonComponent::AttrSpringForce(), 0.1f, this->attributes)),
		friction(new Variant(CameraBehaviorThirdPersonComponent::AttrFriction(), 0.5f, this->attributes)),
		springLength(new Variant(CameraBehaviorThirdPersonComponent::AttrSpringLength(), 6.0f, this->attributes))
	{
		
	}

	CameraBehaviorThirdPersonComponent::~CameraBehaviorThirdPersonComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CameraBehaviorBaseComponent] Destructor camera behavior third person component for game object: " + this->gameObjectPtr->getName());
	}

	bool CameraBehaviorThirdPersonComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		bool success = CameraBehaviorComponent::init(propertyElement, filename);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "YOffset")
		{
			this->yOffset->setValue(XMLConverter::getAttribReal(propertyElement, "data", 2.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "LookAtOffset")
		{
			this->lookAtOffset->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SpringForce")
		{
			this->springForce->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.1f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Friction")
		{
			this->friction->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.5f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SpringLength")
		{
			this->springLength->setValue(XMLConverter::getAttribReal(propertyElement, "data", 6.0f));
			propertyElement = propertyElement->next_sibling("property");
		}

		return success;
	}

	GameObjectCompPtr CameraBehaviorThirdPersonComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		CameraBehaviorThirdPersonCompPtr clonedCompPtr(boost::make_shared<CameraBehaviorThirdPersonComponent>());

		
		// Do not clone activated, since its no visible and switched manually in game object controller
		// clonedCompPtr->setActivated(this->activated->getBool());
		
		clonedCompPtr->setYOffset(this->yOffset->getReal());
		clonedCompPtr->setLookAtOffset(this->lookAtOffset->getVector3());
		clonedCompPtr->setSpringForce(this->springForce->getReal());
		clonedCompPtr->setFriction(this->friction->getReal());
		clonedCompPtr->setSpringLength(this->springLength->getReal());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool CameraBehaviorThirdPersonComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CameraBehaviorThirdPersonComponent] Init camera behavior third person component for game object: " + this->gameObjectPtr->getName());

		return CameraBehaviorComponent::postInit();
	}

	bool CameraBehaviorThirdPersonComponent::connect(void)
	{
		bool success = CameraBehaviorComponent::connect();
		
		return success;
	}

	bool CameraBehaviorThirdPersonComponent::disconnect(void)
	{
		return CameraBehaviorComponent::disconnect();
	}

	void CameraBehaviorThirdPersonComponent::actualizeValue(Variant* attribute)
	{
		CameraBehaviorComponent::actualizeValue(attribute);
		
		if (CameraBehaviorThirdPersonComponent::AttrYOffset() == attribute->getName())
		{
			this->setYOffset(attribute->getReal());
		}
		else if (CameraBehaviorThirdPersonComponent::AttrLookAtOffset() == attribute->getName())
		{
			this->setLookAtOffset(attribute->getVector3());
		}
		else if (CameraBehaviorThirdPersonComponent::AttrSpringForce() == attribute->getName())
		{
			this->setSpringForce(attribute->getReal());
		}
		else if (CameraBehaviorThirdPersonComponent::AttrFriction() == attribute->getName())
		{
			this->setFriction(attribute->getReal());
		}
		else if (CameraBehaviorThirdPersonComponent::AttrSpringLength() == attribute->getName())
		{
			this->setSpringLength(attribute->getReal());
		}
	}

	void CameraBehaviorThirdPersonComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		CameraBehaviorComponent::writeXML(propertiesXML, doc, filePath);
		
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "YOffset"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->yOffset->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "LookAtOffset"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->lookAtOffset->getVector3())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SpringForce"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->springForce->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Friction"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->friction->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SpringLength"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->springLength->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	void CameraBehaviorThirdPersonComponent::setActivated(bool activated)
	{
		if (true == activated && nullptr == this->baseCamera)
		{
			this->baseCamera = new ThirdPersonCamera(this->gameObjectPtr->getSceneNode(), this->gameObjectPtr->getDefaultDirection(),
				this->yOffset->getReal(), this->lookAtOffset->getVector3(), this->springForce->getReal(), this->friction->getReal(), this->springLength->getReal());
			
			if (Core::getSingletonPtr()->getOptionDesiredSimulationUpdates() <= 30)
			{
				this->baseCamera->setSmoothValue(0.0f);
			}
		}
		CameraBehaviorComponent::setActivated(activated);
	}

	Ogre::String CameraBehaviorThirdPersonComponent::getClassName(void) const
	{
		return "CameraBehaviorThirdPersonComponent";
	}

	Ogre::String CameraBehaviorThirdPersonComponent::getParentClassName(void) const
	{
		return "CameraBehaviorComponent";
	}
	
	void CameraBehaviorThirdPersonComponent::setYOffset(Ogre::Real yOffset)
	{
		this->yOffset->setValue(yOffset);
	}

	Ogre::Real CameraBehaviorThirdPersonComponent::getYOffset(void) const
	{
		return yOffset->getReal();
	}

	void CameraBehaviorThirdPersonComponent::setLookAtOffset(const Ogre::Vector3& lookAtOffset)
	{
		this->lookAtOffset->setValue(lookAtOffset);
	}

	const Ogre::Vector3 CameraBehaviorThirdPersonComponent::getLookAtOffset(void) const
	{
		return lookAtOffset->getVector3();
	}
	
	void CameraBehaviorThirdPersonComponent::setSpringForce(Ogre::Real springForce)
	{
		this->springForce->setValue(springForce);
	}

	Ogre::Real CameraBehaviorThirdPersonComponent::getSpringForce(void) const
	{
		return springForce->getReal();
	}
	
	void CameraBehaviorThirdPersonComponent::setFriction(Ogre::Real friction)
	{
		this->friction->setValue(friction);
	}

	Ogre::Real CameraBehaviorThirdPersonComponent::getFriction(void) const
	{
		return friction->getReal();
	}
	
	void CameraBehaviorThirdPersonComponent::setSpringLength(Ogre::Real springLength)
	{
		this->springLength->setValue(springLength);
	}

	Ogre::Real CameraBehaviorThirdPersonComponent::getSpringLength(void) const
	{
		return springLength->getReal();
	}
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	CameraBehaviorFollow2DComponent::CameraBehaviorFollow2DComponent()
		: CameraBehaviorComponent(),
		smoothValue(new Variant(CameraBehaviorFollow2DComponent::AttrSmoothValue(), 0.0f, this->attributes)),
		offsetPosition(new Variant(CameraBehaviorFollow2DComponent::AttrOffsetPosition(), Ogre::Vector3(0.0f, 1.0f, -5.0f), this->attributes)),
		borderOffset(new Variant(CameraBehaviorFollow2DComponent::AttrBorderOffset(), Ogre::Vector3(50.0f, 0.0f, 0.0f), this->attributes))
	{

	}

	CameraBehaviorFollow2DComponent::~CameraBehaviorFollow2DComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CameraBehaviorFollow2DComponent] Destructor camera behavior follow 2D component for game object: " + this->gameObjectPtr->getName());
	}

	bool CameraBehaviorFollow2DComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		bool success = CameraBehaviorComponent::init(propertyElement, filename);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SmoothValue")
		{
			this->smoothValue->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.1f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OffsetPosition")
		{
			this->offsetPosition->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3(0.0f, 1.0f, -5.0f)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "BorderOffset")
		{
			this->borderOffset->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		return success;
	}

	GameObjectCompPtr CameraBehaviorFollow2DComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		CameraBehaviorFollow2DCompPtr clonedCompPtr(boost::make_shared<CameraBehaviorFollow2DComponent>());

		
		// Do not clone activated, since its no visible and switched manually in game object controller
		// clonedCompPtr->setActivated(this->activated->getBool());
		
		clonedCompPtr->setSmoothValue(this->smoothValue->getReal());
		clonedCompPtr->setOffsetPosition(this->offsetPosition->getVector3());
		clonedCompPtr->setBorderOffset(this->borderOffset->getVector3());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool CameraBehaviorFollow2DComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CameraBehaviorFollow2DComponent] Init camera behavior follow 2D component for game object: " + this->gameObjectPtr->getName());

		return CameraBehaviorComponent::postInit();
	}

	bool CameraBehaviorFollow2DComponent::connect(void)
	{
		bool success = CameraBehaviorComponent::connect();
		
		return success;
	}

	bool CameraBehaviorFollow2DComponent::disconnect(void)
	{
		return CameraBehaviorComponent::disconnect();
	}

	void CameraBehaviorFollow2DComponent::actualizeValue(Variant* attribute)
	{
		CameraBehaviorComponent::actualizeValue(attribute);
		
		if (CameraBehaviorFollow2DComponent::AttrSmoothValue() == attribute->getName())
		{
			this->setSmoothValue(attribute->getReal());
		}
		else if (CameraBehaviorFollow2DComponent::AttrOffsetPosition() == attribute->getName())
		{
			this->setOffsetPosition(attribute->getVector3());
		}
		else if (CameraBehaviorFollow2DComponent::AttrBorderOffset() == attribute->getName())
		{
			this->setBorderOffset(attribute->getVector3());
		}
	}

	void CameraBehaviorFollow2DComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		CameraBehaviorComponent::writeXML(propertiesXML, doc, filePath);
		
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SmoothValue"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->smoothValue->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OffsetPosition"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->offsetPosition->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "BorderOffset"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->borderOffset->getVector3())));
		propertiesXML->append_node(propertyXML);
	}

	void CameraBehaviorFollow2DComponent::setActivated(bool activated)
	{
		if (true == activated && nullptr == this->baseCamera)
		{
			this->baseCamera = new FollowCamera2D(this->gameObjectPtr->getSceneNode(), this->offsetPosition->getVector3(), this->smoothValue->getReal());
		}
		CameraBehaviorComponent::setActivated(activated);
		if (nullptr != this->baseCamera)
		{
			static_cast<FollowCamera2D*>(this->baseCamera)->setBorderOffset(this->borderOffset->getVector3());
			static_cast<FollowCamera2D*>(this->baseCamera)->setBounds(Core::getSingletonPtr()->getCurrentWorldBoundLeftNear(), Core::getSingletonPtr()->getCurrentWorldBoundRightFar());
		}
	}

	Ogre::String CameraBehaviorFollow2DComponent::getClassName(void) const
	{
		return "CameraBehaviorFollow2DComponent";
	}

	Ogre::String CameraBehaviorFollow2DComponent::getParentClassName(void) const
	{
		return "CameraBehaviorComponent";
	}
	
	void CameraBehaviorFollow2DComponent::setSmoothValue(Ogre::Real smoothValue)
	{
		this->smoothValue->setValue(smoothValue);
	}

	Ogre::Real CameraBehaviorFollow2DComponent::getSmoothValue(void) const
	{
		return smoothValue->getReal();
	}
	
	void CameraBehaviorFollow2DComponent::setOffsetPosition(const Ogre::Vector3& offsetPosition)
	{
		this->offsetPosition->setValue(offsetPosition);
	}

	Ogre::Vector3 CameraBehaviorFollow2DComponent::getOffsetPosition(void) const
	{
		return offsetPosition->getVector3();
	}

	void CameraBehaviorFollow2DComponent::setBorderOffset(const Ogre::Vector3& borderOffset)
	{
		Ogre::Vector3 tempBorderOffset = Ogre::Vector3(Ogre::Math::Abs(borderOffset.x), Ogre::Math::Abs(borderOffset.y), Ogre::Math::Abs(borderOffset.z));

		Ogre::Vector3 leftNearBound = Core::getSingletonPtr()->getCurrentWorldBoundLeftNear();
		Ogre::Vector3 rightFarBound = Core::getSingletonPtr()->getCurrentWorldBoundRightFar();

		Ogre::Vector3 border = (leftNearBound + rightFarBound) / 4.0f;

		if (tempBorderOffset.x != 0.0f && tempBorderOffset.x > border.x)
		{
			tempBorderOffset.x = border.x;
		}
		if (tempBorderOffset.z != 0.0f && tempBorderOffset.y > border.y)
		{
			tempBorderOffset.y = border.y;
		}
		if (tempBorderOffset.z != 0.0f && tempBorderOffset.z > border.z)
		{
			tempBorderOffset.z = border.z;
		}
		
		this->borderOffset->setValue(tempBorderOffset);
	}

	Ogre::Vector3 CameraBehaviorFollow2DComponent::getBorderOffset(void) const
	{
		return this->borderOffset->getVector3();
	}
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	CameraBehaviorZoomComponent::CameraBehaviorZoomComponent()
		: CameraBehaviorComponent(),
		category(new Variant(CameraBehaviorZoomComponent::AttrCategory(), Ogre::String("All"), this->attributes)),
		smoothValue(new Variant(CameraBehaviorZoomComponent::AttrSmoothValue(), 0.2f, this->attributes)),
		growMultiplicator(new Variant(CameraBehaviorZoomComponent::AttrGrowMultiplicator(), 2.0f, this->attributes))
	{
		this->growMultiplicator->setDescription("Sets a grow multiplicator how fast the orthogonal camera window size will increase, so that the game objects remain in view."
			"Play with this value, because it also depends e.g. how fast your game objects are moving.");
	}

	CameraBehaviorZoomComponent::~CameraBehaviorZoomComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CameraBehaviorZoomComponent] Destructor camera behavior zoom component for game object: " + this->gameObjectPtr->getName());
	}

	bool CameraBehaviorZoomComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		bool success = CameraBehaviorComponent::init(propertyElement, filename);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Category")
		{
			this->category->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SmoothValue")
		{
			this->smoothValue->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.1f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "GrowMultiplicator")
		{
			this->growMultiplicator->setValue(XMLConverter::getAttribReal(propertyElement, "data", 2.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		return success;
	}

	GameObjectCompPtr CameraBehaviorZoomComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		CameraBehaviorZoomCompPtr clonedCompPtr(boost::make_shared<CameraBehaviorZoomComponent>());

		
		// Do not clone activated, since its no visible and switched manually in game object controller
		// clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setCategory(this->category->getString());
		clonedCompPtr->setSmoothValue(this->smoothValue->getReal());
		clonedCompPtr->setGrowMultiplicator(this->growMultiplicator->getReal());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool CameraBehaviorZoomComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CameraBehaviorZoomComponent] Init camera behavior zoom component for game object: " + this->gameObjectPtr->getName());

		return CameraBehaviorComponent::postInit();
	}

	bool CameraBehaviorZoomComponent::connect(void)
	{
		bool success = CameraBehaviorComponent::connect();
		
		return success;
	}

	bool CameraBehaviorZoomComponent::disconnect(void)
	{
		return CameraBehaviorComponent::disconnect();
	}

	void CameraBehaviorZoomComponent::actualizeValue(Variant* attribute)
	{
		CameraBehaviorComponent::actualizeValue(attribute);
		
		if (CameraBehaviorZoomComponent::AttrCategory() == attribute->getName())
		{
			this->setCategory(attribute->getString());
		}
		else if (CameraBehaviorZoomComponent::AttrSmoothValue() == attribute->getName())
		{
			this->setSmoothValue(attribute->getReal());
		}
		else if (CameraBehaviorZoomComponent::AttrGrowMultiplicator() == attribute->getName())
		{
			this->setGrowMultiplicator(attribute->getReal());
		}
	}

	void CameraBehaviorZoomComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		CameraBehaviorComponent::writeXML(propertiesXML, doc, filePath);
		
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Category"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->category->getString())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SmoothValue"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->smoothValue->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "GrowMultiplicator"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->growMultiplicator->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	void CameraBehaviorZoomComponent::setActivated(bool activated)
	{
		if (true == activated && nullptr == this->baseCamera)
		{
			this->baseCamera = new ZoomCamera(Ogre::Vector3::ZERO, this->smoothValue->getReal());
		}
		CameraBehaviorComponent::setActivated(activated);
		if (nullptr != this->baseCamera)
		{
			static_cast<ZoomCamera*>(this->baseCamera)->setCategory(this->category->getString());
			static_cast<ZoomCamera*>(this->baseCamera)->setGrowMultiplicator(this->growMultiplicator->getReal());
		}
	}

	Ogre::String CameraBehaviorZoomComponent::getClassName(void) const
	{
		return "CameraBehaviorZoomComponent";
	}

	Ogre::String CameraBehaviorZoomComponent::getParentClassName(void) const
	{
		return "CameraBehaviorComponent";
	}
	
	void CameraBehaviorZoomComponent::setCategory(const Ogre::String& category)
	{
		this->category->setValue(category);

		if (nullptr != this->baseCamera)
		{
			static_cast<ZoomCamera*>(this->baseCamera)->setGrowMultiplicator(this->growMultiplicator->getReal());
		}
	}
	
	Ogre::String CameraBehaviorZoomComponent::getCategory(void) const
	{
		return this->category->getString();
	}
	
	void CameraBehaviorZoomComponent::setSmoothValue(Ogre::Real smoothValue)
	{
		this->smoothValue->setValue(smoothValue);
	}

	Ogre::Real CameraBehaviorZoomComponent::getSmoothValue(void) const
	{
		return smoothValue->getReal();
	}

	void CameraBehaviorZoomComponent::setGrowMultiplicator(Ogre::Real growMultiplicator)
	{
		if (growMultiplicator < 0.0f)
		{
			growMultiplicator = 1.0f;
		}
		else if (growMultiplicator > 5.0f)
		{
			growMultiplicator = 5.0f;
		}

		this->growMultiplicator->setValue(growMultiplicator);

		if (nullptr != this->baseCamera)
		{
			static_cast<ZoomCamera*>(this->baseCamera)->setGrowMultiplicator(this->growMultiplicator->getReal());
		}
	}

	Ogre::Real CameraBehaviorZoomComponent::getGrowMultiplicator(void) const
	{
		return this->growMultiplicator->getReal();
	}

}; // namespace end
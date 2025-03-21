#include "NOWAPrecompiled.h"
#include "CameraBehaviorAttachComponent.h"
#include "utilities/XMLConverter.h"
#include "modules/LuaScriptApi.h"
#include "main/EventManager.h"
#include "main/AppStateManager.h"
#include "gameobject/GameObjectFactory.h"
#include "camera/AttachCamera.h"

#include "OgreAbiUtils.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	CameraBehaviorAttachComponent::CameraBehaviorAttachComponent()
		: CameraBehaviorComponent(),
		name("CameraBehaviorAttachComponent"),
		smoothValue(new Variant(CameraBehaviorAttachComponent::AttrSmoothValue(), 0.6f, this->attributes)),
		offsetPosition(new Variant(CameraBehaviorAttachComponent::AttrOffsetPosition(), Ogre::Vector3::ZERO, this->attributes)),
		offsetOrientation(new Variant(CameraBehaviorAttachComponent::AttrOffsetOrientation(), Ogre::Vector3::ZERO, this->attributes))
	{
		
	}

	CameraBehaviorAttachComponent::~CameraBehaviorAttachComponent(void)
	{
		
	}

	const Ogre::String& CameraBehaviorAttachComponent::getName() const
	{
		return this->name;
	}

	void CameraBehaviorAttachComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<CameraBehaviorAttachComponent>(CameraBehaviorAttachComponent::getStaticClassId(), CameraBehaviorAttachComponent::getStaticClassName());
	}
	
	void CameraBehaviorAttachComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool CameraBehaviorAttachComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		CameraBehaviorComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SmoothValue")
		{
			this->smoothValue->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.3f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OffsetPosition")
		{
			this->offsetPosition->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OffsetOrientation")
		{
			this->offsetOrientation->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		
		return true;
	}

	GameObjectCompPtr CameraBehaviorAttachComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		CameraBehaviorAttachCompPtr clonedCompPtr(boost::make_shared<CameraBehaviorAttachComponent>());

		// Do not clone activated, since its no visible and switched manually in game object controller
		// clonedCompPtr->setActivated(this->activated->getBool());

		clonedCompPtr->setSmoothValue(this->smoothValue->getReal());
		clonedCompPtr->setOffsetPosition(this->offsetPosition->getVector3());
		clonedCompPtr->setOffsetOrientation(this->offsetOrientation->getVector3());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		clonedCompPtr->setCameraGameObjectId(this->cameraGameObjectId->getULong());

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool CameraBehaviorAttachComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CameraBehaviorAttachComponent] Init component for game object: " + this->gameObjectPtr->getName());

		return CameraBehaviorComponent::postInit();
	}

	bool CameraBehaviorAttachComponent::connect(void)
	{
		bool success = CameraBehaviorComponent::connect();

		this->setActivated(true);
		
		return success;
	}

	bool CameraBehaviorAttachComponent::disconnect(void)
	{
		return CameraBehaviorComponent::disconnect();
	}

	bool CameraBehaviorAttachComponent::onCloned(void)
	{
		
		return true;
	}

	void CameraBehaviorAttachComponent::onRemoveComponent(void)
	{
		CameraBehaviorComponent::onRemoveComponent();
	}
	
	void CameraBehaviorAttachComponent::onOtherComponentRemoved(unsigned int index)
	{
		
	}
	
	void CameraBehaviorAttachComponent::onOtherComponentAdded(unsigned int index)
	{
		
	}
	
	void CameraBehaviorAttachComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
			// Do something
		}
	}

	void CameraBehaviorAttachComponent::actualizeValue(Variant* attribute)
	{
		CameraBehaviorComponent::actualizeValue(attribute);

		if (CameraBehaviorAttachComponent::AttrSmoothValue() == attribute->getName())
		{
			this->setSmoothValue(attribute->getReal());
		}
		else if (CameraBehaviorAttachComponent::AttrOffsetPosition() == attribute->getName())
		{
			this->setOffsetPosition(attribute->getVector3());
		}
		else if (CameraBehaviorAttachComponent::AttrOffsetOrientation() == attribute->getName())
		{
			this->setOffsetOrientation(attribute->getVector3());
		}
	}

	void CameraBehaviorAttachComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		CameraBehaviorComponent::writeXML(propertiesXML, doc);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SmoothValue"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->smoothValue->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OffsetPosition"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->offsetPosition->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OffsetOrientation"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->offsetOrientation->getVector3())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String CameraBehaviorAttachComponent::getClassName(void) const
	{
		return "CameraBehaviorAttachComponent";
	}

	Ogre::String CameraBehaviorAttachComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void CameraBehaviorAttachComponent::setActivated(bool activated)
	{
		if (true == activated && nullptr == this->baseCamera)
		{
			this->acquireActiveCamera();
			this->baseCamera = new AttachCamera(AppStateManager::getSingletonPtr()->getCameraManager()->getCameraBehaviorId(), this->gameObjectPtr->getSceneNode(),
				this->offsetPosition->getVector3(), MathHelper::getInstance()->degreesToQuat(this->offsetOrientation->getVector3()), this->smoothValue->getReal(), this->gameObjectPtr->getDefaultDirection());
		}

		CameraBehaviorComponent::setActivated(activated);

		if (true == activated)
		{
			Ogre::Real smoothValue = this->smoothValue->getReal();
			if (0.0f == smoothValue)
			{
				smoothValue = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCameraBehavior(this->activeCamera)->getSmoothValue();
			}
			this->baseCamera->setSmoothValue(smoothValue);
		}
	}

	bool CameraBehaviorAttachComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void CameraBehaviorAttachComponent::setSmoothValue(Ogre::Real smoothValue)
	{
		this->smoothValue->setValue(smoothValue);
	}

	Ogre::Real CameraBehaviorAttachComponent::getSmoothValue(void) const
	{
		return this->smoothValue->getReal();
	}

	void CameraBehaviorAttachComponent::setOffsetPosition(const Ogre::Vector3& offsetPosition)
	{
		this->offsetPosition->setValue(offsetPosition);
	}

	Ogre::Vector3 CameraBehaviorAttachComponent::getOffsetPosition(void) const
	{
		return this->offsetPosition->getVector3();
	}

	void CameraBehaviorAttachComponent::setOffsetOrientation(const Ogre::Vector3& offsetOrientation)
	{
		this->offsetOrientation->setValue(offsetOrientation);
	}

	Ogre::Vector3 CameraBehaviorAttachComponent::getOffsetOrientation(void) const
	{
		return this->offsetOrientation->getVector3();
	}

	// Lua registration part

	CameraBehaviorAttachComponent* getCameraBehaviorAttachComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<CameraBehaviorAttachComponent>(gameObject->getComponentWithOccurrence<CameraBehaviorAttachComponent>(occurrenceIndex)).get();
	}

	CameraBehaviorAttachComponent* getCameraBehaviorAttachComponent(GameObject* gameObject)
	{
		return makeStrongPtr<CameraBehaviorAttachComponent>(gameObject->getComponent<CameraBehaviorAttachComponent>()).get();
	}

	CameraBehaviorAttachComponent* getCameraBehaviorAttachComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<CameraBehaviorAttachComponent>(gameObject->getComponentFromName<CameraBehaviorAttachComponent>(name)).get();
	}

	void CameraBehaviorAttachComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
		[
			class_<CameraBehaviorAttachComponent, CameraBehaviorComponent>("CameraBehaviorAttachComponent")
			.def("setSmoothValue", &CameraBehaviorAttachComponent::setSmoothValue)
			.def("getSmoothValue", &CameraBehaviorAttachComponent::getSmoothValue)
			.def("setOffsetPosition", &CameraBehaviorAttachComponent::setOffsetPosition)
			.def("getOffsetPosition", &CameraBehaviorAttachComponent::getOffsetPosition)
			.def("setOffsetOrientation", &CameraBehaviorAttachComponent::setOffsetOrientation)
			.def("getOffsetOrientation", &CameraBehaviorAttachComponent::getOffsetOrientation)
		];
		LuaScriptApi::getInstance()->addClassToCollection("CameraBehaviorAttachComponent", "class inherits CameraBehaviorComponent", CameraBehaviorAttachComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("CameraBehaviorAttachComponent", "void setSmoothValue(float smoothValue)", "Sets the camera value for more smooth transform. Note: Setting to 0, camera transform is not smooth, setting to 1 would be to smooth and lag behind, a good value is 0.1");
		LuaScriptApi::getInstance()->addClassToCollection("CameraBehaviorAttachComponent", "float getSmoothValue()", "Gets the camera value for more smooth transform.");
		LuaScriptApi::getInstance()->addClassToCollection("CameraBehaviorAttachComponent", "void setOffsetPosition(Vector3 offsetPosition)", "Sets the camera offset position, it should be away from the game object.");
		LuaScriptApi::getInstance()->addClassToCollection("CameraBehaviorAttachComponent", "Vector3 getOffsetPosition()", "Gets the offset position, the camera is away from the game object.");
		LuaScriptApi::getInstance()->addClassToCollection("CameraBehaviorAttachComponent", "void setOffsetOrientation(Vector3 offsetOrientation)", "Sets the camera offset orientation.");
		LuaScriptApi::getInstance()->addClassToCollection("CameraBehaviorAttachComponent", "Vector3 getOffsetOrientation()", "Gets the offset orietation of the camera.");

		gameObjectClass.def("getCameraBehaviorAttachComponentFromName", &getCameraBehaviorAttachComponentFromName);
		gameObjectClass.def("getCameraBehaviorAttachComponent", (CameraBehaviorAttachComponent * (*)(GameObject*)) & getCameraBehaviorAttachComponent);
		// If its desired to create several of this components for one game object
		gameObjectClass.def("getCameraBehaviorAttachComponentFromIndex", (CameraBehaviorAttachComponent * (*)(GameObject*, unsigned int)) & getCameraBehaviorAttachComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "CameraBehaviorAttachComponent getCameraBehaviorAttachComponentFromIndex(unsigned int occurrenceIndex)", "Gets the component by the given occurence index, since a game object may this component maybe several times.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "CameraBehaviorAttachComponent getCameraBehaviorAttachComponent()", "Gets the component. This can be used if the game object this component just once.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "CameraBehaviorAttachComponent getCameraBehaviorAttachComponentFromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castCameraBehaviorAttachComponent", &GameObjectController::cast<CameraBehaviorAttachComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "CameraBehaviorAttachComponent castCameraBehaviorAttachComponent(CameraBehaviorAttachComponent other)", "Casts an incoming type from function for lua auto completion.");
	}

	bool CameraBehaviorAttachComponent::canStaticAddComponent(GameObject* gameObject)
	{
		if (gameObject->getComponentCount<CameraBehaviorAttachComponent>() < 2)
		{
			return true;
		}
	}

}; //namespace end
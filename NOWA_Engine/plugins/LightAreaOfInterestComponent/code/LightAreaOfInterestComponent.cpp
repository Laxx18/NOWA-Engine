#include "NOWAPrecompiled.h"
#include "LightAreaOfInterestComponent.h"
#include "utilities/XMLConverter.h"
#include "modules/LuaScriptApi.h"
#include "main/EventManager.h"
#include "main/AppStateManager.h"
#include "gameobject/GameObjectFactory.h"

#include "OgreAbiUtils.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	LightAreaOfInterestComponent::LightAreaOfInterestComponent()
		: GameObjectComponent(),
		name("LightAreaOfInterestComponent"),
		activated(new Variant(LightAreaOfInterestComponent::AttrActivated(), true, this->attributes)),
		halfSize(new Variant(LightAreaOfInterestComponent::AttrHalfSize(), Ogre::Vector3(1.0f, 1.0f, 0.5f), this->attributes)),
		sphereRadius(new Variant(LightAreaOfInterestComponent::AttrSphereRadius(), 30.0f, this->attributes))
	{
		this->activated->setDescription("Whether this AoI is active and should be used by InstantRadiosity.");
		this->halfSize->setDescription("Half-size of the AoI box. Adjust to match your window/opening size. "
			"For a 2x3 meter window, use half-size (1, 1.5, 0.25).");
		this->sphereRadius->setDescription("Sphere radius for ray tracing. Larger values ensure rays hit walls "
			"behind the opening. Use ~30 for typical indoor scenes.");
	}

	LightAreaOfInterestComponent::~LightAreaOfInterestComponent(void)
	{

	}

	void LightAreaOfInterestComponent::initialise()
	{

	}

	const Ogre::String& LightAreaOfInterestComponent::getName() const
	{
		return this->name;
	}

	void LightAreaOfInterestComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<LightAreaOfInterestComponent>(LightAreaOfInterestComponent::getStaticClassId(), LightAreaOfInterestComponent::getStaticClassName());
	}

	void LightAreaOfInterestComponent::shutdown()
	{
		// Do nothing here
	}

	void LightAreaOfInterestComponent::uninstall()
	{
		// Do nothing here
	}

	void LightAreaOfInterestComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool LightAreaOfInterestComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		bool success = GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "HalfSize")
		{
			this->halfSize->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SphereRadius")
		{
			this->sphereRadius->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		return success;
	}

	GameObjectCompPtr LightAreaOfInterestComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		LightAreaOfInterestComponentPtr clonedCompPtr(boost::make_shared<LightAreaOfInterestComponent>());

		clonedCompPtr->setHalfSize(this->halfSize->getVector3());
		clonedCompPtr->setSphereRadius(this->sphereRadius->getReal());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		clonedCompPtr->setActivated(this->activated->getBool());

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool LightAreaOfInterestComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[LightAreaOfInterestComponent] Init component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool LightAreaOfInterestComponent::connect(void)
	{
		GameObjectComponent::connect();

		return true;
	}

	bool LightAreaOfInterestComponent::disconnect(void)
	{
		GameObjectComponent::disconnect();

		return true;
	}

	void LightAreaOfInterestComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();
	}

	void LightAreaOfInterestComponent::update(Ogre::Real dt, bool notSimulating)
	{
		// This component doesn't need updates - it's just a marker with data
	}

	void LightAreaOfInterestComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (LightAreaOfInterestComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (LightAreaOfInterestComponent::AttrHalfSize() == attribute->getName())
		{
			this->setHalfSize(attribute->getVector3());
		}
		else if (LightAreaOfInterestComponent::AttrSphereRadius() == attribute->getName())
		{
			this->setSphereRadius(attribute->getReal());
		}
	}

	void LightAreaOfInterestComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "HalfSize"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->halfSize->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SphereRadius"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->sphereRadius->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String LightAreaOfInterestComponent::getClassName(void) const
	{
		return "LightAreaOfInterestComponent";
	}

	Ogre::String LightAreaOfInterestComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void LightAreaOfInterestComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
	}

	bool LightAreaOfInterestComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void LightAreaOfInterestComponent::setHalfSize(const Ogre::Vector3& halfSize)
	{
		Ogre::Vector3 clampedSize = halfSize;
		// Ensure minimum size
		if (clampedSize.x < 0.01f) clampedSize.x = 0.01f;
		if (clampedSize.y < 0.01f) clampedSize.y = 0.01f;
		if (clampedSize.z < 0.01f) clampedSize.z = 0.01f;
		this->halfSize->setValue(clampedSize);
	}

	Ogre::Vector3 LightAreaOfInterestComponent::getHalfSize(void) const
	{
		return this->halfSize->getVector3();
	}

	void LightAreaOfInterestComponent::setSphereRadius(Ogre::Real sphereRadius)
	{
		if (sphereRadius < 0.1f)
		{
			sphereRadius = 0.1f;
		}
		this->sphereRadius->setValue(sphereRadius);
	}

	Ogre::Real LightAreaOfInterestComponent::getSphereRadius(void) const
	{
		return this->sphereRadius->getReal();
	}

	Ogre::Vector3 LightAreaOfInterestComponent::getCenter(void) const
	{
		if (nullptr != this->gameObjectPtr)
		{
			return this->gameObjectPtr->getPosition();
		}
		return Ogre::Vector3::ZERO;
	}

	// Lua registration part

	LightAreaOfInterestComponent* getLightAreaOfInterestComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<LightAreaOfInterestComponent>(gameObject->getComponentWithOccurrence<LightAreaOfInterestComponent>(occurrenceIndex)).get();
	}

	LightAreaOfInterestComponent* getLightAreaOfInterestComponent(GameObject* gameObject)
	{
		return makeStrongPtr<LightAreaOfInterestComponent>(gameObject->getComponent<LightAreaOfInterestComponent>()).get();
	}

	LightAreaOfInterestComponent* getLightAreaOfInterestComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<LightAreaOfInterestComponent>(gameObject->getComponentFromName<LightAreaOfInterestComponent>(name)).get();
	}

	void LightAreaOfInterestComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
			[
				class_<LightAreaOfInterestComponent, GameObjectComponent>("LightAreaOfInterestComponent")
					.def("setActivated", &LightAreaOfInterestComponent::setActivated)
					.def("isActivated", &LightAreaOfInterestComponent::isActivated)
					.def("setHalfSize", &LightAreaOfInterestComponent::setHalfSize)
					.def("getHalfSize", &LightAreaOfInterestComponent::getHalfSize)
					.def("setSphereRadius", &LightAreaOfInterestComponent::setSphereRadius)
					.def("getSphereRadius", &LightAreaOfInterestComponent::getSphereRadius)
					.def("getCenter", &LightAreaOfInterestComponent::getCenter)
			];

		LuaScriptApi::getInstance()->addClassToCollection("LightAreaOfInterestComponent", "class inherits GameObjectComponent", LightAreaOfInterestComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("LightAreaOfInterestComponent", "void setActivated(bool activated)", "Sets whether this AoI is active.");
		LuaScriptApi::getInstance()->addClassToCollection("LightAreaOfInterestComponent", "bool isActivated()", "Gets whether this AoI is active.");
		LuaScriptApi::getInstance()->addClassToCollection("LightAreaOfInterestComponent", "void setHalfSize(Vector3 halfSize)", "Sets the half-size of the AoI box.");
		LuaScriptApi::getInstance()->addClassToCollection("LightAreaOfInterestComponent", "Vector3 getHalfSize()", "Gets the half-size of the AoI box.");
		LuaScriptApi::getInstance()->addClassToCollection("LightAreaOfInterestComponent", "void setSphereRadius(float sphereRadius)", "Sets the sphere radius for ray tracing.");
		LuaScriptApi::getInstance()->addClassToCollection("LightAreaOfInterestComponent", "float getSphereRadius()", "Gets the sphere radius.");
		LuaScriptApi::getInstance()->addClassToCollection("LightAreaOfInterestComponent", "Vector3 getCenter()", "Gets the center position (from GameObject position).");

		gameObjectClass.def("getLightAreaOfInterestComponentFromName", &getLightAreaOfInterestComponentFromName);
		gameObjectClass.def("getLightAreaOfInterestComponent", (LightAreaOfInterestComponent * (*)(GameObject*)) & getLightAreaOfInterestComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "LightAreaOfInterestComponent getLightAreaOfInterestComponent()", "Gets the component.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "LightAreaOfInterestComponent getLightAreaOfInterestComponentFromName(String name)", "Gets the component by name.");

		gameObjectControllerClass.def("castLightAreaOfInterestComponent", &GameObjectController::cast<LightAreaOfInterestComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "LightAreaOfInterestComponent castLightAreaOfInterestComponent(LightAreaOfInterestComponent other)", "Casts an incoming type from function for lua auto completion.");
	}

	bool LightAreaOfInterestComponent::canStaticAddComponent(GameObject* gameObject)
	{
		// Can add multiple LightAoI components per scene (one per window/opening)
		return true;
	}

}; //namespace end
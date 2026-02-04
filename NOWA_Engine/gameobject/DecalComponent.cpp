#include "NOWAPrecompiled.h"

#include "DecalComponent.h"
#include "modules/GraphicsModule.h"
#include "GameObjectController.h"
#include "utilities/XMLConverter.h"
#include "modules/DeployResourceModule.h"
#include "main/AppStateManager.h"
#include "modules/DecalsModule.h"
#include "modules/LuaScriptApi.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	DecalComponent::DecalComponent()
		: GameObjectComponent(),
		decal(nullptr),
		diffuseTextureName(new Variant(DecalComponent::AttrDiffuseTextureName(), Ogre::String("floor_diffuse.PNG"), this->attributes)),
		normalTextureName(new Variant(DecalComponent::AttrNormalTextureName(), Ogre::String("floor_bump.PNG"), this->attributes)),
		emissiveTextureName(new Variant(DecalComponent::AttrEmissiveTextureName(), Ogre::String(""), this->attributes)),
		ignoreAlpha(new Variant(DecalComponent::AttrIgnoreAlpha(), false, this->attributes)),
		metalness(new Variant(DecalComponent::AttrMetalness(), 1.0f, this->attributes)),
		roughness(new Variant(DecalComponent::AttrRoughness(), 1.0f, this->attributes)),
		rectSize(new Variant(DecalComponent::AttrRectSize(), Ogre::Vector3(1.0f, 1.0f, 1.0f), this->attributes))
	{
		this->metalness->setConstraints(0.0f, 1.0f);
		this->roughness->setConstraints(0.0f, 1.0f);
		this->diffuseTextureName->setDescription("Diffuse decal texture (goes into the reserved decals texture array pool). Use empty string to disable diffuse for this decal.");
		this->normalTextureName->setDescription("Normal decal texture (RG8_SNORM in reserved pool). Use empty string to disable normals for this decal.");
		this->emissiveTextureName->setDescription("Emissive decal texture. If empty, emissive will follow diffuse (Hlms optimisation). Use empty diffuse + empty emissive to disable emission.");
		this->ignoreAlpha->setDescription("If true, the decal ignores alpha from the diffuse texture (treats it as fully opaque).");
		this->metalness->setDescription("Metalness multiplier applied by the decal. Typical range 0..1.");
		this->roughness->setDescription("Roughness multiplier applied by the decal. Typical range 0..1.");
		this->rectSize->setDescription("Decals are 2D by nature. The depth of the decal indicates its influence on the objects."
			"Decals are like oriented boxes, and everything inside the box will be affected by the decal.");
	}

	DecalComponent::~DecalComponent()
	{
		
	}

	bool DecalComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "DiffuseTextureName")
		{
			this->diffuseTextureName->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "NormalTextureName")
		{
			this->normalTextureName->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "EmissiveTextureName")
		{
			this->emissiveTextureName->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "IgnoreAlpha")
		{
			this->ignoreAlpha->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Metalness")
		{
			this->metalness->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Roughness")
		{
			this->roughness->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RectSize")
		{
			this->rectSize->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	GameObjectCompPtr DecalComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		DecalCompPtr clonedCompPtr(boost::make_shared<DecalComponent>());

		clonedCompPtr->setDiffuseTextureName(this->diffuseTextureName->getString());
		clonedCompPtr->setNormalTextureName(this->normalTextureName->getString());
		clonedCompPtr->setEmissiveTextureName(this->emissiveTextureName->getString());
		clonedCompPtr->setIgnoreAlpha(this->ignoreAlpha->getBool());
		clonedCompPtr->setMetalness(this->metalness->getReal());
		clonedCompPtr->setRoughness(this->roughness->getReal());
		clonedCompPtr->setRectSize(this->rectSize->getVector3());
		
		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool DecalComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DecalComponent] Init decal component for game object: " + this->gameObjectPtr->getName());

		this->createDecal();
		return true;
	}

	void DecalComponent::onRemoveComponent(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DecalComponent] Destructor decal component for game object: " + this->gameObjectPtr->getName());
		if (nullptr != this->decal)
		{
			AppStateManager::getSingletonPtr()->getDecalsModule()->destroyDecal(this->gameObjectPtr->getSceneManager(), this->decal);
			this->decal = nullptr;
		}
	}

	bool DecalComponent::connect(void)
	{
		GameObjectComponent::connect();

		return true;
	}

	bool DecalComponent::disconnect(void)
	{
		GameObjectComponent::disconnect();

		return true;
	}

	void DecalComponent::update(Ogre::Real dt, bool notSimulating)
	{
		
	}

	void DecalComponent::createDecal(void)
	{
		if (nullptr != this->decal)
		{
			return;
		}

		Ogre::SceneManager* sceneManager = this->gameObjectPtr->getSceneManager();
		Ogre::SceneNode* sceneNode = this->gameObjectPtr->getSceneNode();

		this->decal = AppStateManager::getSingletonPtr()->getDecalsModule()->createDecal(sceneManager, sceneNode);

		if (nullptr == this->decal)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DecalComponent] Could not create decal for game object: " + this->gameObjectPtr->getName());
			return;
		}

		// The GameObject owns an Ogre movable object now (do not destroy it from main thread).
		this->gameObjectPtr->setDoNotDestroyMovableObject(true);

		// Assign textures via reserved pools (Ogre-Next decals sample behaviour).
		AppStateManager::getSingletonPtr()->getDecalsModule()->setDecalTextures(sceneManager, this->decal, this->diffuseTextureName->getString(), this->normalTextureName->getString(), this->emissiveTextureName->getString());

		AppStateManager::getSingletonPtr()->getDecalsModule()->setIgnoreAlpha(this->decal, this->ignoreAlpha->getBool());
		AppStateManager::getSingletonPtr()->getDecalsModule()->setMetalness(this->decal, this->metalness->getReal());
		AppStateManager::getSingletonPtr()->getDecalsModule()->setRoughness(this->decal, this->roughness->getReal());
		AppStateManager::getSingletonPtr()->getDecalsModule()->setRectSize(this->decal, this->rectSize->getVector3());

		// Register movable object on render thread (keeps your original behaviour).
		GraphicsModule::RenderCommand renderCommand = [this]()
		{
			this->gameObjectPtr->init(this->decal);
		};
		NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "DecalComponent::createDecal::init");

		// Register after the component has been created
		AppStateManager::getSingletonPtr()->getGameObjectController()->registerGameObject(gameObjectPtr);
	}

	void DecalComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (DecalComponent::AttrDiffuseTextureName() == attribute->getName())
		{
			this->setDiffuseTextureName(attribute->getString());
		}
		else if (DecalComponent::AttrNormalTextureName() == attribute->getName())
		{
			this->setNormalTextureName(attribute->getString());
		}
		else if (DecalComponent::AttrEmissiveTextureName() == attribute->getName())
		{
			this->setEmissiveTextureName(attribute->getString());
		}
		else if (DecalComponent::AttrIgnoreAlpha() == attribute->getName())
		{
			this->setIgnoreAlpha(attribute->getBool());
		}
		else if (DecalComponent::AttrMetalness() == attribute->getName())
		{
			this->setMetalness(attribute->getReal());
		}
		else if (DecalComponent::AttrRoughness() == attribute->getName())
		{
			this->setRoughness(attribute->getReal());
		}
		else if (DecalComponent::AttrRectSize() == attribute->getName())
		{
			this->setRectSize(attribute->getVector3());
		}
	}

	void DecalComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "DiffuseTextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->diffuseTextureName->getString())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "NormalTextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->normalTextureName->getString())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "EmissiveTextureName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->emissiveTextureName->getString())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "IgnoreAlpha"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->ignoreAlpha->getBool())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Metalness"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->metalness->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Roughness"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->roughness->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RectSize"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->rectSize->getVector3())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String DecalComponent::getClassName(void) const
	{
		return "DecalComponent";
	}

	Ogre::String DecalComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void DecalComponent::setDiffuseTextureName(const Ogre::String& diffuseTextureName)
	{
		this->diffuseTextureName->setValue(diffuseTextureName);

		if (nullptr != this->decal)
		{
			AppStateManager::getSingletonPtr()->getDecalsModule()->setDecalTextures(this->gameObjectPtr->getSceneManager(), this->decal, this->diffuseTextureName->getString(), this->normalTextureName->getString(), this->emissiveTextureName->getString());
		}
	}

	Ogre::String DecalComponent::getDiffuseTextureName(void) const
	{
		return this->diffuseTextureName->getString();
	}
	
	void DecalComponent::setNormalTextureName(const Ogre::String& normalTextureName)
	{
		this->normalTextureName->setValue(normalTextureName);

		if (nullptr != this->decal)
		{
			AppStateManager::getSingletonPtr()->getDecalsModule()->setDecalTextures(this->gameObjectPtr->getSceneManager(), this->decal,
				this->diffuseTextureName->getString(),
				this->normalTextureName->getString(),
				this->emissiveTextureName->getString());
		}
	}

	Ogre::String DecalComponent::getNormalTextureName(void) const
	{
		return this->normalTextureName->getString();
	}
	
	void DecalComponent::setEmissiveTextureName(const Ogre::String& emissiveTextureName)
	{
		this->emissiveTextureName->setValue(emissiveTextureName);

		if (nullptr != this->decal)
		{
			AppStateManager::getSingletonPtr()->getDecalsModule()->setDecalTextures(this->gameObjectPtr->getSceneManager(), this->decal, this->diffuseTextureName->getString(),
				this->normalTextureName->getString(), this->emissiveTextureName->getString());
		}
	}

	Ogre::String DecalComponent::getEmissiveTextureName(void) const
	{
		return this->emissiveTextureName->getString();
	}
	
	void DecalComponent::setIgnoreAlpha(bool ignoreAlpha)
	{
		this->ignoreAlpha->setValue(ignoreAlpha);
		if (nullptr != this->decal)
		{
			AppStateManager::getSingletonPtr()->getDecalsModule()->setIgnoreAlpha(this->decal, ignoreAlpha);
		}
	}

	bool DecalComponent::getIgnoreAlpha(void) const
	{
		return this->ignoreAlpha->getBool();
	}

	void DecalComponent::setMetalness(Ogre::Real metalness)
	{
		this->metalness->setValue(metalness);
		if (nullptr != this->decal)
		{
			AppStateManager::getSingletonPtr()->getDecalsModule()->setMetalness(this->decal, metalness);
		}
	}

	Ogre::Real DecalComponent::getMetalness(void) const
	{
		return this->metalness->getReal();
	}

	void DecalComponent::setRoughness(Ogre::Real roughness)
	{
		this->roughness->setValue(roughness);
		if (nullptr != this->decal)
		{
			AppStateManager::getSingletonPtr()->getDecalsModule()->setRoughness(this->decal, roughness);
		}
	}

	Ogre::Real DecalComponent::getRoughness(void) const
	{
		return this->roughness->getReal();
	}

	void DecalComponent::setRectSize(const Ogre::Vector3& rectSizeAndDimensions)
	{
		this->rectSize->setValue(rectSizeAndDimensions);
		if (nullptr != this->decal)
		{
			AppStateManager::getSingletonPtr()->getDecalsModule()->setRectSize(this->decal, rectSizeAndDimensions);
		}
	}

	Ogre::Vector3 DecalComponent::getRectSize(void) const
	{
		return this->rectSize->getVector3();
	}

	// ------------------------------------------------------------
// DecalComponent Lua bindings (snipped)
// ------------------------------------------------------------
	DecalComponent* getDecalComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<DecalComponent>(gameObject->getComponentWithOccurrence<DecalComponent>(occurrenceIndex)).get();
	}

	DecalComponent* getDecalComponent(GameObject* gameObject)
	{
		return makeStrongPtr<DecalComponent>(gameObject->getComponent<DecalComponent>()).get();
	}

	DecalComponent* getDecalComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<DecalComponent>(gameObject->getComponentFromName<DecalComponent>(name)).get();
	}

	void DecalComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
		[
			class_<DecalComponent, GameObjectComponent>("DecalComponent")
			.def("setActivated", &DecalComponent::setActivated)
			.def("isActivated", &DecalComponent::isActivated)

			.def("setDiffuseTextureName", &DecalComponent::setDiffuseTextureName)
			.def("getDiffuseTextureName", &DecalComponent::getDiffuseTextureName)

			.def("setNormalTextureName", &DecalComponent::setNormalTextureName)
			.def("getNormalTextureName", &DecalComponent::getNormalTextureName)

			.def("setEmissiveTextureName", &DecalComponent::setEmissiveTextureName)
			.def("getEmissiveTextureName", &DecalComponent::getEmissiveTextureName)

			.def("setIgnoreAlpha", &DecalComponent::setIgnoreAlpha)
			.def("getIgnoreAlpha", &DecalComponent::getIgnoreAlpha)

			.def("setMetalness", &DecalComponent::setMetalness)
			.def("getMetalness", &DecalComponent::getMetalness)

			.def("setRoughness", &DecalComponent::setRoughness)
			.def("getRoughness", &DecalComponent::getRoughness)

			.def("setRectSize", &DecalComponent::setRectSize)
			.def("getRectSize", &DecalComponent::getRectSize)
		];

		LuaScriptApi::getInstance()->addClassToCollection("DecalComponent", "class inherits GameObjectComponent", DecalComponent::getStaticInfoText());

		LuaScriptApi::getInstance()->addClassToCollection("DecalComponent", "void setActivated(bool activated)",
			"Sets whether this component is active. If deactivated, the decal is not rendered.");

		LuaScriptApi::getInstance()->addClassToCollection("DecalComponent", "bool isActivated()",
			"Gets whether this component is active.");

		LuaScriptApi::getInstance()->addClassToCollection("DecalComponent", "void setDiffuseTextureName(String diffuseTextureName)",
			"Sets the diffuse decal texture name. Uses the DecalsModule texture pool (texture arrays) internally.");

		LuaScriptApi::getInstance()->addClassToCollection("DecalComponent", "String getDiffuseTextureName()",
			"Gets the diffuse decal texture name.");

		LuaScriptApi::getInstance()->addClassToCollection("DecalComponent", "void setNormalTextureName(String normalTextureName)",
			"Sets the normal decal texture name (recommended format in Ogre-Next pools: RG8_SNORM).");

		LuaScriptApi::getInstance()->addClassToCollection("DecalComponent", "String getNormalTextureName()",
			"Gets the normal decal texture name.");

		LuaScriptApi::getInstance()->addClassToCollection("DecalComponent", "void setEmissiveTextureName(String emissiveTextureName)",
			"Sets the emissive decal texture name. If empty, emissive will automatically use the diffuse texture for optimization.");

		LuaScriptApi::getInstance()->addClassToCollection("DecalComponent", "String getEmissiveTextureName()",
			"Gets the emissive decal texture name.");

		LuaScriptApi::getInstance()->addClassToCollection("DecalComponent", "void setIgnoreAlpha(bool ignoreAlpha)",
			"If true, ignores diffuse alpha. Useful for decals that should always apply fully regardless of alpha channel.");

		LuaScriptApi::getInstance()->addClassToCollection("DecalComponent", "bool getIgnoreAlpha()",
			"Gets whether diffuse alpha is ignored.");

		LuaScriptApi::getInstance()->addClassToCollection("DecalComponent", "void setMetalness(float metalness)",
			"Sets the metalness contribution for the decal (0..1).");

		LuaScriptApi::getInstance()->addClassToCollection("DecalComponent", "float getMetalness()",
			"Gets the metalness contribution.");

		LuaScriptApi::getInstance()->addClassToCollection("DecalComponent", "void setRoughness(float roughness)",
			"Sets the roughness contribution for the decal (0..1).");

		LuaScriptApi::getInstance()->addClassToCollection("DecalComponent", "float getRoughness()",
			"Gets the roughness contribution.");

		LuaScriptApi::getInstance()->addClassToCollection("DecalComponent", "void setRectSize(Vector3 rectSize)",
			"Sets the decal projector volume size (width/height/depth) in local space. Controls how far the decal affects surfaces.");

		LuaScriptApi::getInstance()->addClassToCollection("DecalComponent", "Vector3 getRectSize()",
			"Gets the decal projector volume size.");

		gameObjectClass.def("getDecalComponentFromName", &getDecalComponentFromName);
		gameObjectClass.def("getDecalComponent", (DecalComponent * (*)(GameObject*)) & getDecalComponent);
		// If its desired to create several of this components for one game object
		gameObjectClass.def("getDecalComponent2", (DecalComponent * (*)(GameObject*, unsigned int)) & getDecalComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "DecalComponent getDecalComponent2(unsigned int occurrenceIndex)",
			"Gets the component by the given occurence index, since a game object may have this component several times.");

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "DecalComponent getDecalComponent()",
			"Gets the component. This can be used if the game object has this component just once.");

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "DecalComponent getDecalComponentFromName(String name)",
			"Gets the component from name.");

		gameObjectControllerClass.def("castDecalComponent", &GameObjectController::cast<DecalComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "DecalComponent castDecalComponent(DecalComponent other)",
			"Casts an incoming type from function for lua auto completion.");
	}

}; // namespace end
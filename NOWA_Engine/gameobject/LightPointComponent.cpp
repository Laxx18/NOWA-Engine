#include "NOWAPrecompiled.h"
#include "LightPointComponent.h"
#include "GameObjectController.h"
#include "utilities/XMLConverter.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	LightPointComponent::LightPointComponent()
		: GameObjectComponent(),
		light(nullptr),
		dummyEntity(nullptr),
		diffuseColor(new Variant(LightPointComponent::AttrDiffuseColor(), Ogre::Vector3::UNIT_SCALE, this->attributes)),
		specularColor(new Variant(LightPointComponent::AttrSpecularColor(), Ogre::Vector3::UNIT_SCALE, this->attributes)),
		powerScale(new Variant(LightPointComponent::AttrPowerScale(), 3.14159f, this->attributes)),
		attenuationMode(new Variant(LightPointComponent::AttrAttenuationMode(), { Ogre::String("Radius"), Ogre::String("Range") }, this->attributes)),
		attenuationRange(new Variant(LightPointComponent::AttrAttenuationRange(), 23.0f, this->attributes)),
		attenuationConstant(new Variant(LightPointComponent::AttrAttenuationConstant(), 0.5f, this->attributes)),
		attenuationLinear(new Variant(LightPointComponent::AttrAttenuationLinear(), 0.0f, this->attributes)),
		attenuationQuadratic(new Variant(LightPointComponent::AttrAttenuationQuadratic(), 0.5f, this->attributes)),
		attenuationRadius(new Variant(LightPointComponent::AttrAttenuationRadius(), 10.0f, this->attributes)),
		attenuationLumThreshold(new Variant(LightPointComponent::AttrAttenuationLumThreshold(), 0.00192f, this->attributes)),
		castShadows(new Variant(LightPointComponent::AttrCastShadows(), true, this->attributes)),
		showDummyEntity(new Variant(LightPointComponent::AttrShowDummyEntity(), false, this->attributes))
	{
		this->diffuseColor->addUserData(GameObject::AttrActionColorDialog());
		this->specularColor->addUserData(GameObject::AttrActionColorDialog());
		this->attenuationMode->addUserData(GameObject::AttrActionNeedRefresh());
		this->attenuationRadius->setVisible(false);
		this->attenuationLumThreshold->setVisible(false);
	}

	LightPointComponent::~LightPointComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[LightPointComponent] Destructor light point component for game object: " + this->gameObjectPtr->getName());
		if (nullptr != this->light)
		{
			this->gameObjectPtr->getSceneNode()->detachObject(this->light);
			this->gameObjectPtr->getSceneManager()->destroyMovableObject(this->light);
			this->light = nullptr;
			this->dummyEntity = nullptr;
		}
	}

	bool LightPointComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Diffuse")
		{
			this->diffuseColor->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3::UNIT_SCALE));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Specular")
		{
			this->specularColor->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3::UNIT_SCALE));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "PowerScale")
		{
			this->powerScale->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AttenuationMode")
		{
			this->attenuationMode->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AttenuationRange")
		{
			this->attenuationRange->setValue(XMLConverter::getAttribReal(propertyElement, "data", 23.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AttenuationConstant")
		{
			this->attenuationConstant->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.5f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AttenuationLinear")
		{
			this->attenuationLinear->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AttenuationQuadratic")
		{
			this->attenuationQuadratic->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.5f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AttenuationRadius")
		{
			this->attenuationRadius->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AttenuationLumThreshold")
		{
			this->attenuationLumThreshold->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CastShadows")
		{
			this->castShadows->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ShowDummyEntity")
		{
			this->showDummyEntity->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	GameObjectCompPtr LightPointComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		LightPointCompPtr clonedCompPtr(boost::make_shared<LightPointComponent>());

		
		clonedCompPtr->setDiffuseColor(this->diffuseColor->getVector3());
		clonedCompPtr->setSpecularColor(this->specularColor->getVector3());
		clonedCompPtr->setPowerScale(this->powerScale->getReal());
		clonedCompPtr->setAttenuationRange(this->attenuationRange->getReal());
		clonedCompPtr->setAttenuationConstant(this->attenuationConstant->getReal());
		clonedCompPtr->setAttenuationLinear(this->attenuationLinear->getReal());
		clonedCompPtr->setAttenuationQuadratic(this->attenuationQuadratic->getReal());
		clonedCompPtr->setAttenuationRadius(this->attenuationRadius->getReal());
		clonedCompPtr->setAttenuationLumThreshold(this->attenuationLumThreshold->getReal());
		clonedCompPtr->setCastShadows(this->castShadows->getBool());
		clonedCompPtr->setShowDummyEntity(this->showDummyEntity->getBool());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool LightPointComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[LightPointComponent] Init light point component for game object: " + this->gameObjectPtr->getName());

		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);

		this->createLight();
		return true;
	}

	bool LightPointComponent::connect(void)
	{
		this->dummyEntity->setVisible(this->showDummyEntity->getBool());

		return true;
	}

	bool LightPointComponent::disconnect(void)
	{
		this->dummyEntity->setVisible(true);

		return true;
	}

	void LightPointComponent::update(Ogre::Real dt, bool notSimulating)
	{
		
	}

	void LightPointComponent::createLight(void)
	{
		if (nullptr == this->light)
		{
			this->light = this->gameObjectPtr->getSceneManager()->createLight();

			this->light->setType(Ogre::Light::LightTypes::LT_POINT);
			this->light->setCastShadows(true);
			this->light->setDiffuseColour(this->diffuseColor->getVector3().x, this->diffuseColor->getVector3().y, this->diffuseColor->getVector3().z);
			this->light->setSpecularColour(this->specularColor->getVector3().x, this->specularColor->getVector3().y, this->specularColor->getVector3().z);
			this->light->setPowerScale(this->powerScale->getReal());
			if ("Range" == this->attenuationMode->getListSelectedValue())
				this->light->setAttenuation(this->attenuationRange->getReal(), this->attenuationConstant->getReal(), this->attenuationLinear->getReal(), this->attenuationQuadratic->getReal());
			else
				this->light->setAttenuationBasedOnRadius(this->attenuationRadius->getReal(), this->attenuationLumThreshold->getReal());
			this->light->setCastShadows(this->castShadows->getBool());
			
			this->gameObjectPtr->getSceneNode()->attachObject(light);

#if 0
			// Optionally, create a visual representation of the light (e.g., a sphere)
			Ogre::v1::Entity* lightSphere = this->gameObjectPtr->getSceneManager()->createEntity(Ogre::SceneManager::PT_SPHERE);
			Ogre::SceneNode* sphereNode = this->gameObjectPtr->getSceneNode()->createChildSceneNode();
			sphereNode->attachObject(lightSphere);

			// Scale down the sphere to make it smaller
			sphereNode->setScale(0.01f, 0.01f, 0.01f);
#endif

			Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
			if (nullptr != entity)
			{
				entity->setCastShadows(false);
				// Borrow the entity from the game object
				this->dummyEntity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
			}
			else
			{
				Ogre::Item* item = this->gameObjectPtr->getMovableObject<Ogre::Item>();
				if (item != nullptr)
				{
					item->setCastShadows(false);
				}
			}
		}
	}

	void LightPointComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (LightPointComponent::AttrDiffuseColor() == attribute->getName())
		{
			this->setDiffuseColor(attribute->getVector3());
		}
		else if (LightPointComponent::AttrSpecularColor() == attribute->getName())
		{
			this->setSpecularColor(attribute->getVector3());
		}
		else if (LightPointComponent::AttrPowerScale() == attribute->getName())
		{
			this->setPowerScale(attribute->getReal());
		}
		else if (LightPointComponent::AttrAttenuationMode() == attribute->getName())
		{
			this->setAttenuationMode(attribute->getListSelectedValue());
		}
		else if (LightPointComponent::AttrAttenuationRange() == attribute->getName())
		{
			this->setAttenuationRange(attribute->getReal());
		}
		else if (LightPointComponent::AttrAttenuationConstant() == attribute->getName())
		{
			this->setAttenuationConstant(attribute->getReal());
		}
		else if (LightPointComponent::AttrAttenuationLinear() == attribute->getName())
		{
			this->setAttenuationLinear(attribute->getReal());
		}
		else if (LightPointComponent::AttrAttenuationQuadratic() == attribute->getName())
		{
			this->setAttenuationQuadratic(attribute->getReal());
		}
		else if (LightPointComponent::AttrAttenuationRadius() == attribute->getName())
		{
			this->setAttenuationRadius(attribute->getReal());
		}
		else if (LightPointComponent::AttrAttenuationLumThreshold() == attribute->getName())
		{
			this->setAttenuationLumThreshold(attribute->getReal());
		}
		else if (LightPointComponent::AttrCastShadows() == attribute->getName())
		{
			this->setCastShadows(attribute->getBool());
		}
	}

	void LightPointComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Diffuse"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->diffuseColor->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Specular"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->specularColor->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "PowerScale"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->powerScale->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AttenuationMode"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->attenuationMode->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AttenuationRange"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->attenuationRange->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AttenuationConstant"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->attenuationConstant->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AttenuationLinear"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->attenuationLinear->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AttenuationQuadratic"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->attenuationQuadratic->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AttenuationRadius"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->attenuationRadius->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AttenuationLumThreshold"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->attenuationLumThreshold->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "CastShadows"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->castShadows->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ShowDummyEntity"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->showDummyEntity->getBool())));
		propertiesXML->append_node(propertyXML);
	}

	void LightPointComponent::setActivated(bool activated)
	{
		if (nullptr != this->light)
		{
			this->light->setVisible(activated);
		}
	}

	bool LightPointComponent::isActivated(void) const
	{
		if (nullptr != this->light)
		{
			return this->light->isVisible();
		}
		return false;
	}

	Ogre::String LightPointComponent::getClassName(void) const
	{
		return "LightPointComponent";
	}

	Ogre::String LightPointComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void LightPointComponent::setDiffuseColor(const Ogre::Vector3& diffuseColor)
	{
		this->diffuseColor->setValue(diffuseColor);
		if (nullptr != this->light)
		{
			this->light->setDiffuseColour(this->diffuseColor->getVector3().x, this->diffuseColor->getVector3().y, this->diffuseColor->getVector3().z);
		}
	}

	Ogre::Vector3 LightPointComponent::getDiffuseColor(void) const
	{
		return this->diffuseColor->getVector3();
	}

	void LightPointComponent::setSpecularColor(const Ogre::Vector3& specularColor)
	{
		this->specularColor->setValue(specularColor);
		if (nullptr != this->light)
		{
			this->light->setSpecularColour(this->specularColor->getVector3().x, this->specularColor->getVector3().y, this->specularColor->getVector3().z);
		}
	}

	Ogre::Vector3 LightPointComponent::getSpecularColor(void) const
	{
		return this->specularColor->getVector3();
	}

	void LightPointComponent::setPowerScale(Ogre::Real powerScale)
	{
		this->powerScale->setValue(powerScale);
		if (nullptr != this->light)
		{
			this->light->setPowerScale(this->powerScale->getReal());
		}
	}

	Ogre::Real LightPointComponent::getPowerScale(void) const
	{
		return this->powerScale->getReal();
	}
	
	void LightPointComponent::setAttenuationMode(const Ogre::String& attenuationMode)
	{
		this->attenuationMode->setListSelectedValue(attenuationMode);
		
		if ("Range" == attenuationMode)
		{
			this->attenuationRange->setVisible(true);
			this->attenuationConstant->setVisible(true);
			this->attenuationLinear->setVisible(true);
			this->attenuationQuadratic->setVisible(true);
			this->attenuationRadius->setVisible(false);
			this->attenuationLumThreshold->setVisible(false);
			if (nullptr != this->light)
			{
				this->light->setAttenuation(this->attenuationRange->getReal(), this->attenuationConstant->getReal(), this->attenuationLinear->getReal(), this->attenuationQuadratic->getReal());
			}
		}
		else
		{
			this->attenuationRange->setVisible(false);
			this->attenuationConstant->setVisible(false);
			this->attenuationLinear->setVisible(false);
			this->attenuationQuadratic->setVisible(false);
			this->attenuationRadius->setVisible(true);
			this->attenuationLumThreshold->setVisible(true);
			if (nullptr != this->light)
			{
				this->light->setAttenuationBasedOnRadius(this->attenuationRadius->getReal(), this->attenuationLumThreshold->getReal());
			}
		}
	}

	Ogre::String LightPointComponent::getAttenuationMode(void) const
	{
		return this->attenuationMode->getListSelectedValue();
	}

	void LightPointComponent::setAttenuationRange(Ogre::Real attenuationRange)
	{
		this->attenuationRange->setValue(attenuationRange);
		if (nullptr != this->light)
		{
			this->light->setAttenuation(this->attenuationRange->getReal(), this->attenuationConstant->getReal(), this->attenuationLinear->getReal(), this->attenuationQuadratic->getReal());
		}
	}

	Ogre::Real LightPointComponent::getAttenuationRange(void) const
	{
		return this->attenuationRange->getReal();
	}

	void LightPointComponent::setAttenuationConstant(Ogre::Real attenuationConstant)
	{
		this->attenuationConstant->setValue(attenuationConstant);
		if (nullptr != this->light)
		{
			this->light->setAttenuation(this->attenuationRange->getReal(), this->attenuationConstant->getReal(), this->attenuationLinear->getReal(), this->attenuationQuadratic->getReal());
		}
	}

	Ogre::Real LightPointComponent::getAttenuationConstant(void) const
	{
		return this->attenuationConstant->getReal();
	}

	void LightPointComponent::setAttenuationLinear(Ogre::Real attenuationLinear)
	{
		this->attenuationLinear->setValue(attenuationLinear);
		if (nullptr != this->light)
		{
			this->light->setAttenuation(this->attenuationRange->getReal(), this->attenuationConstant->getReal(), this->attenuationLinear->getReal(), this->attenuationQuadratic->getReal());
		}
	}

	Ogre::Real LightPointComponent::getAttenuationLinear(void) const
	{
		return this->attenuationLinear->getReal();
	}

	void LightPointComponent::setAttenuationQuadratic(Ogre::Real attenuationQuadratic)
	{
		this->attenuationQuadratic->setValue(attenuationQuadratic);
		if (nullptr != this->light)
		{
			this->light->setAttenuation(this->attenuationRange->getReal(), this->attenuationConstant->getReal(), this->attenuationLinear->getReal(), this->attenuationQuadratic->getReal());
		}
	}

	Ogre::Real LightPointComponent::getAttenuationQuadratic(void) const
	{
		return this->attenuationQuadratic->getReal();
	}
	
	void LightPointComponent::setAttenuationRadius(Ogre::Real attenuationRadius)
	{
		this->attenuationRadius->setValue(attenuationRadius);
		if (nullptr != this->light)
		{
			this->light->setAttenuationBasedOnRadius(this->attenuationRadius->getReal(), this->attenuationLumThreshold->getReal());
		}
	}

	Ogre::Real LightPointComponent::getAttenuationRadius(void) const
	{
		return this->attenuationRadius->getReal();
	}
	
	void LightPointComponent::setAttenuationLumThreshold(Ogre::Real attenuationLumThreshold)
	{
		this->attenuationLumThreshold->setValue(attenuationLumThreshold);
		if (nullptr != this->light)
		{
			this->light->setAttenuationBasedOnRadius(this->attenuationRadius->getReal(), this->attenuationLumThreshold->getReal());
		}
	}

	Ogre::Real LightPointComponent::getAttenuationLumThreshold(void) const
	{
		return this->attenuationRadius->getReal();
	}

	void LightPointComponent::setCastShadows(bool castShadows)
	{
		this->castShadows->setValue(castShadows);
		if (nullptr != this->light)
		{
			this->light->setCastShadows(this->castShadows->getBool());
		}
	}

	bool LightPointComponent::getCastShadows(void) const
	{
		return this->castShadows->getBool();
	}

	void LightPointComponent::setShowDummyEntity(bool showDummyEntity)
	{
		this->showDummyEntity->setValue(showDummyEntity);
	}

	bool LightPointComponent::getShowDummyEntity(void) const
	{
		return 	this->showDummyEntity->getBool();
	}

	Ogre::Light* LightPointComponent::getOgreLight(void) const
	{
		return this->light;
	}

}; // namespace end
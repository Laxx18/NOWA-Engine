#include "NOWAPrecompiled.h"
#include "LightSpotComponent.h"
#include "GameObjectController.h"
#include "utilities/XMLConverter.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	LightSpotComponent::LightSpotComponent()
		: GameObjectComponent(),
		light(nullptr),
		dummyEntity(nullptr),
		diffuseColor(new Variant(LightSpotComponent::AttrDiffuseColor(), Ogre::Vector3::UNIT_SCALE, this->attributes)),
		specularColor(new Variant(LightSpotComponent::AttrSpecularColor(), Ogre::Vector3::UNIT_SCALE, this->attributes)),
		powerScale(new Variant(LightSpotComponent::AttrPowerScale(), 3.14159f, this->attributes)),
		attenuationMode(new Variant(LightSpotComponent::AttrAttenuationMode(), { Ogre::String("Radius"), Ogre::String("Range") }, this->attributes)),
		attenuationRange(new Variant(LightSpotComponent::AttrAttenuationRange(), 23.0f, this->attributes)),
		attenuationConstant(new Variant(LightSpotComponent::AttrAttenuationConstant(), 0.5f, this->attributes)),
		attenuationLinear(new Variant(LightSpotComponent::AttrAttenuationLinear(), 0.0f, this->attributes)),
		attenuationQuadratic(new Variant(LightSpotComponent::AttrAttenuationQuadratic(), 0.5f, this->attributes)),
		attenuationRadius(new Variant(LightSpotComponent::AttrAttenuationRadius(), 10.0f, this->attributes)),
		attenuationLumThreshold(new Variant(LightSpotComponent::AttrAttenuationLumThreshold(), 0.00192f, this->attributes)),
		castShadows(new Variant(LightSpotComponent::AttrCastShadows(), true, this->attributes)),
		size(new Variant(LightSpotComponent::AttrSize(), Ogre::Vector3(30.0f, 40.0f, 1.0f), this->attributes)),
		nearClipDistance(new Variant(LightSpotComponent::AttrNearClipDistance(), 0.1f, this->attributes)),
		showDummyEntity(new Variant(LightSpotComponent::AttrShowDummyEntity(), false, this->attributes))
	{
		this->diffuseColor->addUserData(GameObject::AttrActionColorDialog());
		this->specularColor->addUserData(GameObject::AttrActionColorDialog());
		this->attenuationMode->addUserData(GameObject::AttrActionNeedRefresh());
		this->attenuationRadius->setVisible(false);
		this->attenuationLumThreshold->setVisible(false);
		this->size->setDescription("Values: inner angle (degree), outer angle (degree), falloff (real). Sets the range of a spotlight, i.e. the angle of the inner and outer cones and the rate of falloff between them.");
		this->nearClipDistance->setDescription("Set the near clip plane distance to be used by spotlights that use light clipping, allowing you to render spots as if they start from further down their frustum.");
	}

	LightSpotComponent::~LightSpotComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[LightSpotComponent] Destructor light spot component for game object: " + this->gameObjectPtr->getName());
		if (nullptr != this->light)
		{
			ENQUEUE_RENDER_COMMAND_WAIT("LightSpotComponent::~LightSpotComponent",
			{
				this->gameObjectPtr->getSceneNode()->detachObject(this->light);
				this->gameObjectPtr->getSceneManager()->destroyMovableObject(this->light);
			});
			this->light = nullptr;
			this->dummyEntity = nullptr;
		}
	}

	bool LightSpotComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

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
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Size")
		{
			this->size->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "NearClipDistance")
		{
			this->nearClipDistance->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ShowDummyEntity")
		{
			this->showDummyEntity->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	GameObjectCompPtr LightSpotComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		LightSpotCompPtr clonedCompPtr(boost::make_shared<LightSpotComponent>());

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
		clonedCompPtr->setSize(this->size->getVector3());
		clonedCompPtr->setNearClipDistance(this->nearClipDistance->getReal());
		clonedCompPtr->setShowDummyEntity(this->showDummyEntity->getBool());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool LightSpotComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[LightSpotComponent] Init light spot component for game object: " + this->gameObjectPtr->getName());

		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);

		this->createLight();
		return true;
	}

	bool LightSpotComponent::connect(void)
	{
		if (nullptr != this->dummyEntity)
		{
			bool visible = this->showDummyEntity->getBool() && this->gameObjectPtr->isVisible();
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("LightSpotComponent::connect setVisible", _1(visible),
			{
				this->dummyEntity->setVisible(visible);
			});
		}

		return true;
	}

	bool LightSpotComponent::disconnect(void)
	{
		if (nullptr != this->dummyEntity)
		{
			bool visible = this->gameObjectPtr->isVisible();
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("LightSpotComponent::disconnect setVisible", _1(visible),
			{
				this->dummyEntity->setVisible(visible);
			});
		}

		return true;
	}

	void LightSpotComponent::update(Ogre::Real dt, bool notSimulating)
	{
		
	}

	void LightSpotComponent::createLight(void)
	{
		if (nullptr == this->light)
		{
			ENQUEUE_RENDER_COMMAND_WAIT("LightSpotComponent::createLight",
			{
				this->light = this->gameObjectPtr->getSceneManager()->createLight();

				this->light->setType(Ogre::Light::LightTypes::LT_SPOTLIGHT);
				this->light->setCastShadows(true);
				this->light->setDiffuseColour(this->diffuseColor->getVector3().x, this->diffuseColor->getVector3().y, this->diffuseColor->getVector3().z);
				this->light->setSpecularColour(this->specularColor->getVector3().x, this->specularColor->getVector3().y, this->specularColor->getVector3().z);
				this->light->setPowerScale(this->powerScale->getReal());

				if ("Range" == this->attenuationMode->getListSelectedValue())
					this->light->setAttenuation(this->attenuationRange->getReal(), this->attenuationConstant->getReal(), this->attenuationLinear->getReal(), this->attenuationQuadratic->getReal());
				else
					this->light->setAttenuationBasedOnRadius(this->attenuationRadius->getReal(), this->attenuationLumThreshold->getReal());

				this->light->setCastShadows(this->castShadows->getBool());
				this->setSize(this->size->getVector3());
				this->light->setSpotlightNearClipDistance(this->nearClipDistance->getReal());

				this->gameObjectPtr->getSceneNode()->attachObject(light);

				// this->light->setAffectParentNode(true);

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
			});
		}
	}

	void LightSpotComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (LightSpotComponent::AttrDiffuseColor() == attribute->getName())
		{
			this->setDiffuseColor(attribute->getVector3());
		}
		else if (LightSpotComponent::AttrSpecularColor() == attribute->getName())
		{
			this->setSpecularColor(attribute->getVector3());
		}
		else if (LightSpotComponent::AttrPowerScale() == attribute->getName())
		{
			this->setPowerScale(attribute->getReal());
		}
		else if (LightSpotComponent::AttrAttenuationMode() == attribute->getName())
		{
			this->setAttenuationMode(attribute->getListSelectedValue());
		}
		else if (LightSpotComponent::AttrAttenuationRange() == attribute->getName())
		{
			this->setAttenuationRange(attribute->getReal());
		}
		else if (LightSpotComponent::AttrAttenuationConstant() == attribute->getName())
		{
			this->setAttenuationConstant(attribute->getReal());
		}
		else if (LightSpotComponent::AttrAttenuationLinear() == attribute->getName())
		{
			this->setAttenuationLinear(attribute->getReal());
		}
		else if (LightSpotComponent::AttrAttenuationQuadratic() == attribute->getName())
		{
			this->setAttenuationQuadratic(attribute->getReal());
		}
		else if (LightSpotComponent::AttrAttenuationRadius() == attribute->getName())
		{
			this->setAttenuationRadius(attribute->getReal());
		}
		else if (LightSpotComponent::AttrAttenuationLumThreshold() == attribute->getName())
		{
			this->setAttenuationLumThreshold(attribute->getReal());
		}
		else if (LightSpotComponent::AttrCastShadows() == attribute->getName())
		{
			this->setCastShadows(attribute->getBool());
		}
		else if (LightSpotComponent::AttrShowDummyEntity() == attribute->getName())
		{
			this->setShowDummyEntity(attribute->getBool());
		}
		else if (LightSpotComponent::AttrSize() == attribute->getName())
		{
			this->setSize(attribute->getVector3());
		}
		else if (LightSpotComponent::AttrNearClipDistance() == attribute->getName())
		{
			this->setNearClipDistance(attribute->getReal());
		}
	}

	void LightSpotComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Size"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->size->getVector3())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "NearClipDistance"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->nearClipDistance->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ShowDummyEntity"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->showDummyEntity->getBool())));
		propertiesXML->append_node(propertyXML);
	}

	void LightSpotComponent::setActivated(bool activated)
	{
		if (nullptr != this->light)
		{
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("LightSpotComponent::setActivated", _1(activated),
			{
				this->light->setVisible(activated);
			});
		}
	}

	bool LightSpotComponent::isActivated(void) const
	{
		if (nullptr != this->light)
		{
			return this->light->isVisible();
		}
		return false;
	}

	Ogre::String LightSpotComponent::getClassName(void) const
	{
		return "LightSpotComponent";
	}

	Ogre::String LightSpotComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void LightSpotComponent::setDiffuseColor(const Ogre::Vector3& diffuseColor)
	{
		this->diffuseColor->setValue(diffuseColor);
		if (nullptr != this->light)
		{
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("LightSpotComponent::setDiffuseColor", _1(diffuseColor),
			{
				this->light->setDiffuseColour(diffuseColor.x, diffuseColor.y, diffuseColor.z);
			});
		}
	}

	Ogre::Vector3 LightSpotComponent::getDiffuseColor(void) const
	{
		return this->diffuseColor->getVector3();
	}

	void LightSpotComponent::setSpecularColor(const Ogre::Vector3& specularColor)
	{
		this->specularColor->setValue(specularColor);
		if (nullptr != this->light)
		{
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("LightSpotComponent::setSpecularColor", _1(specularColor),
			{
				this->light->setSpecularColour(specularColor.x, specularColor.y, specularColor.z);
			});
		}
	}

	Ogre::Vector3 LightSpotComponent::getSpecularColor(void) const
	{
		return this->specularColor->getVector3();
	}

	void LightSpotComponent::setPowerScale(Ogre::Real powerScale)
	{
		this->powerScale->setValue(powerScale);
		if (nullptr != this->light)
		{
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("LightSpotComponent::setPowerScale", _1(powerScale),
			{
				this->light->setPowerScale(powerScale);
			});
		}
	}

	Ogre::Real LightSpotComponent::getPowerScale(void) const
	{
		return this->powerScale->getReal();
	}
	
	void LightSpotComponent::setAttenuationMode(const Ogre::String& attenuationMode)
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
				ENQUEUE_RENDER_COMMAND_WAIT("LightSpotComponent::setAttenuationMode1",
				{
					this->light->setAttenuation(this->attenuationRange->getReal(), this->attenuationConstant->getReal(), this->attenuationLinear->getReal(), this->attenuationQuadratic->getReal());
				});
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
				ENQUEUE_RENDER_COMMAND_WAIT("LightSpotComponent::setAttenuationMode2",
				{
					this->light->setAttenuationBasedOnRadius(this->attenuationRadius->getReal(), this->attenuationLumThreshold->getReal());
				});
			}
		}
	}

	Ogre::String LightSpotComponent::getAttenuationMode(void) const
	{
		return this->attenuationMode->getListSelectedValue();
	}
	
	void LightSpotComponent::setAttenuationRange(Ogre::Real attenuationRange)
	{
		this->attenuationRange->setValue(attenuationRange);
		if (nullptr != this->light)
		{
			ENQUEUE_RENDER_COMMAND_WAIT("LightSpotComponent::setAttenuationRange",
			{
				this->light->setAttenuation(this->attenuationRange->getReal(), this->attenuationConstant->getReal(), this->attenuationLinear->getReal(), this->attenuationQuadratic->getReal());
			});
		}
	}

	Ogre::Real LightSpotComponent::getAttenuationRange(void) const
	{
		return this->attenuationRange->getReal();
	}

	void LightSpotComponent::setAttenuationConstant(Ogre::Real attenuationConstant)
	{
		this->attenuationConstant->setValue(attenuationConstant);
		if (nullptr != this->light)
		{
			ENQUEUE_RENDER_COMMAND_WAIT("LightSpotComponent::setAttenuationConstant",
			{
				this->light->setAttenuation(this->attenuationRange->getReal(), this->attenuationConstant->getReal(), this->attenuationLinear->getReal(), this->attenuationQuadratic->getReal());
			});
		}
	}

	Ogre::Real LightSpotComponent::getAttenuationConstant(void) const
	{
		return this->attenuationConstant->getReal();
	}

	void LightSpotComponent::setAttenuationLinear(Ogre::Real attenuationLinear)
	{
		this->attenuationLinear->setValue(attenuationLinear);
		if (nullptr != this->light)
		{
			ENQUEUE_RENDER_COMMAND_WAIT("LightSpotComponent::setAttenuationLinear",
			{
				this->light->setAttenuation(this->attenuationRange->getReal(), this->attenuationConstant->getReal(), this->attenuationLinear->getReal(), this->attenuationQuadratic->getReal());
			});
		}
	}

	Ogre::Real LightSpotComponent::getAttenuationLinear(void) const
	{
		return this->attenuationLinear->getReal();
	}

	void LightSpotComponent::setAttenuationQuadratic(Ogre::Real attenuationQuadratic)
	{
		this->attenuationQuadratic->setValue(attenuationQuadratic);
		if (nullptr != this->light)
		{
			ENQUEUE_RENDER_COMMAND_WAIT("LightSpotComponent::setAttenuationQuadratic",
			{
				this->light->setAttenuation(this->attenuationRange->getReal(), this->attenuationConstant->getReal(), this->attenuationLinear->getReal(), this->attenuationQuadratic->getReal());
			});
		}
	}

	Ogre::Real LightSpotComponent::getAttenuationQuadratic(void) const
	{
		return this->attenuationQuadratic->getReal();
	}
	
	void LightSpotComponent::setAttenuationRadius(Ogre::Real attenuationRadius)
	{
		this->attenuationRadius->setValue(attenuationRadius);
		if (nullptr != this->light)
		{
			ENQUEUE_RENDER_COMMAND_WAIT("LightSpotComponent::setAttenuationRadius",
			{
				this->light->setAttenuationBasedOnRadius(this->attenuationRadius->getReal(), this->attenuationLumThreshold->getReal());
			});
		}
	}

	Ogre::Real LightSpotComponent::getAttenuationRadius(void) const
	{
		return this->attenuationRadius->getReal();
	}
	
	void LightSpotComponent::setAttenuationLumThreshold(Ogre::Real attenuationLumThreshold)
	{
		this->attenuationLumThreshold->setValue(attenuationLumThreshold);
		if (nullptr != this->light)
		{
			ENQUEUE_RENDER_COMMAND_WAIT("LightSpotComponent::setAttenuationLumThreshold",
			{
				this->light->setAttenuationBasedOnRadius(this->attenuationRadius->getReal(), this->attenuationLumThreshold->getReal());
			});
		}
	}

	Ogre::Real LightSpotComponent::getAttenuationLumThreshold(void) const
	{
		return this->attenuationRadius->getReal();
	}

	void LightSpotComponent::setCastShadows(bool castShadows)
	{
		this->castShadows->setValue(castShadows);
		if (nullptr != this->light)
		{
			ENQUEUE_RENDER_COMMAND_WAIT("LightSpotComponent::setCastShadows",
			{
				this->light->setCastShadows(this->castShadows->getBool());
			});
		}
	}

	bool LightSpotComponent::getCastShadows(void) const
	{
		return this->castShadows->getBool();
	}
	
	void LightSpotComponent::setSize(const Ogre::Vector3& size)
	{
		this->size->setValue(size);
		if (nullptr != this->light)
		{
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("LightSpotComponent::setSize", _1(size),
			{
				Ogre::Degree innerAngle(size.x);
				Ogre::Degree outerAngle(size.y);
				this->light->setSpotlightInnerAngle(Ogre::Radian(innerAngle.valueRadians()));
				this->light->setSpotlightOuterAngle(Ogre::Radian(outerAngle.valueRadians()));
				this->light->setSpotlightFalloff(size.z);
			});
		}
	}
	
	Ogre::Vector3 LightSpotComponent::getSize(void) const
	{
		return this->size->getVector3();
	}
	
	void LightSpotComponent::setNearClipDistance(Ogre::Real nearClipDistance)
	{
		if (nearClipDistance < 0.0f)
			nearClipDistance = 0.001f;
		this->nearClipDistance->setValue(nearClipDistance);
		if (nullptr != this->light)
		{
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("LightSpotComponent::setNearClipDistance", _1(nearClipDistance),
			{
				this->light->setSpotlightNearClipDistance(nearClipDistance);
			});
		}
	}
	
	Ogre::Real LightSpotComponent::LightSpotComponent::getNearClipDistance(void) const
	{
		return this->nearClipDistance->getReal();
	}
	
	void LightSpotComponent::setShowDummyEntity(bool showDummyEntity)
	{
		this->showDummyEntity->setValue(showDummyEntity);
	}

	bool LightSpotComponent::getShowDummyEntity(void) const
	{
		return 	this->showDummyEntity->getBool();
	}

	Ogre::Light* LightSpotComponent::getOgreLight(void) const
	{
		return this->light;
	}

}; // namespace end
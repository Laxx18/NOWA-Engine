#include "NOWAPrecompiled.h"
#include "LightDirectionalComponent.h"
#include "GameObjectController.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	LightDirectionalComponent::LightDirectionalComponent()
		: GameObjectComponent(),
		light(nullptr),
		dummyEntity(nullptr),
		oldOrientation(Ogre::Quaternion::IDENTITY),
		transformUpdateTimer(0.0f),
		diffuseColor(new Variant(LightDirectionalComponent::AttrDiffuseColor(), Ogre::Vector3::UNIT_SCALE, this->attributes)),
		specularColor(new Variant(LightDirectionalComponent::AttrSpecularColor(), Ogre::Vector3::UNIT_SCALE, this->attributes)),
		powerScale(new Variant(LightDirectionalComponent::AttrPowerScale(), 3.14159f, this->attributes)),
		direction(new Variant(LightDirectionalComponent::AttrDirection(), Ogre::Vector3(-1.0f, -1.0f, -1.0f), this->attributes)),
		affectParentNode(new Variant(LightDirectionalComponent::AttrAffectParentNode(), true, this->attributes)),
		attenuationRadius(new Variant(LightDirectionalComponent::AttrAttenuationRadius(), 10.0f, this->attributes)),
		attenuationLumThreshold(new Variant(LightDirectionalComponent::AttrAttenuationLumThreshold(), 0.00192f, this->attributes)),
		castShadows(new Variant(LightDirectionalComponent::AttrCastShadows(), true, this->attributes)),
		showDummyEntity(new Variant(LightDirectionalComponent::AttrShowDummyEntity(), false, this->attributes))
	{
		this->diffuseColor->addUserData(GameObject::AttrActionColorDialog());
		this->specularColor->addUserData(GameObject::AttrActionColorDialog());
		this->direction->setVisible(false);
	}

	LightDirectionalComponent::~LightDirectionalComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[LightDirectionalComponent] Destructor light directional component for game object: " + this->gameObjectPtr->getName());
		if (nullptr != this->light)
		{
			ENQUEUE_RENDER_COMMAND_WAIT("LightDirectionalComponent::~LightDirectionalComponent",
			{
				this->gameObjectPtr->getSceneNode()->detachObject(this->light);
				this->gameObjectPtr->getSceneManager()->destroyMovableObject(this->light);
			});
			this->light = nullptr;
			this->dummyEntity = nullptr;
		}
	}

	bool LightDirectionalComponent::init(rapidxml::xml_node<>*& propertyElement)
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
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Direction")
		{
			this->direction->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3::ZERO));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AffectParentNode")
		{
			this->affectParentNode->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
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

	GameObjectCompPtr LightDirectionalComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		LightDirectionalCompPtr clonedCompPtr(boost::make_shared<LightDirectionalComponent>());

		
		clonedCompPtr->setDiffuseColor(this->diffuseColor->getVector3());
		clonedCompPtr->setSpecularColor(this->specularColor->getVector3());
		clonedCompPtr->setPowerScale(this->powerScale->getReal());
		// clonedCompPtr->setDirection(this->direction->getVector3());
		clonedCompPtr->setAffectParentNode(this->affectParentNode->getBool());
		clonedCompPtr->setAttenuationRadius(this->attenuationRadius->getReal());
		clonedCompPtr->setAttenuationLumThreshold(this->attenuationLumThreshold->getReal());
		clonedCompPtr->setCastShadows(this->castShadows->getBool());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		clonedCompPtr->setShowDummyEntity(this->showDummyEntity->getBool());

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool LightDirectionalComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[LightDirectionalComponent] Init light directional component for game object: " + this->gameObjectPtr->getName());

		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);

		this->createLight();
		return true;
	}

	bool LightDirectionalComponent::connect(void)
	{
		if (nullptr != this->dummyEntity)
		{
			bool visible = this->showDummyEntity->getBool() && this->gameObjectPtr->isVisible();
			ENQUEUE_RENDER_COMMAND_MULTI("LightDirectionalComponent::connect setVisible", _1(visible),
			{
				this->dummyEntity->setVisible(visible);
			});
		}

		return true;
	}

	bool LightDirectionalComponent::disconnect(void)
	{
		if (nullptr != this->dummyEntity)
		{
			bool visible = this->gameObjectPtr->isVisible();
			this->dummyEntity->setVisible(visible);
		}

		return true;
	}

	void LightDirectionalComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
			this->transformUpdateTimer += dt;
			// Check only each second
			if (this->transformUpdateTimer >= 2.0f)
			{
				this->transformUpdateTimer = 0.0f;

				// Modify orientation if difference larger than epsilon
				if (false == this->gameObjectPtr->getOrientation().equals(this->oldOrientation, Ogre::Degree(0.5f)))
				{
					if (nullptr != this->light)
					{
						ENQUEUE_RENDER_COMMAND("LightDirectionalComponent::update",
						{
							// Actualize the ambient light, when the direction changed
							this->gameObjectPtr->getSceneManager()->setAmbientLight(this->gameObjectPtr->getSceneManager()->getAmbientLightUpperHemisphere(),
								this->gameObjectPtr->getSceneManager()->getAmbientLightLowerHemisphere(), -this->light->getDirection()/* * Ogre::Vector3::UNIT_Y * 0.2f*/);
						});
					}
				}

				this->oldOrientation = this->gameObjectPtr->getOrientation();
			}
		}
	}

	void LightDirectionalComponent::createLight(void)
	{
		if (nullptr == this->light)
		{
			ENQUEUE_RENDER_COMMAND_WAIT("LightDirectionalComponent::createLight",
			{
				this->light = this->gameObjectPtr->getSceneManager()->createLight();

				this->light->setType(Ogre::Light::LightTypes::LT_DIRECTIONAL);
				this->light->setCastShadows(true);
				this->light->setDiffuseColour(this->diffuseColor->getVector3().x, this->diffuseColor->getVector3().y, this->diffuseColor->getVector3().z);
				this->light->setSpecularColour(this->specularColor->getVector3().x, this->specularColor->getVector3().y, this->specularColor->getVector3().z);
				this->light->setPowerScale(this->powerScale->getReal());
				this->light->setAffectParentNode(this->affectParentNode->getBool());
				this->light->setAttenuationBasedOnRadius(this->attenuationRadius->getReal(), this->attenuationLumThreshold->getReal());
				// this->light->setAttenuation(this->attenuationRange->getReal(), this->attenuationConstant->getReal(), this->attenuationLinear->getReal(), this->attenuationQuadratic->getReal());
				this->light->setCastShadows(this->castShadows->getBool());
				// light->setDirection(this->gameObjectPtr->getSceneNode()->getOrientation() * Ogre::Vector3::NEGATIVE_UNIT_Z);
				this->gameObjectPtr->getSceneNode()->attachObject(this->light);

				Ogre::v1::Entity * entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
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
				this->setDirection(this->direction->getVector3());
			});
		}
	}

	void LightDirectionalComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (LightDirectionalComponent::AttrDiffuseColor() == attribute->getName())
		{
			this->setDiffuseColor(attribute->getVector3());
		}
		else if (LightDirectionalComponent::AttrSpecularColor() == attribute->getName())
		{
			this->setSpecularColor(attribute->getVector3());
		}
		else if (LightDirectionalComponent::AttrPowerScale() == attribute->getName())
		{
			this->setPowerScale(attribute->getReal());
		}
		else if (LightDirectionalComponent::AttrDirection() == attribute->getName())
		{
			this->setDirection(attribute->getVector3());
		}
		else if (LightDirectionalComponent::AttrAffectParentNode() == attribute->getName())
		{
			this->setAffectParentNode(attribute->getBool());
		}
		else if (LightDirectionalComponent::AttrAttenuationRadius() == attribute->getName())
		{
			this->setAttenuationRadius(attribute->getReal());
		}
		else if (LightDirectionalComponent::AttrAttenuationLumThreshold() == attribute->getName())
		{
			this->setAttenuationLumThreshold(attribute->getReal());
		}
		else if (LightDirectionalComponent::AttrCastShadows() == attribute->getName())
		{
			this->setCastShadows(attribute->getBool());
		}
	}

	void LightDirectionalComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Direction"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->direction->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AffectParentNode"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->affectParentNode->getBool())));
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

	void LightDirectionalComponent::setActivated(bool activated)
	{
		if (nullptr != this->light)
		{
			ENQUEUE_RENDER_COMMAND_MULTI("LightDirectionalComponent::setActivated", _1(activated),
			{
				this->light->setVisible(activated);
			});
		}
	}

	bool LightDirectionalComponent::isActivated(void) const
	{
		if (nullptr != this->light)
		{
			return this->light->isVisible();
		}
		return false;
	}

	Ogre::String LightDirectionalComponent::getClassName(void) const
	{
		return "LightDirectionalComponent";
	}

	Ogre::String LightDirectionalComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void LightDirectionalComponent::setDiffuseColor(const Ogre::Vector3& diffuseColor)
	{
		this->diffuseColor->setValue(diffuseColor);
		if (nullptr != this->light)
		{
			ENQUEUE_RENDER_COMMAND_MULTI("LightDirectionalComponent::setDiffuseColor", _1(diffuseColor),
			{
				this->light->setDiffuseColour(this->diffuseColor->getVector3().x, this->diffuseColor->getVector3().y, this->diffuseColor->getVector3().z);
			});
		}
	}

	Ogre::Vector3 LightDirectionalComponent::getDiffuseColor(void) const
	{
		return this->diffuseColor->getVector3();
	}

	void LightDirectionalComponent::setSpecularColor(const Ogre::Vector3& specularColor)
	{
		this->specularColor->setValue(specularColor);
		if (nullptr != this->light)
		{
			ENQUEUE_RENDER_COMMAND_MULTI("LightDirectionalComponent::setSpecularColor", _1(specularColor),
			{
				this->light->setSpecularColour(this->specularColor->getVector3().x, this->specularColor->getVector3().y, this->specularColor->getVector3().z);
			});
		}
	}

	Ogre::Vector3 LightDirectionalComponent::getSpecularColor(void) const
	{
		return this->specularColor->getVector3();
	}

	void LightDirectionalComponent::setPowerScale(Ogre::Real powerScale)
	{
		this->powerScale->setValue(powerScale);
		if (nullptr != this->light)
		{
			ENQUEUE_RENDER_COMMAND_MULTI("LightDirectionalComponent::setPowerScale", _1(powerScale),
			{
				this->light->setPowerScale(powerScale);
			});
		}
	}

	Ogre::Real LightDirectionalComponent::getPowerScale(void) const
	{
		return this->powerScale->getReal();
	}

	void LightDirectionalComponent::setDirection(const Ogre::Vector3& direction)
	{
		this->direction->setValue(direction);
		if (nullptr != this->light)
		{
			ENQUEUE_RENDER_COMMAND_MULTI("LightDirectionalComponent::setDirection", _1(direction),
			{
				this->light->setDirection(this->direction->getVector3());
				// Actualize the ambient light, when the direction changed
				this->gameObjectPtr->getSceneManager()->setAmbientLight(this->gameObjectPtr->getSceneManager()->getAmbientLightUpperHemisphere(),
					this->gameObjectPtr->getSceneManager()->getAmbientLightLowerHemisphere(), -this->light->getDirection()/* * Ogre::Vector3::UNIT_Y * 0.2f*/);
			});
		}
	}

	Ogre::Vector3 LightDirectionalComponent::getDirection(void) const
	{
		return this->direction->getVector3();
	}

	void LightDirectionalComponent::setAffectParentNode(bool affectParentNode)
	{
		this->affectParentNode->setValue(affectParentNode);
		if (nullptr != this->light)
		{
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("LightDirectionalComponent::setAffectParentNode", _1(affectParentNode),
			{
				this->light->setAffectParentNode(this->affectParentNode->getBool());
			});
		}
	}

	bool LightDirectionalComponent::getAffectParentNode(void) const
	{
		return this->affectParentNode->getBool();
	}
	
	void LightDirectionalComponent::setAttenuationRadius(Ogre::Real attenuationRadius)
	{
		this->attenuationRadius->setValue(attenuationRadius);
		if (nullptr != this->light)
		{
			ENQUEUE_RENDER_COMMAND_MULTI("LightDirectionalComponent::setAttenuationRadius", _1(attenuationRadius),
			{
				this->light->setAttenuationBasedOnRadius(this->attenuationRadius->getReal(), this->attenuationLumThreshold->getReal());
			});
		}
	}

	Ogre::Real LightDirectionalComponent::getAttenuationRadius(void) const
	{
		return this->attenuationRadius->getReal();
	}
	
	void LightDirectionalComponent::setAttenuationLumThreshold(Ogre::Real attenuationLumThreshold)
	{
		this->attenuationLumThreshold->setValue(attenuationLumThreshold);
		if (nullptr != this->light)
		{
			ENQUEUE_RENDER_COMMAND_MULTI("LightDirectionalComponent::setAttenuationLumThreshold", _1(attenuationLumThreshold),
			{
				this->light->setAttenuationBasedOnRadius(this->attenuationRadius->getReal(), this->attenuationLumThreshold->getReal());
			});
		}
	}

	Ogre::Real LightDirectionalComponent::getAttenuationLumThreshold(void) const
	{
		return this->attenuationRadius->getReal();
	}

	void LightDirectionalComponent::setCastShadows(bool castShadows)
	{
		this->castShadows->setValue(castShadows);
		if (nullptr != this->light)
		{
			ENQUEUE_RENDER_COMMAND_MULTI("LightDirectionalComponent::setCastShadows", _1(castShadows),
			{
				this->light->setCastShadows(castShadows);
			});
		}
	}

	bool LightDirectionalComponent::getCastShadows(void) const
	{
		return this->castShadows->getBool();
	}

	void LightDirectionalComponent::setShowDummyEntity(bool showDummyEntity)
	{
		this->showDummyEntity->setValue(showDummyEntity);
	}

	bool LightDirectionalComponent::getShowDummyEntity(void) const
	{
		return 	this->showDummyEntity->getBool();
	}

	Ogre::Light* LightDirectionalComponent::getOgreLight(void) const
	{
		return this->light;
	}

}; // namespace end
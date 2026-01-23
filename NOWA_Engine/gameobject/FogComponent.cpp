#include "NOWAPrecompiled.h"
#include "FogComponent.h"
#include "GameObjectController.h"
#include "main/Core.h"
#include "shader/HlmsFog.h"
#include "utilities/XMLConverter.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	FogComponent::FogComponent()
		: GameObjectComponent(),
		fogListener(nullptr),
		activated(new Variant(FogComponent::AttrActivated(), true, this->attributes)),
		fogMode(new Variant(FogComponent::AttrFogMode(), { Ogre::String("Linear"), Ogre::String("Exponential"), Ogre::String("Exponential 2") }, this->attributes)),
		color(new Variant(FogComponent::AttrColor(), Ogre::Vector3::UNIT_SCALE, this->attributes)),
		expDensity(new Variant(FogComponent::AttrExpDensity(), 0.001f, this->attributes)),
		linearStart(new Variant(FogComponent::AttrLinearStart(), 10.0f, this->attributes)),
		linearEnd(new Variant(FogComponent::AttrLinearEnd(), 50.0f, this->attributes))
	{
		this->color->addUserData(GameObject::AttrActionColorDialog());
		this->fogMode->addUserData(GameObject::AttrActionNeedRefresh());
		this->expDensity->setConstraints(0.001f, 1.0f);
		this->linearStart->setConstraints(0.0f, 1000.0f);
		this->linearEnd->setConstraints(1.0f, 2000.0f);
	}

	FogComponent::~FogComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[FogComponent] Destructor fog component for game object: " + this->gameObjectPtr->getName());
	}

	bool FogComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FogMode")
		{
			this->fogMode->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Color")
		{
			this->color->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ExpDensity")
		{
			this->expDensity->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "LinearStart")
		{
			this->linearStart->setValue(XMLConverter::getAttribReal(propertyElement, "data", 23.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "LinearEnd")
		{
			this->linearEnd->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.5f));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	GameObjectCompPtr FogComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		return GameObjectCompPtr();
	}

	bool FogComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[FogComponent] Init fog component for game object: " + this->gameObjectPtr->getName());

		if (nullptr == this->fogListener)
		{
			this->fogListener = new HlmsFogListener();
			Core::getSingletonPtr()->getBaseListenerContainer()->addConcreteListener(this->fogListener);
		}

		this->createFog();
		return true;
	}

	void FogComponent::update(Ogre::Real dt, bool notSimulating)
	{

	}

	void FogComponent::createFog(void)
	{
		Ogre::FogMode fogMode = Ogre::FOG_NONE;

		if (true == this->activated->getBool())
		{
			if (nullptr == this->fogListener)
			{
				Core::getSingletonPtr()->setupHlms(true);

				this->fogListener = new HlmsFogListener();
				Core::getSingletonPtr()->getBaseListenerContainer()->addConcreteListener(this->fogListener);
			}
		}

		if ("Linear" == this->fogMode->getListSelectedValue())
		{
			fogMode = Ogre::FOG_LINEAR;
		}
		else if ("Exponential" == this->fogMode->getListSelectedValue())
		{
			fogMode = Ogre::FOG_EXP;
		}
		else if ("Exponential 2" == this->fogMode->getListSelectedValue())
		{
			fogMode = Ogre::FOG_EXP2;
		}

		Ogre::ColourValue colorValue = Ogre::ColourValue(this->color->getVector3().x, this->color->getVector3().y, this->color->getVector3().z, 1.0f);

		this->gameObjectPtr->getSceneManager()->setFog(fogMode, colorValue, this->expDensity->getReal(), this->linearStart->getReal(), this->linearEnd->getReal());

		if (false == this->activated->getBool())
		{
			this->gameObjectPtr->getSceneManager()->setFog(Ogre::FOG_NONE);
			Core::getSingletonPtr()->getBaseListenerContainer()->removeConcreteListener(this->fogListener);
			this->fogListener = nullptr;

			Core::getSingletonPtr()->setupHlms(false);
		}
	}

	void FogComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (FogComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (FogComponent::AttrFogMode() == attribute->getName())
		{
			this->setFogMode(attribute->getListSelectedValue());
		}
		else if (FogComponent::AttrExpDensity() == attribute->getName())
		{
			this->setExpDensity(attribute->getReal());
		}
		else if (FogComponent::AttrLinearStart() == attribute->getName())
		{
			this->setLinearStart(attribute->getReal());
		}
		else if (FogComponent::AttrLinearEnd() == attribute->getName())
		{
			this->setLinearEnd(attribute->getReal());
		}
	}

	void FogComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "FogMode"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->fogMode->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Color"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->color->getVector3())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ExpDensity"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->expDensity->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "LinearStart"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->linearStart->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "LinearEnd"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->linearEnd->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	void FogComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
		this->createFog();
	}

	bool FogComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void FogComponent::setFogMode(const Ogre::String& fogMode)
	{
		this->fogMode->setListSelectedValue(fogMode);
		this->createFog();
	}

	Ogre::String FogComponent::getFogMode(void) const
	{
		return this->fogMode->getListSelectedValue();
	}

	void FogComponent::setColor(const Ogre::Vector3& color)
	{
		this->color->setValue(color);
		this->createFog();
	}

	Ogre::Vector3 FogComponent::getColor(void) const
	{
		return this->color->getVector3();
	}

	void FogComponent::setExpDensity(Ogre::Real expDensity)
	{
		this->expDensity->setValue(expDensity);
		this->createFog();
	}

	Ogre::Real FogComponent::getExpDensity(void) const
	{
		return this->expDensity->getReal();
	}

	void FogComponent::setLinearStart(Ogre::Real linearStart)
	{
		this->linearStart->setValue(linearStart);
		this->createFog();
	}

	Ogre::Real FogComponent::getLinearStart(void) const
	{
		return this->linearStart->getReal();
	}

	void FogComponent::setLinearEnd(Ogre::Real linearEnd)
	{
		this->linearEnd->setValue(linearEnd);
		this->createFog();
	}

	Ogre::Real FogComponent::getLinearEnd(void) const
	{
		return this->linearEnd->getReal();
	}

	Ogre::String FogComponent::getClassName(void) const
	{
		return "FogComponent";
	}

	Ogre::String FogComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

}; // namespace end
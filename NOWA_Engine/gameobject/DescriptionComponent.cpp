#include "NOWAPrecompiled.h"
#include "DescriptionComponent.h"
#include "utilities/XMLConverter.h"
#include "GameObjectController.h"

#include "../res/resource.h"
#include <windows.h>
#include <shellapi.h>

namespace
{
	Ogre::String encodeNewlines(const Ogre::String& text)
	{
		Ogre::String encodedText = text;
		size_t pos = 0;
		while ((pos = encodedText.find('\n', pos)) != Ogre::String::npos)
		{
			encodedText.replace(pos, 1, "&#10;");
			pos += 5;  // Move past the replacement
		}
		return encodedText;
	}

	Ogre::String decodeNewlines(const Ogre::String& text)
	{
		Ogre::String decodedText = text;
		size_t pos = 0;
		while ((pos = decodedText.find("&#10;", pos)) != Ogre::String::npos)
		{
			decodedText.replace(pos, 5, "\n");
			pos += 1;  // Move past the replacement
		}
		return decodedText;
	}

}

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	DescriptionComponent::DescriptionComponent()
		: GameObjectComponent(),
		description(new Variant(DescriptionComponent::AttrDescription(), Ogre::String(), this->attributes))

	{
		this->description->setValue("<p float='left'><img width='260' height='159'>pic_NOWA</img> </p><br/>"
									"<p align='center'><color value='#FFFFFF'><url value='" + Ogre::String(NOWA_COMPANYDOMAIN_STR) + "'>" + Ogre::String(NOWA_COMPANYDOMAIN_STR) + "</url></color></p><br/><br/>"
									"<p><color value='#1D6EA7'>" + "No description." + "</color></p>");
		this->description->addUserData(GameObject::AttrActionMultiLine());
	}

	DescriptionComponent::~DescriptionComponent(void)
	{
		
	}

	bool DescriptionComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == DescriptionComponent::AttrDescription())
		{
			Ogre::String descriptionText = XMLConverter::getAttrib(propertyElement, "data");
			this->description->setValue(decodeNewlines(descriptionText));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	GameObjectCompPtr DescriptionComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		return nullptr;
	}

	bool DescriptionComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DescriptionComponent] Init component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool DescriptionComponent::connect(void)
	{
		return true;
	}

	bool DescriptionComponent::disconnect(void)
	{
		
		return true;
	}

	bool DescriptionComponent::onCloned(void)
	{
		
		return true;
	}

	void DescriptionComponent::onRemoveComponent(void)
	{
		
	}
	
	void DescriptionComponent::onOtherComponentRemoved(unsigned int index)
	{
		
	}
	
	void DescriptionComponent::onOtherComponentAdded(unsigned int index)
	{
		
	}
	
	void DescriptionComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
			// Do something
		}
	}

	void DescriptionComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (DescriptionComponent::AttrDescription() == attribute->getName())
		{
			this->setDescription(attribute->getString());
		}
	}

	void DescriptionComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		GameObjectComponent::writeXML(propertiesXML, doc);

		Ogre::String descriptionText = encodeNewlines(this->description->getString());

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Description"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, descriptionText)));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String DescriptionComponent::getClassName(void) const
	{
		return "DescriptionComponent";
	}

	Ogre::String DescriptionComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void DescriptionComponent::setDescription(const Ogre::String& description)
	{
		this->description->setValue(description);
	}

	Ogre::String DescriptionComponent::getDescription(void) const
	{
		return this->description->getString();
	}

	bool DescriptionComponent::canStaticAddComponent(GameObject* gameObject)
	{
		// Can only be added once
		auto descriptionCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<DescriptionComponent>());
		if (nullptr != descriptionCompPtr)
		{
			return false;
		}

		if(descriptionCompPtr != nullptr && descriptionCompPtr->getOwner()->getId() != GameObjectController::MAIN_GAMEOBJECT_ID)
		{
			return false;
		}

		return true;
	}

}; //namespace end
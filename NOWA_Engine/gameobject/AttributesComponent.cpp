#include "NOWAPrecompiled.h"
#include "AttributesComponent.h"
#include "utilities/XMLConverter.h"
#include "main/Core.h"
#include "main/AppStateManager.h"
#include <fstream>

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	AttributesComponent::AttributesComponent()
		: GameObjectComponent(),
		cloneWithInitValues(true)
	{
		this->attributesCount = new Variant(AttributesComponent::AttrAttributesCount(), 1, this->attributes);
		this->attributeNames.emplace_back(new Variant(AttributesComponent::AttrAttributeName() + "0", Ogre::String(""), this->attributes));
		this->attributeTypes.emplace_back(new Variant(AttributesComponent::AttrAttributeType() + "0", { "Bool", "Int", "Real", "String", "Vector2", "Vector3", "Vector4"/*, "StringList"*/ }, this->attributes));
		this->attributeValues.emplace_back(new Variant(AttributesComponent::AttrAttributeValue() + "0", Ogre::String(""), this->attributes));
		// Since when attributes count is changed, the whole properties must be refreshed, so that new field may come for attributes
		this->attributesCount->addUserData(GameObject::AttrActionNeedRefresh());
	}

	AttributesComponent::~AttributesComponent()
	{
		// this->deleteAllAttributes(); will be handled in game object component
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AttributesComponent] Destructor physics attributes component for game object: " 
			+ this->gameObjectPtr->getName() + "'");
	}

	bool AttributesComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AttributesCount")
		{
			this->attributesCount->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data", 1));
			propertyElement = propertyElement->next_sibling("property");
		}
		
		size_t oldSize = this->attributeNames.size();

		if (this->attributeNames.size() < this->attributesCount->getUInt())
		{
			this->attributeNames.resize(this->attributesCount->getUInt());
			this->attributeTypes.resize(this->attributesCount->getUInt());
			this->attributeValues.resize(this->attributesCount->getUInt());
		}
		for (size_t i = 0; i < this->attributeNames.size(); i++)
		{
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AttributeName" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->attributeNames[i])
				{
					this->attributeNames[i] = new Variant(AttributesComponent::AttrAttributeName() + Ogre::StringConverter::toString(i), XMLConverter::getAttrib(propertyElement, "data"), this->attributes);
				}
				else
				{
					this->attributeNames[i]->setValue(XMLConverter::getAttrib(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AttributeType" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->attributeTypes[i])
				{
					this->attributeTypes[i] = new Variant(AttributesComponent::AttrAttributeType() + Ogre::StringConverter::toString(i),
						{ "Bool", "Int", "Real", "String", "Vector2", "Vector3", "Vector4"/*, "StringList"*/ }, this->attributes);

					this->attributeTypes[i]->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
				}
				else
				{
					this->attributeTypes[i]->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AttributeValue" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->attributeValues[i])
				{
					this->attributeValues[i] = new Variant(AttributesComponent::AttrAttributeValue() + Ogre::StringConverter::toString(i), XMLConverter::getAttrib(propertyElement, "data"), this->attributes);
				}
				else
				{
					this->attributeValues[i]->setValue(XMLConverter::getAttrib(propertyElement, "data"));
				}
				// Make a link from value to name for better identification
				this->attributeValues[i]->setUserData(this->attributeNames[i]->getString());
				this->attributeValues[i]->addUserData(GameObject::AttrActionSeparator());
				propertyElement = propertyElement->next_sibling("property");
			}
		}
		return true;
	}

	GameObjectCompPtr AttributesComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		AttributesCompPtr clonedCompPtr(boost::make_shared<AttributesComponent>());

		
		clonedCompPtr->setCloneWithInitValues(this->cloneWithInitValues);
		
		clonedCompPtr->setAttributesCount(this->attributesCount->getUInt());

		for (unsigned int i = 0; i < static_cast<unsigned int>(this->attributeNames.size()); i++)
		{
			clonedCompPtr->setAttributeName(this->attributeNames[i]->getString(), i);
			Ogre::String type = this->attributeTypes[i]->getListSelectedValue();
			clonedCompPtr->setAttributeType(type, i);

			if ("Bool" == type)
			{
				clonedCompPtr->setAttributeValue(this->attributeValues[i]->getBool(), i);
			}
			else if ("Int" == type)
			{
				clonedCompPtr->setAttributeValue(this->attributeValues[i]->getInt(), i);
			}
			else if ("UInt" == type)
			{
				clonedCompPtr->setAttributeValue(this->attributeValues[i]->getUInt(), i);
			}
			else if ("ULong" == type)
			{
				clonedCompPtr->setAttributeValue(this->attributeValues[i]->getULong(), i);
			}
			else if ("Real" == type || "Float" == type)
			{
				clonedCompPtr->setAttributeValue(this->attributeValues[i]->getReal(), i);
			}
			else if ("String" == type)
			{
				clonedCompPtr->setAttributeValue(this->attributeValues[i]->getString(), i);
			}
			else if ("Vector2" == type)
			{
				clonedCompPtr->setAttributeValue(this->attributeValues[i]->getVector2(), i);
			}
			else if ("Vector3" == type)
			{
				clonedCompPtr->setAttributeValue(this->attributeValues[i]->getVector3(), i);
			}
			else if ("Vector4" == type)
			{
				clonedCompPtr->setAttributeValue(this->attributeValues[i]->getVector4(), i);
			}
			/*else if ("StringList" == type)
			{
			outFile << this->attributeNames[i]->getString() << "=" << this->attributeTypes[i]->getListSelectedValue() << "="<< this->attributeValues[i]->getVector4() << "\n";
			}*/
		}
		
		// Clone also custom created properties
		// clonedCompPtr->cloneAttributes(this->userAttributes);

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
		// return boost::make_shared<AttributesComponent>(*this);
	}

	bool AttributesComponent::postInit(void)
	{
		return true;
	}

	Ogre::String AttributesComponent::getClassName(void) const
	{
		return "AttributesComponent";
	}

	Ogre::String AttributesComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void AttributesComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (AttributesComponent::AttrAttributesCount() == attribute->getName())
		{
			this->setAttributesCount(attribute->getUInt());
		}
		
		for (size_t i = 0; i < this->attributeNames.size(); i++)
		{
			if (AttributesComponent::AttrAttributeName() + Ogre::StringConverter::toString(i) == attribute->getName())
			{
				this->attributeNames[i]->setValue(attribute->getString());
				// Make a link from value to name for better identification
				this->attributeValues[i]->setUserData(this->attributeNames[i]->getString());
// Attention: is that correct? See changecategory
				// this->changeName(attribute->getOldString(), attribute->getString()); // Old string is missing
			}
			else if (AttributesComponent::AttrAttributeType() + Ogre::StringConverter::toString(i) == attribute->getName())
			{
				Ogre::String type = attribute->getListSelectedValue();
				this->attributeTypes[i]->setListSelectedValue(type);
				
				this->changeType(this->attributeNames[i]->getName(), type);
			}
			else if (AttributesComponent::AttrAttributeValue() + Ogre::StringConverter::toString(i) == attribute->getName())
			{
				Ogre::String type = this->attributeTypes[i]->getListSelectedValue();
				this->attributeValues[i]->setValue(attribute->getString());
				if ("Bool" == type)
				{
					this->changeValue<bool>(this->attributeValues[i]->getName(), Ogre::StringConverter::parseBool(attribute->getString()));
				}
				else if ("Int" == type)
				{
					this->changeValue<int>(this->attributeValues[i]->getName(), Ogre::StringConverter::parseInt(attribute->getString()));
				}
				else if ("Real" == type)
				{
					this->changeValue<Ogre::Real>(this->attributeValues[i]->getName(), Ogre::StringConverter::parseReal(attribute->getString()));
				}
				else if ("String" == type)
				{
					this->changeValue<Ogre::String>(this->attributeValues[i]->getName(), attribute->getString());
				}
				else if ("Vector2" == type)
				{
					this->changeValue<Ogre::Vector2>(this->attributeValues[i]->getName(), Ogre::StringConverter::parseVector2(attribute->getString()));
				}
				else if ("Vector3" == type)
				{
					this->changeValue<Ogre::Vector3>(this->attributeValues[i]->getName(), Ogre::StringConverter::parseVector3(attribute->getString()));
				}
				else if ("Vector4" == type)
				{
					this->changeValue<Ogre::Vector4>(this->attributeValues[i]->getName(), Ogre::StringConverter::parseVector4(attribute->getString()));
				}
				else if ("StringList" == type)
				{
					this->changeValue<Ogre::String>(this->attributeValues[i]->getName(), attribute->getString());
				}
			}
		}
	}

	void AttributesComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AttributesCount"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->attributesCount->getUInt())));
		propertiesXML->append_node(propertyXML);

		for (size_t i = 0; i < this->attributeNames.size(); i++)
		{
			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "AttributeName" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->attributeNames[i]->getString())));
			propertiesXML->append_node(propertyXML);
			
			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "AttributeType" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->attributeTypes[i]->getListSelectedValue())));
			propertiesXML->append_node(propertyXML);
			
			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "AttributeValue" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->attributeValues[i]->getString())));
			propertiesXML->append_node(propertyXML);
		}

		/*for (auto& it = attributes.cbegin(); it != attributes.cend(); ++it)
		{
			propertyXML = doc.allocate_node(node_element, "property");

			Ogre::String strType;
			switch (it->second->getType())
			{
				case Variant::VAR_BOOL:
					propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
					propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, it->second->getName())));
					propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, it->second->getBool())));
					break;
				case Variant::VAR_INT:
					propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
					propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, it->second->getName())));
					propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, it->second->getInt())));
					break;
				case Variant::VAR_REAL:
					propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
					propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, it->second->getName())));
					propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, it->second->getReal())));
					break;
				case Variant::VAR_STRING:
					propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
					propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, it->second->getName())));
					propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, it->second->getString())));
					break;
				case Variant::VAR_VEC2:
					propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
					propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, it->second->getName())));
					propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, it->second->getVector2())));
					break;
				case Variant::VAR_VEC3:
					propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
					propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, it->second->getName())));
					propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, it->second->getVector3())));
					break;
				case Variant::VAR_VEC4:
				case Variant::VAR_QUAT:
					propertyXML->append_attribute(doc.allocate_attribute("type", "10"));
					propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, it->second->getName())));
					propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, it->second->getVector4()))); // attentin: here getQuat??
					break;
			}
			propertiesXML->append_node(propertyXML);
		}*/
	}
	
	void AttributesComponent::setAttributesCount(unsigned int attributesCount)
	{
		this->attributesCount->setValue(attributesCount);
		
		size_t oldSize = this->attributeNames.size();

		if (attributesCount > oldSize)
		{
			// Resize the attributes array for count
			this->attributeNames.resize(attributesCount);
			this->attributeTypes.resize(attributesCount);
			this->attributeValues.resize(attributesCount);

			for (size_t i = oldSize; i < this->attributeNames.size(); i++)
			{
				this->attributeNames[i] = new Variant(AttributesComponent::AttrAttributeName() + Ogre::StringConverter::toString(i), Ogre::String(""), this->attributes);
				this->attributeTypes[i] = new Variant(AttributesComponent::AttrAttributeType() + Ogre::StringConverter::toString(i), { "Bool", "Int", "Real", "String", "Vector2", "Vector3", "Vector4", "StringList" }, this->attributes);
				this->attributeValues[i] = new Variant(AttributesComponent::AttrAttributeValue() + Ogre::StringConverter::toString(i), Ogre::String(""), this->attributes);
				// Make a link from value to name for better identification
				this->attributeValues[i]->setUserData(this->attributeNames[i]->getString());
				this->attributeValues[i]->addUserData(GameObject::AttrActionSeparator());
			}
		}
		else if (attributesCount < oldSize)
		{
			for (auto it = this->attributeNames.begin() + attributesCount; it != this->attributeNames.end();)
			{
				Variant* variant = *it;
				// Remove from main attributes list
				for (auto subIt = this->attributes.begin(); subIt != this->attributes.end();)
				{
					if (variant == subIt->second)
					{
						subIt = this->attributes.erase(subIt);
					}
					else
					{
						++subIt;
					}
				}
				// Remove from local attribute names and delete
				it = this->attributeNames.erase(it);
				delete variant;
			}
			for (auto it = this->attributeTypes.begin() + attributesCount; it != this->attributeTypes.end();)
			{
				Variant* variant = *it;
				// Remove from main attributes list
				for (auto subIt = this->attributes.begin(); subIt != this->attributes.end();)
				{
					if (variant == subIt->second)
					{
						subIt = this->attributes.erase(subIt);
					}
					else
					{
						++subIt;
					}
				}
				// Remove from local attribute types and delete
				it = this->attributeTypes.erase(it);
				delete variant;
			}
			for (auto it = this->attributeValues.begin() + attributesCount; it != this->attributeValues.end();)
			{
				Variant* variant = *it;
				// Remove from main attributes list
				for (auto subIt = this->attributes.begin(); subIt != this->attributes.end();)
				{
					if (variant == subIt->second)
					{
						subIt = this->attributes.erase(subIt);
					}
					else
					{
						++subIt;
					}
				}
				// Remove from local attribute values and delete
				it = this->attributeValues.erase(it);
				delete variant;
			}
		}
	}

	unsigned int AttributesComponent::getAttributesCount(void) const
	{
		return this->attributesCount->getUInt();
	}

	Variant* AttributesComponent::getAttributeValueByIndex(unsigned int index) const
	{
		if (index > this->attributeNames.size())
			return nullptr;

		return this->attributeValues[index];
	}
	
	Variant* AttributesComponent::getAttributeValueByName(const Ogre::String& attributeName)
	{
		for (size_t i = 0; i < this->attributeNames.size(); i++)
		{
			if (attributeName == this->attributeNames[i]->getString())
			{
				return this->attributeValues[i];
			}
		}
		return nullptr;
	}

	void AttributesComponent::setAttributeName(const Ogre::String& attributeName, unsigned int index)
	{
		if (index > this->attributeNames.size())
			return;
		this->attributeNames[index]->setValue(attributeName);
		// Make a link from value to name for better identification
			this->attributeValues[index]->setUserData(this->attributeNames[index]->getString());
	}

	Ogre::String AttributesComponent::getAttributeName(unsigned int index)
	{
		if (index > this->attributeNames.size())
			return "";
		return this->attributeNames[index]->getString();
	}
	
	void AttributesComponent::setAttributeType(const Ogre::String& attributeType, unsigned int index)
	{
		if (index > this->attributeTypes.size())
			return;
		this->attributeTypes[index]->setListSelectedValue(attributeType);
	}

	Ogre::String AttributesComponent::getAttributeType(unsigned int index)
	{
		if (index > this->attributeTypes.size())
			return "";
		return this->attributeTypes[index]->getListSelectedValue();
	}

	bool AttributesComponent::getAttributeValueBool(unsigned int index)
	{
		if (index > this->attributeValues.size())
			return false;
		return this->attributeValues[index]->getBool();
	}

	int AttributesComponent::getAttributeValueInt(unsigned int index)
	{
		if(index > this->attributeValues.size())
			return 0;
		return this->attributeValues[index]->getInt();
	}

	unsigned int AttributesComponent::getAttributeValueUInt(unsigned int index)
	{
		if (index > this->attributeValues.size())
			return 0;
		return this->attributeValues[index]->getUInt();
	}

	unsigned long AttributesComponent::getAttributeValueULong(unsigned int index)
	{
		if (index > this->attributeValues.size())
			return 0;
		return this->attributeValues[index]->getULong();
	}

	Ogre::String AttributesComponent::getAttributeValueString(unsigned int index)
	{
		if (index > this->attributeValues.size())
			return "";
		return this->attributeValues[index]->getString();
	}

	Ogre::Real AttributesComponent::getAttributeValueReal(unsigned int index)
	{
		if (index > this->attributeValues.size())
			return 0.0f;
		return this->attributeValues[index]->getReal();
	}

	Ogre::Vector2 AttributesComponent::getAttributeValueVector2(unsigned int index)
	{
		if (index > this->attributeValues.size())
			return Ogre::Vector2::ZERO;
		return this->attributeValues[index]->getVector2();
	}

	Ogre::Vector3 AttributesComponent::getAttributeValueVector3(unsigned int index)
	{
		if (index > this->attributeValues.size())
			return Ogre::Vector3::ZERO;
		return this->attributeValues[index]->getVector3();
	}

	Ogre::Vector4 AttributesComponent::getAttributeValueVector4(unsigned int index)
	{
		if (index > this->attributeValues.size())
			return Ogre::Vector4::ZERO;
		return this->attributeValues[index]->getVector4();
	}

	bool AttributesComponent::getAttributeValueBool(const Ogre::String& name)
	{
		for (size_t i = 0; i < this->attributeNames.size(); i++)
		{
			if (name == this->attributeNames[i]->getString())
			{
				return this->attributeValues[i]->getBool();
			}
		}
		return false;
	}

	int AttributesComponent::getAttributeValueInt(const Ogre::String& name)
	{
		for (size_t i = 0; i < this->attributeNames.size(); i++)
		{
			if (name == this->attributeNames[i]->getString())
			{
				return this->attributeValues[i]->getInt();
			}
		}
		return 0;
	}

	unsigned int AttributesComponent::getAttributeValueUInt(const Ogre::String& name)
	{
		for (size_t i = 0; i < this->attributeNames.size(); i++)
		{
			if (name == this->attributeNames[i]->getString())
			{
				return this->attributeValues[i]->getUInt();
			}
		}
		return 0;
	}

	unsigned long AttributesComponent::getAttributeValueULong(const Ogre::String& name)
	{
		for (size_t i = 0; i < this->attributeNames.size(); i++)
		{
			if (name == this->attributeNames[i]->getString())
			{
				return this->attributeValues[i]->getULong();
			}
		}
		return 0;
	}

	Ogre::String AttributesComponent::getAttributeValueString(const Ogre::String& name)
	{
		for (size_t i = 0; i < this->attributeNames.size(); i++)
		{
			if (name == this->attributeNames[i]->getString())
			{
				return this->attributeValues[i]->getString();
			}
		}
		return "";
	}

	Ogre::Real AttributesComponent::getAttributeValueReal(const Ogre::String& name)
	{
		for (size_t i = 0; i < this->attributeNames.size(); i++)
		{
			if (name == this->attributeNames[i]->getString())
			{
				return this->attributeValues[i]->getReal();
			}
		}
		return 0.0f;
	}

	Ogre::Vector2 AttributesComponent::getAttributeValueVector2(const Ogre::String& name)
	{
		for (size_t i = 0; i < this->attributeNames.size(); i++)
		{
			if (name == this->attributeNames[i]->getString())
			{
				return this->attributeValues[i]->getVector2();
			}
		}
		return Ogre::Vector2::ZERO;
	}

	Ogre::Vector3 AttributesComponent::getAttributeValueVector3(const Ogre::String& name)
	{
		for (size_t i = 0; i < this->attributeNames.size(); i++)
		{
			if (name == this->attributeNames[i]->getString())
			{
				return this->attributeValues[i]->getVector3();
			}
		}
		return Ogre::Vector3::ZERO;
	}

	Ogre::Vector4 AttributesComponent::getAttributeValueVector4(const Ogre::String& name)
	{
		for (size_t i = 0; i < this->attributeNames.size(); i++)
		{
			if (name == this->attributeNames[i]->getString())
			{
				return this->attributeValues[i]->getVector4();
			}
		}
		return Ogre::Vector4::ZERO;
	}

	void AttributesComponent::setCloneWithInitValues(bool cloneWithInitValues)
	{
		this->cloneWithInitValues = cloneWithInitValues;
	}

	bool AttributesComponent::getCloneWithInitValues(void) const
	{
		return this->cloneWithInitValues;
	}
	
	void AttributesComponent::changeName(const Ogre::String& oldName, const Ogre::String& newName)
	{
		if (true == attributeNames.empty())
		{
			return;
		}
// TODO: Implement
		//const auto& it = this->userAttributes.find(oldName);
		//if (it != this->userAttributes.end())
		//{
		//	// here with swap like in goc
		//	it->second->setType(attributeType);
		//}
	}

	void AttributesComponent::changeType(const Ogre::String& attributeName, const Ogre::String& attributeType)
	{
		if (attributeName.empty())
		{
			return;
		}

// TODO: Implement
		/*
		for (auto& attribute : this->userAttributes)
			{
				if (attribute.first == attributeName)
				{
					attribute.second->setValue(newAttributeValue);
					break;
				}
			}
		*/
		/*const auto& it = this->userAttributes.find(attributeName);
		if (it != this->userAttributes.end())
		{
			it->second->setType(attributeType);
		}*/
	}

	bool AttributesComponent::removeAttribute(const Ogre::String& attributeName)
	{
		if (attributeName.empty())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AttributesComponent] Could not remove attribute, because the attribute name is empty for game object: " 
				+ this->gameObjectPtr->getName());
			return false;
		}
		
		unsigned int i = 0;
		for (auto it = this->attributes.begin(); it != this->attributes.end(); ++it)
		{
			auto data = *it;

			if (data.first == attributeName)
			{
				this->attributesCount->setValue(this->attributesCount->getUInt() - 1);

				for (auto it = this->attributeNames.begin() + i; it != this->attributeNames.end();)
				{
					Variant* variant = *it;
					// Remove from main attributes list
					for (auto subIt = this->attributes.begin(); subIt != this->attributes.end();)
					{
						if (variant == subIt->second)
						{
							subIt = this->attributes.erase(subIt);
						}
						else
						{
							++subIt;
						}
					}
					// Remove from local attribute names and delete
					it = this->attributeNames.erase(it);
					delete variant;
				}

				for (auto it = this->attributeTypes.begin() + i; it != this->attributeTypes.end();)
				{
					Variant* variant = *it;
					// Remove from main attributes list
					for (auto subIt = this->attributes.begin(); subIt != this->attributes.end();)
					{
						if (variant == subIt->second)
						{
							subIt = this->attributes.erase(subIt);
						}
						else
						{
							++subIt;
						}
					}
					// Remove from local attribute types and delete
					it = this->attributeTypes.erase(it);
					delete variant;
				}
				for (auto it = this->attributeValues.begin() + i; it != this->attributeValues.end();)
				{
					Variant* variant = *it;
					// Remove from main attributes list
					for (auto subIt = this->attributes.begin(); subIt != this->attributes.end();)
					{
						if (variant == subIt->second)
						{
							subIt = this->attributes.erase(subIt);
						}
						else
						{
							++subIt;
						}
					}
					// Remove from local attribute values and delete
					it = this->attributeValues.erase(it);
					delete variant;
				}
			}

			i++;
		}
		return true;
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AttributesComponent] Could not change Value, because the attribute value name: '" 
			+ attributeName + "' does not exist for game object: " + this->gameObjectPtr->getName());
		return false;
	}

	void AttributesComponent::saveValues(const Ogre::String& saveName, bool crypted)
	{
		AppStateManager::getSingletonPtr()->getGameProgressModule()->saveValues(saveName, this->gameObjectPtr->getId(), crypted);
	}

	void AttributesComponent::saveValue(const Ogre::String& saveName, unsigned int index, bool crypted)
	{
		AppStateManager::getSingletonPtr()->getGameProgressModule()->saveValue(saveName, this->gameObjectPtr->getId(), index, crypted);
	}
	
	void AttributesComponent::saveValue(const Ogre::String& saveName, Variant* attribute, bool crypted)
	{
		int index = -1;
		Ogre::String attributeNameLink = attribute->getUserDataValue("Link");
		for (size_t i = 0; i < this->attributeNames.size(); i++)
		{
			// Check if the link from value to attribute name does match to get the index
			if (attributeNameLink == this->attributeNames[i]->getString())
			{
				index = static_cast<unsigned int>(i);
				break;
			}
		}
		if (index != -1)
			AppStateManager::getSingletonPtr()->getGameProgressModule()->saveValue(saveName, this->gameObjectPtr->getId(), index, crypted);
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AttributesComponent] Could not save value for attribute name: '" + attributeNameLink + "', because the attribute does not exist. GameObject: "
				+ this->gameObjectPtr->getName() + "'");
		}
	}

	bool AttributesComponent::loadValues(const Ogre::String& saveName)
	{
		return AppStateManager::getSingletonPtr()->getGameProgressModule()->loadValues(saveName, this->gameObjectPtr->getId());
	}

	bool AttributesComponent::loadValue(const Ogre::String& saveName, unsigned int index)
	{
		return AppStateManager::getSingletonPtr()->getGameProgressModule()->loadValue(saveName, this->gameObjectPtr->getId(), index);
	}
	
	bool AttributesComponent::loadValue(const Ogre::String& saveName, Variant* attribute)
	{
		if (nullptr == attribute)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AttributesComponent] Could not load value for attribute, because the attribute does not exist. GameObject: "
														   + this->gameObjectPtr->getName() + "'");
			return false;
		}

		int index = -1;
		Ogre::String attributeNameLink = attribute->getUserDataValue("Link");
		for (size_t i = 0; i < this->attributeNames.size(); i++)
		{
			// Check if the link from value to attribute name does match to get the index
			if (attributeNameLink == this->attributeNames[i]->getString())
			{
				index = static_cast<unsigned int>(i);
				break;
			}
		}
		if (index != -1)
			return AppStateManager::getSingletonPtr()->getGameProgressModule()->loadValue(saveName, this->gameObjectPtr->getId(), index);
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AttributesComponent] Could not load value for attribute name: '" + attributeNameLink + "', because the attribute does not exist. GameObject: "
				+ this->gameObjectPtr->getName() + "'");
			return false;
		}
	}
	
	void AttributesComponent::internalSave(std::ostringstream& outFile)
	{
		for (size_t i = 0; i < this->attributeNames.size(); i++)
		{
			Ogre::String type = this->attributeTypes[i]->getListSelectedValue();

			if ("Bool" == type)
			{
				outFile << this->attributeNames[i]->getString() << "=" << this->attributeTypes[i]->getListSelectedValue() << "=" << this->attributeValues[i]->getBool() << "\n";
			}
			else if ("Int" == type)
			{
				outFile << this->attributeNames[i]->getString() << "=" << this->attributeTypes[i]->getListSelectedValue() << "=" << this->attributeValues[i]->getInt() << "\n";
			}
			else if ("UInt" == type)
			{
				outFile << this->attributeNames[i]->getString() << "=" << this->attributeTypes[i]->getListSelectedValue() << "=" << this->attributeValues[i]->getUInt() << "\n";
			}
			else if ("ULong" == type)
			{
				outFile << this->attributeNames[i]->getString() << "=" << this->attributeTypes[i]->getListSelectedValue() << "=" << this->attributeValues[i]->getULong() << "\n";
			}
			else if ("Real" == type || "Float" == type)
			{
				outFile << this->attributeNames[i]->getString() << "=" << this->attributeTypes[i]->getListSelectedValue() << "=" << this->attributeValues[i]->getReal() << "\n";
			}
			else if ("String" == type)
			{
				outFile << this->attributeNames[i]->getString() << "=" << this->attributeTypes[i]->getListSelectedValue() << "=" << this->attributeValues[i]->getString() << "\n";
			}
			else if ("Vector2" == type)
			{
				outFile << this->attributeNames[i]->getString() << "=" << this->attributeTypes[i]->getListSelectedValue() << "=" << this->attributeValues[i]->getVector2() << "\n";
			}
			else if ("Vector3" == type)
			{
				outFile << this->attributeNames[i]->getString() << "=" << this->attributeTypes[i]->getListSelectedValue() << "=" << this->attributeValues[i]->getVector3() << "\n";
			}
			else if ("Vector4" == type)
			{
				outFile << this->attributeNames[i]->getString() << "=" << this->attributeTypes[i]->getListSelectedValue() << "=" << this->attributeValues[i]->getVector4() << "\n";
			}
			/*else if ("StringList" == type)
			{
				outFile << this->attributeNames[i]->getString() << "=" << this->attributeTypes[i]->getListSelectedValue() << "="<< this->attributeValues[i]->getVector4() << "\n";
			}*/
		}
	}
	
	void AttributesComponent::internalSave(Ogre::String& content, unsigned int index)
	{
		size_t lineEndPos = 0;
		int remainingLength = static_cast<unsigned int>(content.size());
		
		Ogre::String tempContent;
		bool firstRound = true;

		unsigned int tempIndex = 0;
		while (lineEndPos < content.size())
		{
			if (tempIndex == index)
			{				
				Ogre::String type = this->attributeTypes[index]->getListSelectedValue();

				if ("Bool" == type)
				{
					tempContent = this->attributeNames[index]->getString() + "=" + this->attributeTypes[index]->getListSelectedValue() + "=" + Ogre::StringConverter::toString(this->attributeValues[index]->getBool()) + "\n";
				}
				else if ("Int" == type)
				{
					tempContent = this->attributeNames[index]->getString() + "=" + this->attributeTypes[index]->getListSelectedValue() + "=" + Ogre::StringConverter::toString(this->attributeValues[index]->getInt()) + "\n";
				}
				else if ("UInt" == type)
				{
					tempContent = this->attributeNames[index]->getString() + "=" + this->attributeTypes[index]->getListSelectedValue() + "=" + Ogre::StringConverter::toString(this->attributeValues[index]->getUInt()) + "\n";
				}
				else if ("ULong" == type)
				{
					tempContent = this->attributeNames[index]->getString() + "=" + this->attributeTypes[index]->getListSelectedValue() + "=" + Ogre::StringConverter::toString(this->attributeValues[index]->getULong()) + "\n";
				}
				else if ("Real" == type || "Float" == type)
				{
					tempContent = this->attributeNames[index]->getString() + "=" + this->attributeTypes[index]->getListSelectedValue() + "=" + Ogre::StringConverter::toString(this->attributeValues[index]->getReal()) + "\n";
				}
				else if ("String" == type)
				{
					tempContent = this->attributeNames[index]->getString() + "=" + this->attributeTypes[index]->getListSelectedValue() + "=" + this->attributeValues[index]->getString() + "\n";
				}
				else if ("Vector2" == type)
				{
					tempContent = this->attributeNames[index]->getString() + "=" + this->attributeTypes[index]->getListSelectedValue() + "=" + Ogre::StringConverter::toString(this->attributeValues[index]->getVector2()) + "\n";
				}
				else if ("Vector3" == type)
				{
					tempContent = this->attributeNames[index]->getString() + "=" + this->attributeTypes[index]->getListSelectedValue() + "=" + Ogre::StringConverter::toString(this->attributeValues[index]->getVector3()) + "\n";
				}
				else if ("Vector4" == type)
				{
					tempContent = this->attributeNames[index]->getString() + "=" + this->attributeTypes[index]->getListSelectedValue() + "=" + Ogre::StringConverter::toString(this->attributeValues[index]->getVector4()) + "\n";
				}
				/*else if ("StringList" == type)
				{
					tempContent = this->attributeNames[index]->getString() + "=" << this->attributeTypes[index]->getListSelectedValue() + "=" + Ogre::StringConverter::toString(this->attributeValues[index]->getVector4()) + "\n";
				}*/
				
				if (true == firstRound)
				{
					remainingLength = static_cast<unsigned int>(content.size() - tempContent.size() - lineEndPos) + 1;
					firstRound = false;
				}
				else
					remainingLength = static_cast<unsigned int>(content.size() - tempContent.size() - lineEndPos);
				
				Ogre::String priorContent;
				if (lineEndPos > 0)
					priorContent = content.substr(0, lineEndPos);

				Ogre::String afterContent;
				if (remainingLength > 0)
					afterContent = content.substr(lineEndPos + tempContent.size(), remainingLength);

				content = priorContent + tempContent + afterContent;
			}
			
			lineEndPos = content.find('\n', lineEndPos);
			if (std::wstring::npos != lineEndPos)
			{
				lineEndPos += 1;
				tempIndex++;
			}
		}
	}
	
	bool AttributesComponent::internalRead(std::istringstream& inFile)
	{
		bool success = false;
		Ogre::String line;
		Ogre::StringVector data;
		
		// parse til eof
		while(std::getline(inFile, line)) // This is used, because white spaces are also read
		{
			// Parse til next game object
			if (line.find("[") != Ogre::String::npos)
				break;
			
			data = Ogre::StringUtil::split(line, "=");
			if (data.size() < 3)
				continue;

			if (true == this->attributeNames.empty())
				return false;
			
			for (size_t i = 0; i < this->attributeNames.size(); i++)
			{
				// Check if its the correkt name and set its value
				if (this->attributeNames[i]->getString() == data[0])
				{
					this->attributeTypes[i]->setListSelectedValue(data[1]);

					if ("Bool" == data[1])
					{
						this->attributeValues[i]->setValue(Ogre::StringConverter::parseBool(data[2]));
						success = true;
					}
					else if ("Int" == data[1])
					{
						this->attributeValues[i]->setValue(Ogre::StringConverter::parseInt(data[2]));
						success = true;
					}
					else if ("UInt" == data[1])
					{
						this->attributeValues[i]->setValue(Ogre::StringConverter::parseUnsignedInt(data[2]));
						success = true;
					}
					else if ("ULong" == data[1])
					{
						this->attributeValues[i]->setValue(Ogre::StringConverter::parseUnsignedLong(data[2]));
						success = true;
					}
					else if ("Real" == data[1] || "Float" == data[1])
					{
						this->attributeValues[i]->setValue(Ogre::StringConverter::parseReal(data[2]));
						success = true;
					}
					else if ("String" == data[1])
					{
						this->attributeValues[i]->setValue(data[2]);
						success = true;
					}
					else if ("Vector2" == data[1])
					{
						this->attributeValues[i]->setValue(Ogre::StringConverter::parseVector2(data[2]));
						success = true;
					}
					else if ("Vector3" == data[1])
					{
						this->attributeValues[i]->setValue(Ogre::StringConverter::parseVector3(data[2]));
						success = true;
					}
					else if ("Vector4" == data[1])
					{
						this->attributeValues[i]->setValue(Ogre::StringConverter::parseVector4(data[2]));
						success = true;
					}
				}
			}
		}

		return success;
	}
	
	bool AttributesComponent::internalRead(std::istringstream& inFile, unsigned int index)
	{
		bool success = false;
		Ogre::String line;
		Ogre::StringVector data;
		
		// parse til eof
		while (std::getline(inFile, line)) // This is used, because white spaces are also read
		{
			data = Ogre::StringUtil::split(line, "=");
			if (data.size() < 3)
				continue;

			if (true == this->attributeNames.empty())
				return false;
			
			if (index > this->attributeNames.size())
				return success;
			
			// Check if its the correkt name and set its value
			if (this->attributeNames[index]->getString() == data[0])
			{
				this->attributeTypes[index]->setListSelectedValue(data[1]);

				if ("Bool" == data[1])
				{
					this->attributeValues[index]->setValue(Ogre::StringConverter::parseBool(data[2]));
					success = true;
				}
				else if ("Int" == data[1])
				{
					this->attributeValues[index]->setValue(Ogre::StringConverter::parseInt(data[2]));
					success = true;
				}
				else if ("UInt" == data[1])
				{
					this->attributeValues[index]->setValue(Ogre::StringConverter::parseUnsignedInt(data[2]));
					success = true;
				}
				else if ("ULong" == data[1])
				{
					this->attributeValues[index]->setValue(Ogre::StringConverter::parseUnsignedLong(data[2]));
					success = true;
				}
				else if ("Real" == data[1] || "Float" == data[1])
				{
					this->attributeValues[index]->setValue(Ogre::StringConverter::parseReal(data[2]));
					success = true;
				}
				else if ("String" == data[1])
				{
					this->attributeValues[index]->setValue(data[2]);
					success = true;
				}
				else if ("Vector2" == data[1])
				{
					this->attributeValues[index]->setValue(Ogre::StringConverter::parseVector2(data[2]));
					success = true;
				}
				else if ("Vector3" == data[1])
				{
					this->attributeValues[index]->setValue(Ogre::StringConverter::parseVector3(data[2]));
					success = true;
				}
				else if ("Vector4" == data[1])
				{
					this->attributeValues[index]->setValue(Ogre::StringConverter::parseVector4(data[2]));
					success = true;
				}

				// Found to be loaded value, so break
				if (true == success)
					break;
			}
			
			// Parse til next game object
			if (line.find("[") != Ogre::String::npos)
				break;
		}

		return success;
	}

}; // namespace end
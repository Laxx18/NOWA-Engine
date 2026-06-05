#include "NOWAPrecompiled.h"
#include "AttributesComponent.h"
#include "main/AppStateManager.h"
#include "main/Core.h"
#include "utilities/XMLConverter.h"
#include <fstream>

#define LUA_CONST_START(class)                                                                                                                                                                                                                           \
    {                                                                                                                                                                                                                                                    \
        object g = globals(lua);                                                                                                                                                                                                                         \
        object table = g[#class];
#define LUA_CONST(class, name) table[#name] = class ::name
#define LUA_CONST_END }

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    AttributesComponent::AttributesComponent() : GameObjectComponent(), cloneWithInitValues(true)
    {
        this->attributesCount = new Variant(AttributesComponent::AttrAttributesCount(), 1, this->attributes);
        this->attributeNames.emplace_back(new Variant(AttributesComponent::AttrAttributeName() + "0", Ogre::String(""), this->attributes));
        this->attributeTypes.emplace_back(new Variant(AttributesComponent::AttrAttributeType() + "0", {"Bool", "Int", "Real", "String", "Vector2", "Vector3", "Vector4"}, this->attributes));
        this->attributeValues.emplace_back(new Variant(AttributesComponent::AttrAttributeValue() + "0", Ogre::String(""), this->attributes));
        // Since when attributes count is changed, the whole properties must be refreshed, so that new field may come for attributes
        this->attributesCount->addUserData(GameObject::AttrActionNeedRefresh());
    }

    AttributesComponent::~AttributesComponent()
    {
        // this->deleteAllAttributes(); will be handled in game object component
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AttributesComponent] Destructor attributes component for game object: " + this->gameObjectPtr->getName() + "'");
    }

    bool AttributesComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        GameObjectComponent::init(propertyElement);

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
                    this->attributeTypes[i] = new Variant(AttributesComponent::AttrAttributeType() + Ogre::StringConverter::toString(i), {"Bool", "Int", "Real", "String", "Vector2", "Vector3", "Vector4"}, this->attributes);

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
        }

        clonedGameObjectPtr->addComponent(clonedCompPtr);
        clonedCompPtr->setOwner(clonedGameObjectPtr);

        GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
        return clonedCompPtr;
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
            }
            else if (AttributesComponent::AttrAttributeType() + Ogre::StringConverter::toString(i) == attribute->getName())
            {
                Ogre::String type = attribute->getListSelectedValue();
                this->attributeTypes[i]->setListSelectedValue(type);
                // changeType is a TODO stub, keep the call for future implementation
                this->changeType(this->attributeNames[i]->getString(), type);
            }
            else if (AttributesComponent::AttrAttributeValue() + Ogre::StringConverter::toString(i) == attribute->getName())
            {
                Ogre::String type = this->attributeTypes[i]->getListSelectedValue();

                // Bug fix: update attributeValues[i] directly by user-visible name, not via the
                // changeValue(Variant internal name) path which never matched anything.
                Ogre::String userAttributeName = this->attributeNames[i]->getString();

                if ("Bool" == type)
                {
                    bool val = Ogre::StringConverter::parseBool(attribute->getString());
                    this->attributeValues[i]->setValue(val);
                    this->fireAttributeChanged(userAttributeName, this->attributeValues[i]);
                }
                else if ("Int" == type)
                {
                    int val = Ogre::StringConverter::parseInt(attribute->getString());
                    this->attributeValues[i]->setValue(val);
                    this->fireAttributeChanged(userAttributeName, this->attributeValues[i]);
                }
                else if ("UInt" == type)
                {
                    unsigned int val = Ogre::StringConverter::parseUnsignedInt(attribute->getString());
                    this->attributeValues[i]->setValue(val);
                    this->fireAttributeChanged(userAttributeName, this->attributeValues[i]);
                }
                else if ("ULong" == type)
                {
                    unsigned long val = Ogre::StringConverter::parseUnsignedLong(attribute->getString());
                    this->attributeValues[i]->setValue(val);
                    this->fireAttributeChanged(userAttributeName, this->attributeValues[i]);
                }
                else if ("Real" == type || "Float" == type)
                {
                    Ogre::Real val = Ogre::StringConverter::parseReal(attribute->getString());
                    this->attributeValues[i]->setValue(val);
                    this->fireAttributeChanged(userAttributeName, this->attributeValues[i]);
                }
                else if ("String" == type)
                {
                    this->attributeValues[i]->setValue(attribute->getString());
                    this->fireAttributeChanged(userAttributeName, this->attributeValues[i]);
                }
                else if ("Vector2" == type)
                {
                    Ogre::Vector2 val = Ogre::StringConverter::parseVector2(attribute->getString());
                    this->attributeValues[i]->setValue(val);
                    this->fireAttributeChanged(userAttributeName, this->attributeValues[i]);
                }
                else if ("Vector3" == type)
                {
                    Ogre::Vector3 val = Ogre::StringConverter::parseVector3(attribute->getString());
                    this->attributeValues[i]->setValue(val);
                    this->fireAttributeChanged(userAttributeName, this->attributeValues[i]);
                }
                else if ("Vector4" == type)
                {
                    Ogre::Vector4 val = Ogre::StringConverter::parseVector4(attribute->getString());
                    this->attributeValues[i]->setValue(val);
                    this->fireAttributeChanged(userAttributeName, this->attributeValues[i]);
                }
                else if ("StringList" == type)
                {
                    this->attributeValues[i]->setValue(attribute->getString());
                    this->fireAttributeChanged(userAttributeName, this->attributeValues[i]);
                }
            }
        }
    }

    void AttributesComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
                this->attributeTypes[i] = new Variant(AttributesComponent::AttrAttributeType() + Ogre::StringConverter::toString(i), {"Bool", "Int", "Real", "String", "Vector2", "Vector3", "Vector4", "StringList"}, this->attributes);
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
                it = this->attributeNames.erase(it);
                delete variant;
            }
            for (auto it = this->attributeTypes.begin() + attributesCount; it != this->attributeTypes.end();)
            {
                Variant* variant = *it;
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
                it = this->attributeTypes.erase(it);
                delete variant;
            }
            for (auto it = this->attributeValues.begin() + attributesCount; it != this->attributeValues.end();)
            {
                Variant* variant = *it;
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
        if (index >= this->attributeValues.size())
        {
            return nullptr;
        }

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
        if (index >= this->attributeNames.size())
        {
            return;
        }
        this->attributeNames[index]->setValue(attributeName);
        // Make a link from value to name for better identification
        this->attributeValues[index]->setUserData(this->attributeNames[index]->getString());
    }

    Ogre::String AttributesComponent::getAttributeName(unsigned int index)
    {
        if (index >= this->attributeNames.size())
        {
            return "";
        }
        return this->attributeNames[index]->getString();
    }

    void AttributesComponent::setAttributeType(const Ogre::String& attributeType, unsigned int index)
    {
        if (index >= this->attributeTypes.size())
        {
            return;
        }
        this->attributeTypes[index]->setListSelectedValue(attributeType);
    }

    Ogre::String AttributesComponent::getAttributeType(unsigned int index)
    {
        if (index >= this->attributeTypes.size())
        {
            return "";
        }
        return this->attributeTypes[index]->getListSelectedValue();
    }

    bool AttributesComponent::getAttributeValueBool(unsigned int index)
    {
        if (index >= this->attributeValues.size())
        {
            return false;
        }
        return this->attributeValues[index]->getBool();
    }

    int AttributesComponent::getAttributeValueInt(unsigned int index)
    {
        // Bug fix: was `index > size` (off-by-one), must be `>=`
        if (index >= this->attributeValues.size())
        {
            return 0;
        }
        return this->attributeValues[index]->getInt();
    }

    unsigned int AttributesComponent::getAttributeValueUInt(unsigned int index)
    {
        if (index >= this->attributeValues.size())
        {
            return 0;
        }
        return this->attributeValues[index]->getUInt();
    }

    unsigned long AttributesComponent::getAttributeValueULong(unsigned int index)
    {
        if (index >= this->attributeValues.size())
        {
            return 0;
        }
        return this->attributeValues[index]->getULong();
    }

    Ogre::String AttributesComponent::getAttributeValueString(unsigned int index)
    {
        if (index >= this->attributeValues.size())
        {
            return "";
        }
        return this->attributeValues[index]->getString();
    }

    Ogre::Real AttributesComponent::getAttributeValueReal(unsigned int index)
    {
        if (index >= this->attributeValues.size())
        {
            return 0.0f;
        }
        return this->attributeValues[index]->getReal();
    }

    Ogre::Vector2 AttributesComponent::getAttributeValueVector2(unsigned int index)
    {
        if (index >= this->attributeValues.size())
        {
            return Ogre::Vector2::ZERO;
        }
        return this->attributeValues[index]->getVector2();
    }

    Ogre::Vector3 AttributesComponent::getAttributeValueVector3(unsigned int index)
    {
        if (index >= this->attributeValues.size())
        {
            return Ogre::Vector3::ZERO;
        }
        return this->attributeValues[index]->getVector3();
    }

    Ogre::Vector4 AttributesComponent::getAttributeValueVector4(unsigned int index)
    {
        if (index >= this->attributeValues.size())
        {
            return Ogre::Vector4::ZERO;
        }
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
        if (true == this->attributeNames.empty())
        {
            return;
        }
        // TODO: Implement rename in the attributes map and all internal vectors
    }

    void AttributesComponent::changeType(const Ogre::String& attributeName, const Ogre::String& attributeType)
    {
        if (attributeName.empty())
        {
            return;
        }
        // TODO: Implement type change - requires reinterpreting the stored value
    }

    bool AttributesComponent::removeAttribute(const Ogre::String& attributeName)
    {
        if (attributeName.empty())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AttributesComponent] Could not remove attribute, because the attribute name is empty for game object: " + this->gameObjectPtr->getName());
            return false;
        }

        // Bug fix: the old code iterated this->attributes and compared against data.first which holds
        // the Variant's internal registration name (e.g. "Attribute Name 0"), never the user-visible
        // attribute name. Search attributeNames by value instead.
        for (size_t i = 0; i < this->attributeNames.size(); i++)
        {
            if (this->attributeNames[i]->getString() != attributeName)
            {
                continue;
            }

            this->attributesCount->setValue(this->attributesCount->getUInt() - 1);

            // Remove the name Variant
            {
                Variant* variant = this->attributeNames[i];
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
                this->attributeNames.erase(this->attributeNames.begin() + i);
                delete variant;
            }

            // Remove the type Variant
            {
                Variant* variant = this->attributeTypes[i];
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
                this->attributeTypes.erase(this->attributeTypes.begin() + i);
                delete variant;
            }

            // Remove the value Variant
            {
                Variant* variant = this->attributeValues[i];
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
                this->attributeValues.erase(this->attributeValues.begin() + i);
                delete variant;
            }

            return true;
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AttributesComponent] Could not remove attribute, because the attribute name: '" + attributeName + "' does not exist for game object: " + this->gameObjectPtr->getName());
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
                index = static_cast<int>(i);
                break;
            }
        }
        if (index != -1)
        {
            AppStateManager::getSingletonPtr()->getGameProgressModule()->saveValue(saveName, this->gameObjectPtr->getId(), static_cast<unsigned int>(index), crypted);
        }
        else
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
                "[AttributesComponent] Could not save value for attribute name: '" + attributeNameLink + "', because the attribute does not exist. GameObject: " + this->gameObjectPtr->getName() + "'");
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
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AttributesComponent] Could not load value for attribute, because the attribute does not exist. GameObject: " + this->gameObjectPtr->getName() + "'");
            return false;
        }

        int index = -1;
        Ogre::String attributeNameLink = attribute->getUserDataValue("Link");
        for (size_t i = 0; i < this->attributeNames.size(); i++)
        {
            // Check if the link from value to attribute name does match to get the index
            if (attributeNameLink == this->attributeNames[i]->getString())
            {
                index = static_cast<int>(i);
                break;
            }
        }
        if (index != -1)
        {
            return AppStateManager::getSingletonPtr()->getGameProgressModule()->loadValue(saveName, this->gameObjectPtr->getId(), static_cast<unsigned int>(index));
        }
        else
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
                "[AttributesComponent] Could not load value for attribute name: '" + attributeNameLink + "', because the attribute does not exist. GameObject: " + this->gameObjectPtr->getName() + "'");
            return false;
        }
    }

    void AttributesComponent::reactOnAttributeChanged(luabind::object closureFunction)
    {
        this->attributeChangedClosureFunction = closureFunction;
    }

    void AttributesComponent::fireAttributeChanged(const Ogre::String& userAttributeName, Variant* attributeVariant)
    {
        if (nullptr == this->gameObjectPtr->getLuaScript())
        {
            return;
        }
        if (!this->attributeChangedClosureFunction.is_valid())
        {
            return;
        }

        // Capture by value for the attribute name; capture the Variant* (its lifetime is guaranteed
        // by the component which outlives any queued logic command).
        NOWA::AppStateManager::LogicCommand logicCommand = [this, userAttributeName, attributeVariant]()
        {
            try
            {
                luabind::call_function<void>(this->attributeChangedClosureFunction, userAttributeName, attributeVariant);
            }
            catch (luabind::error& error)
            {
                luabind::object errorMsg(luabind::from_stack(error.state(), -1));
                std::stringstream msg;
                msg << errorMsg;

                Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[AttributesComponent] Caught error in 'reactOnAttributeChanged' for attribute '" + userAttributeName + "': " + Ogre::String(error.what()) + " details: " + msg.str());
            }
        };
        NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
    }

    // ---------------------------------------------------------------------------
    // Lua helper free functions (formerly in LuaScriptApi.cpp)
    // ---------------------------------------------------------------------------

    static AttributesComponent* getAttributesComponentFromIndex(GameObject* gameObject, unsigned int occurrenceIndex)
    {
        return makeStrongPtr<AttributesComponent>(gameObject->getComponentWithOccurrence<AttributesComponent>(occurrenceIndex)).get();
    }

    static AttributesComponent* getAttributesComponent(GameObject* gameObject)
    {
        return makeStrongPtr<AttributesComponent>(gameObject->getComponent<AttributesComponent>()).get();
    }

    static AttributesComponent* getAttributesComponentFromName(GameObject* gameObject, const Ogre::String& name)
    {
        return makeStrongPtr<AttributesComponent>(gameObject->getComponentFromName<AttributesComponent>(name)).get();
    }

    // Typed add-attribute helpers
    static void addAttributeBool(AttributesComponent* comp, const Ogre::String& name, bool value)
    {
        comp->addAttribute(name, value, Variant::VAR_BOOL);
    }

    static void addAttributeFloat(AttributesComponent* comp, const Ogre::String& name, Ogre::Real value)
    {
        comp->addAttribute(name, value, Variant::VAR_REAL);
    }

    static void addAttributeString(AttributesComponent* comp, const Ogre::String& name, const Ogre::String& value)
    {
        comp->addAttribute(name, value, Variant::VAR_STRING);
    }

    static void addAttributeVector2(AttributesComponent* comp, const Ogre::String& name, const Ogre::Vector2& value)
    {
        comp->addAttribute(name, value, Variant::VAR_VEC2);
    }

    static void addAttributeVector3(AttributesComponent* comp, const Ogre::String& name, const Ogre::Vector3& value)
    {
        comp->addAttribute(name, value, Variant::VAR_VEC3);
    }

    static void addAttributeVector4(AttributesComponent* comp, const Ogre::String& name, const Ogre::Vector4& value)
    {
        comp->addAttribute(name, value, Variant::VAR_VEC4);
    }

    // Typed change-value helpers
    static void changeValueBool(AttributesComponent* comp, const Ogre::String& name, bool value)
    {
        comp->changeValue(name, value);
    }

    static void changeValueFloat(AttributesComponent* comp, const Ogre::String& name, Ogre::Real value)
    {
        comp->changeValue(name, value);
    }

    static void changeValueString(AttributesComponent* comp, const Ogre::String& name, const Ogre::String& value)
    {
        comp->changeValue(name, value);
    }

    static void changeValueVector2(AttributesComponent* comp, const Ogre::String& name, const Ogre::Vector2& value)
    {
        comp->changeValue(name, value);
    }

    static void changeValueVector3(AttributesComponent* comp, const Ogre::String& name, const Ogre::Vector3& value)
    {
        comp->changeValue(name, value);
    }

    static void changeValueVector4(AttributesComponent* comp, const Ogre::String& name, const Ogre::Vector4& value)
    {
        comp->changeValue(name, value);
    }

    // Variant number helpers used by Lua bindings
    static void incrementValueNumber(Variant* variant, Ogre::Real amount)
    {
        variant->setValue(variant->getReal() + amount);
    }

    static void decrementValueNumber(Variant* variant, Ogre::Real amount)
    {
        variant->setValue(variant->getReal() - amount);
    }

    // ---------------------------------------------------------------------------
    // Static Lua API registration
    // ---------------------------------------------------------------------------

    void AttributesComponent::createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass)
    {
        module(lua)
        [
            class_<Variant>("Variant")
            .def(constructor<>())
            .def("assign", (void (Variant::*)(const Variant&))&Variant::assign)
            .def("getType", &Variant::getType)
            .def("getName", &Variant::getName)
            .def("setValueNumber", (void (Variant::*)(Ogre::Real))&Variant::setValue)
            .def("incrementValueNumber", &incrementValueNumber)
            .def("decrementValueNumber", &decrementValueNumber)
            .def("setValueBool", (void (Variant::*)(bool))&Variant::setValue)
            .def("setValueString", (void (Variant::*)(const Ogre::String&))&Variant::setValue)
            .def("setValueId", (void (Variant::*)(const Ogre::String&))&Variant::setValue)
            .def("setValueVector2", (void (Variant::*)(const Ogre::Vector2&))&Variant::setValue)
            .def("setValueVector3", (void (Variant::*)(const Ogre::Vector3&))&Variant::setValue)
            .def("setValueVector4", (void (Variant::*)(const Ogre::Vector4&))&Variant::setValue)
            .def("getValueNumber", &Variant::getReal)
            .def("getValueBool", &Variant::getBool)
            .def("getValueString", &Variant::getString)
            .def("getValueId", &Variant::getString)
            .def("getValueVector2", &Variant::getVector2)
            .def("getValueVector3", &Variant::getVector3)
            .def("getValueVector4", &Variant::getVector4)
        ];

        LuaScriptApi::getInstance()->addClassToCollection("Variant", "class", "The class Variant is a common datatype for any attribute key and value.");
        LuaScriptApi::getInstance()->addClassToCollection("Variant", "void assign(Variant other)", "Assigns another value to this Variant.");
        LuaScriptApi::getInstance()->addClassToCollection("Variant", "int getType()", "Gets the type of this variant.");
        LuaScriptApi::getInstance()->addClassToCollection("Variant", "String getName()", "Gets the name of this variant.");
        LuaScriptApi::getInstance()->addClassToCollection("Variant", "void setValueNumber(number value)", "Sets the number value.");
        LuaScriptApi::getInstance()->addClassToCollection("Variant", "void incrementValueNumber(number amount)", "Increments the number value by the given amount.");
        LuaScriptApi::getInstance()->addClassToCollection("Variant", "void decrementValueNumber(number amount)", "Decrements the number value by the given amount.");
        LuaScriptApi::getInstance()->addClassToCollection("Variant", "void setValueBool(bool value)", "Sets the bool value.");
        LuaScriptApi::getInstance()->addClassToCollection("Variant", "void setValueString(String value)", "Sets the String value.");
        LuaScriptApi::getInstance()->addClassToCollection("Variant", "void setValueId(String id)", "Sets the Game Object ID value.");
        LuaScriptApi::getInstance()->addClassToCollection("Variant", "void setValueVector2(Vector2 value)", "Sets the Vector2 value.");
        LuaScriptApi::getInstance()->addClassToCollection("Variant", "void setValueVector3(Vector3 value)", "Sets the Vector3 value.");
        LuaScriptApi::getInstance()->addClassToCollection("Variant", "void setValueVector4(Vector4 value)", "Sets the Vector4 value.");
        LuaScriptApi::getInstance()->addClassToCollection("Variant", "number getValueNumber()", "Gets the number value.");
        LuaScriptApi::getInstance()->addClassToCollection("Variant", "bool getValueBool()", "Gets the bool value.");
        LuaScriptApi::getInstance()->addClassToCollection("Variant", "String getValueString()", "Gets the string value.");
        LuaScriptApi::getInstance()->addClassToCollection("Variant", "String getValueId()", "Gets the id value.");
        LuaScriptApi::getInstance()->addClassToCollection("Variant", "Vector2 getValueVector2()", "Gets the Vector2 value.");
        LuaScriptApi::getInstance()->addClassToCollection("Variant", "Vector3 getValueVector3()", "Gets the Vector3 value.");
        LuaScriptApi::getInstance()->addClassToCollection("Variant", "Vector4 getValueVector4()", "Gets the Vector4 value.");

        LUA_CONST_START(Variant)
            LUA_CONST(Variant, VAR_NULL);
            LUA_CONST(Variant, VAR_BOOL);
            LUA_CONST(Variant, VAR_REAL);
            LUA_CONST(Variant, VAR_INT);
            LUA_CONST(Variant, VAR_VEC2);
            LUA_CONST(Variant, VAR_VEC3);
            LUA_CONST(Variant, VAR_VEC4);
            LUA_CONST(Variant, VAR_STRING);
        LUA_CONST_END;

        LuaScriptApi::getInstance()->addClassToCollection("Variant", "VAR_NULL", "Null type");
        LuaScriptApi::getInstance()->addClassToCollection("Variant", "VAR_BOOL", "Bool type");
        LuaScriptApi::getInstance()->addClassToCollection("Variant", "VAR_REAL", "Number type");
        LuaScriptApi::getInstance()->addClassToCollection("Variant", "VAR_INT", "Integer type");
        LuaScriptApi::getInstance()->addClassToCollection("Variant", "VAR_VEC2", "Vector2 type");
        LuaScriptApi::getInstance()->addClassToCollection("Variant", "VAR_VEC3", "Vector3 type");
        LuaScriptApi::getInstance()->addClassToCollection("Variant", "VAR_VEC4", "Vector4 type");
        LuaScriptApi::getInstance()->addClassToCollection("Variant", "VAR_STRING", "String type");

        module(lua)
        [
            class_<AttributesComponent, GameObjectComponent>("AttributesComponent")
            .def("getAttributeValueByIndex", &AttributesComponent::getAttributeValueByIndex)
            .def("getAttributeValueByName", &AttributesComponent::getAttributeValueByName)
            .def("getAttributeValueBool", (bool (AttributesComponent::*)(unsigned int))&AttributesComponent::getAttributeValueBool)
            .def("getAttributeValueNumber", (Ogre::Real (AttributesComponent::*)(unsigned int))&AttributesComponent::getAttributeValueReal)
            .def("getAttributeValueString", (Ogre::String (AttributesComponent::*)(unsigned int))&AttributesComponent::getAttributeValueString)
            .def("getAttributeValueId", (Ogre::String (AttributesComponent::*)(unsigned int))&AttributesComponent::getAttributeValueString)
            .def("getAttributeValueVector2", (Ogre::Vector2 (AttributesComponent::*)(unsigned int))&AttributesComponent::getAttributeValueVector2)
            .def("getAttributeValueVector3", (Ogre::Vector3 (AttributesComponent::*)(unsigned int))&AttributesComponent::getAttributeValueVector3)
            .def("getAttributeValueVector4", (Ogre::Vector4 (AttributesComponent::*)(unsigned int))&AttributesComponent::getAttributeValueVector4)

            .def("getAttributeValueBool2", (bool (AttributesComponent::*)(const Ogre::String&))&AttributesComponent::getAttributeValueBool)
            .def("getAttributeValueNumber2", (Ogre::Real (AttributesComponent::*)(const Ogre::String&))&AttributesComponent::getAttributeValueReal)
            .def("getAttributeValueString2", (Ogre::String (AttributesComponent::*)(const Ogre::String&))&AttributesComponent::getAttributeValueString)
            .def("getAttributeValueId2", (Ogre::String (AttributesComponent::*)(const Ogre::String&))&AttributesComponent::getAttributeValueString)
            .def("getAttributeValueVector22", (Ogre::Vector2 (AttributesComponent::*)(const Ogre::String&))&AttributesComponent::getAttributeValueVector2)
            .def("getAttributeValueVector32", (Ogre::Vector3 (AttributesComponent::*)(const Ogre::String&))&AttributesComponent::getAttributeValueVector3)
            .def("getAttributeValueVector42", (Ogre::Vector4 (AttributesComponent::*)(const Ogre::String&))&AttributesComponent::getAttributeValueVector4)

            .def("addAttributeBool", &addAttributeBool)
            .def("addAttributeNumber", &addAttributeFloat)
            .def("addAttributeString", &addAttributeString)
            .def("addAttributeId", &addAttributeString)
            .def("addAttributeVector2", &addAttributeVector2)
            .def("addAttributeVector3", &addAttributeVector3)
            .def("addAttributeVector4", &addAttributeVector4)

            .def("changeValueBool", &changeValueBool)
            .def("changeValueNumber", &changeValueFloat)
            .def("changeValueString", &changeValueString)
            .def("changeValueId", &changeValueString)
            .def("changeValueVector2", &changeValueVector2)
            .def("changeValueVector3", &changeValueVector3)
            .def("changeValueVector4", &changeValueVector4)

            .def("saveValues", &AttributesComponent::saveValues)
            .def("saveValue", (void (AttributesComponent::*)(const Ogre::String&, unsigned int, bool))&AttributesComponent::saveValue)
            .def("saveValue2", (void (AttributesComponent::*)(const Ogre::String&, Variant*, bool))&AttributesComponent::saveValue)
            .def("loadValues", &AttributesComponent::loadValues)
            .def("loadValue", (bool (AttributesComponent::*)(const Ogre::String&, unsigned int))&AttributesComponent::loadValue)
            .def("loadValue2", (bool (AttributesComponent::*)(const Ogre::String&, Variant*))&AttributesComponent::loadValue)

            .def("reactOnAttributeChanged", &AttributesComponent::reactOnAttributeChanged)
        ];

        LuaScriptApi::getInstance()->addClassToCollection("AttributesComponent", "class inherits GameObjectComponent", AttributesComponent::getStaticInfoText());
        LuaScriptApi::getInstance()->addClassToCollection("AttributesComponent", "Variant getAttributeValueByIndex(unsigned int index)", "Gets the attribute value as Variant by the given index.");
        LuaScriptApi::getInstance()->addClassToCollection("AttributesComponent", "Variant getAttributeValueByName(String name)", "Gets the attribute value as Variant by the given name.");
        LuaScriptApi::getInstance()->addClassToCollection("AttributesComponent", "bool getAttributeValueBool(unsigned int index)", "Gets the bool value by index.");
        LuaScriptApi::getInstance()->addClassToCollection("AttributesComponent", "number getAttributeValueNumber(unsigned int index)", "Gets the number value by index.");
        LuaScriptApi::getInstance()->addClassToCollection("AttributesComponent", "String getAttributeValueString(unsigned int index)", "Gets the string value by index.");
        LuaScriptApi::getInstance()->addClassToCollection("AttributesComponent", "String getAttributeValueId(unsigned int index)", "Gets the id value by index.");
        LuaScriptApi::getInstance()->addClassToCollection("AttributesComponent", "Vector2 getAttributeValueVector2(unsigned int index)", "Gets the Vector2 value by index.");
        LuaScriptApi::getInstance()->addClassToCollection("AttributesComponent", "Vector3 getAttributeValueVector3(unsigned int index)", "Gets the Vector3 value by index.");
        LuaScriptApi::getInstance()->addClassToCollection("AttributesComponent", "Vector4 getAttributeValueVector4(unsigned int index)", "Gets the Vector4 value by index.");
        LuaScriptApi::getInstance()->addClassToCollection("AttributesComponent", "bool getAttributeValueBool2(String name)", "Gets the bool value by name.");
        LuaScriptApi::getInstance()->addClassToCollection("AttributesComponent", "number getAttributeValueNumber2(String name)", "Gets the number value by name.");
        LuaScriptApi::getInstance()->addClassToCollection("AttributesComponent", "String getAttributeValueString2(String name)", "Gets the string value by name.");
        LuaScriptApi::getInstance()->addClassToCollection("AttributesComponent", "String getAttributeValueId2(String name)", "Gets the id value by name.");
        LuaScriptApi::getInstance()->addClassToCollection("AttributesComponent", "Vector2 getAttributeValueVector22(String name)", "Gets the Vector2 value by name.");
        LuaScriptApi::getInstance()->addClassToCollection("AttributesComponent", "Vector3 getAttributeValueVector32(String name)", "Gets the Vector3 value by name.");
        LuaScriptApi::getInstance()->addClassToCollection("AttributesComponent", "Vector4 getAttributeValueVector42(String name)", "Gets the Vector4 value by name.");

        LuaScriptApi::getInstance()->addClassToCollection("AttributesComponent", "void addAttributeBool(String name, bool value)", "Adds a bool attribute.");
        LuaScriptApi::getInstance()->addClassToCollection("AttributesComponent", "void addAttributeNumber(String name, number value)", "Adds a number attribute.");
        LuaScriptApi::getInstance()->addClassToCollection("AttributesComponent", "void addAttributeString(String name, String value)", "Adds a string attribute.");
        LuaScriptApi::getInstance()->addClassToCollection("AttributesComponent", "void addAttributeId(String name, String id)", "Adds an id attribute.");
        LuaScriptApi::getInstance()->addClassToCollection("AttributesComponent", "void addAttributeVector2(String name, Vector2 value)", "Adds a Vector2 attribute.");
        LuaScriptApi::getInstance()->addClassToCollection("AttributesComponent", "void addAttributeVector3(String name, Vector3 value)", "Adds a Vector3 attribute.");
        LuaScriptApi::getInstance()->addClassToCollection("AttributesComponent", "void addAttributeVector4(String name, Vector4 value)", "Adds a Vector4 attribute.");

        LuaScriptApi::getInstance()->addClassToCollection("AttributesComponent", "void changeValueBool(String name, bool value)", "Changes the bool attribute value by name.");
        LuaScriptApi::getInstance()->addClassToCollection("AttributesComponent", "void changeValueNumber(String name, number value)", "Changes the number attribute value by name.");
        LuaScriptApi::getInstance()->addClassToCollection("AttributesComponent", "void changeValueString(String name, String value)", "Changes the string attribute value by name.");
        LuaScriptApi::getInstance()->addClassToCollection("AttributesComponent", "void changeValueId(String name, String id)", "Changes the id attribute value by name.");
        LuaScriptApi::getInstance()->addClassToCollection("AttributesComponent", "void changeValueVector2(String name, Vector2 value)", "Changes the Vector2 attribute value by name.");
        LuaScriptApi::getInstance()->addClassToCollection("AttributesComponent", "void changeValueVector3(String name, Vector3 value)", "Changes the Vector3 attribute value by name.");
        LuaScriptApi::getInstance()->addClassToCollection("AttributesComponent", "void changeValueVector4(String name, Vector4 value)", "Changes the Vector4 attribute value by name.");

        LuaScriptApi::getInstance()->addClassToCollection("AttributesComponent", "void saveValues(String saveName, bool crypted)", "Saves all attribute values.");
        LuaScriptApi::getInstance()->addClassToCollection("AttributesComponent", "void saveValue(String saveName, unsigned int index, bool crypted)", "Saves the attribute value at the given index.");
        LuaScriptApi::getInstance()->addClassToCollection("AttributesComponent", "void saveValue2(String saveName, Variant attribute, bool crypted)", "Saves the attribute value for the given Variant.");
        LuaScriptApi::getInstance()->addClassToCollection("AttributesComponent", "bool loadValues(String saveName)", "Loads all attribute values.");
        LuaScriptApi::getInstance()->addClassToCollection("AttributesComponent", "bool loadValue(String saveName, unsigned int index)", "Loads the attribute value at the given index.");
        LuaScriptApi::getInstance()->addClassToCollection("AttributesComponent", "bool loadValue2(String saveName, Variant attribute)", "Loads the attribute value for the given Variant.");
        LuaScriptApi::getInstance()->addClassToCollection("AttributesComponent", "void reactOnAttributeChanged(func closure(String attributeName, Variant attribute))",
            "Registers a Lua closure that is called whenever any attribute value is changed via actualizeValue (e.g. from NOWA-Design or from C++). "
            "The closure receives the user-visible attribute name and the Variant holding the new value. "
            "Example: attributesComp:reactOnAttributeChanged(function(name, attr) if name == 'energy' then ... end end)");

        gameObjectClass.def("getAttributesComponent", (AttributesComponent * (*)(GameObject*)) & getAttributesComponent);
        gameObjectClass.def("getAttributesComponentFromIndex", (AttributesComponent * (*)(GameObject*, unsigned int)) & getAttributesComponentFromIndex);
        gameObjectClass.def("getAttributesComponentFromName", &getAttributesComponentFromName);

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "AttributesComponent getAttributesComponent()", "Gets the AttributesComponent of this game object.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "AttributesComponent getAttributesComponentFromIndex(unsigned int occurrenceIndex)", "Gets the AttributesComponent by occurrence index.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "AttributesComponent getAttributesComponentFromName(String name)", "Gets the AttributesComponent by name.");

        gameObjectControllerClass.def("castAttributesComponent", &GameObjectController::cast<AttributesComponent>);
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "AttributesComponent castAttributesComponent(AttributesComponent other)", "Casts an incoming type for Lua auto completion.");
    }

    // ---------------------------------------------------------------------------
    // Internal save/load helpers (unchanged logic, kept at end of file)
    // ---------------------------------------------------------------------------

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
        }
    }

    void AttributesComponent::internalSave(Ogre::String& content, unsigned int index)
    {
        size_t lineEndPos = 0;
        int remainingLength = static_cast<int>(content.size());

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

                if (true == firstRound)
                {
                    remainingLength = static_cast<int>(content.size() - tempContent.size() - lineEndPos) + 1;
                    firstRound = false;
                }
                else
                {
                    remainingLength = static_cast<int>(content.size() - tempContent.size() - lineEndPos);
                }

                Ogre::String priorContent;
                if (lineEndPos > 0)
                {
                    priorContent = content.substr(0, lineEndPos);
                }

                Ogre::String afterContent;
                if (remainingLength > 0)
                {
                    afterContent = content.substr(lineEndPos + tempContent.size(), static_cast<size_t>(remainingLength));
                }

                content = priorContent + tempContent + afterContent;
            }

            // Bug fix: was std::wstring::npos, must be Ogre::String::npos (std::string::npos)
            lineEndPos = content.find('\n', lineEndPos);
            if (Ogre::String::npos != lineEndPos)
            {
                lineEndPos += 1;
                tempIndex++;
            }
            else
            {
                break;
            }
        }
    }

    bool AttributesComponent::internalRead(std::istringstream& inFile)
    {
        bool success = false;
        Ogre::String line;
        Ogre::StringVector data;

        // parse til eof
        while (std::getline(inFile, line))
        {
            // Parse til next game object
            if (line.find("[") != Ogre::String::npos)
            {
                break;
            }

            data = Ogre::StringUtil::split(line, "=");
            if (data.size() < 3)
            {
                continue;
            }

            if (true == this->attributeNames.empty())
            {
                return false;
            }

            for (size_t i = 0; i < this->attributeNames.size(); i++)
            {
                // Check if its the correct name and set its value
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
        while (std::getline(inFile, line))
        {
            data = Ogre::StringUtil::split(line, "=");
            if (data.size() < 3)
            {
                continue;
            }

            if (true == this->attributeNames.empty())
            {
                return false;
            }

            if (index >= this->attributeNames.size())
            {
                return success;
            }

            // Check if its the correct name and set its value
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

                // Found the value to load, stop searching
                if (true == success)
                {
                    break;
                }
            }

            // Parse til next game object
            if (line.find("[") != Ogre::String::npos)
            {
                break;
            }
        }

        return success;
    }

}; // namespace end
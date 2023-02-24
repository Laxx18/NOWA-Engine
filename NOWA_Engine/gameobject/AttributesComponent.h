#ifndef ATTRIBUTES_COMPONENT_H
#define ATTRIBUTES_COMPONENT_H

#include "GameObjectComponent.h"
#include <map>

namespace NOWA
{
	class EXPORTED AttributesComponent : public GameObjectComponent
	{
	public:
		typedef boost::shared_ptr<AttributesComponent> AttributesCompPtr;
		friend class GameProgressModule;
		
	public:
	
		AttributesComponent();

		virtual ~AttributesComponent();

		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		virtual bool postInit(void) override;

		virtual Ogre::String getClassName(void) const override;

		virtual Ogre::String getParentClassName(void) const override;

		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("AttributesComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "AttributesComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		virtual void update(Ogre::Real dt, bool notSimulating = false) { };

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		/**
		 * @see	GameObjectComponent::getStaticInfoText
		 */
		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: A GameObject may have custom attributes like energy, coins etc. These attributes can also be used in lua scripts. They can also be saved and loaded.";
		}
		
		void setAttributesCount(unsigned int attributesCount);

		unsigned int getAttributesCount(void) const;

		Variant* getAttributeValueByIndex(unsigned int index) const;
		
		Variant* getAttributeValueByName(const Ogre::String& attributeName);

		void setAttributeName(const Ogre::String& attributeName, unsigned int index);

		Ogre::String getAttributeName(unsigned int index);
		
		void setAttributeType(const Ogre::String& attributeType, unsigned int index);

		Ogre::String getAttributeType(unsigned int index);

		template <typename T>
		void setAttributeValue(T attributeValue, unsigned int index)
		{
			if (index > this->attributeValues.size())
				return;
			this->attributeValues[index]->setValue(attributeValue);
		}

		bool getAttributeValueBool(unsigned int index);

		int getAttributeValueInt(unsigned int index);

		unsigned int getAttributeValueUInt(unsigned int index);

		unsigned long getAttributeValueULong(unsigned int index);

		Ogre::String getAttributeValueString(unsigned int index);

		Ogre::Real getAttributeValueReal(unsigned int index);

		Ogre::Vector2 getAttributeValueVector2(unsigned int index);

		Ogre::Vector3 getAttributeValueVector3(unsigned int index);

		Ogre::Vector4 getAttributeValueVector4(unsigned int index);

		bool getAttributeValueBool(const Ogre::String& name);

		int getAttributeValueInt(const Ogre::String& name);

		unsigned int getAttributeValueUInt(const Ogre::String& name);

		unsigned long getAttributeValueULong(const Ogre::String& name);

		Ogre::String getAttributeValueString(const Ogre::String& name);

		Ogre::Real getAttributeValueReal(const Ogre::String& name);

		Ogre::Vector2 getAttributeValueVector2(const Ogre::String& name);

		Ogre::Vector3 getAttributeValueVector3(const Ogre::String& name);

		Ogre::Vector4 getAttributeValueVector4(const Ogre::String& name);

		void setCloneWithInitValues(bool cloneWithInitValues);

		bool getCloneWithInitValues(void) const;
		
		void changeName(const Ogre::String& oldName, const Ogre::String& newName);

		template <typename T>
		bool addAttribute(const Ogre::String& attributeName, const T& newAttributeValue, Variant::Types attributeType)
		{
			if (attributeName.empty())
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AttributesComponent] Could not add attribute, because the attribute name is empty for game object: " 
					+ this->gameObjectPtr->getName());
				return false;
			}
			for (auto& attribute : this->attributes)
			{
				if (attribute.first == attributeName)
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AttributesComponent] Could not add attribute, because the attribute name: '" 
						+ attributeName + "' does already exist for game object: " + this->gameObjectPtr->getName());
					return false;
				}
			}
			this->setAttributesCount(this->attributesCount->getUInt() + 1);
			this->attributeNames[this->attributesCount->getUInt() - 1]->setValue(attributeName);

			Ogre::String strType;
			if (Variant::VAR_BOOL == attributeType)
			{
				strType = "Bool";
			}
			else if (Variant::VAR_INT == attributeType)
			{
				strType = "Int";
			}
			else if (Variant::VAR_UINT == attributeType)
			{
				strType = "UInt";
			}
			else if (Variant::VAR_ULONG == attributeType)
			{
				strType = "ULong";
			}
			else if (Variant::VAR_REAL == attributeType)
			{
				strType = "Float";
			}
			else if (Variant::VAR_STRING == attributeType)
			{
				strType = "String";
			}
			else if (Variant::VAR_VEC2 == attributeType)
			{
				strType = "Vector2";
			}
			else if (Variant::VAR_VEC3 == attributeType)
			{
				strType = "Vector3";
			}
			else if (Variant::VAR_VEC4 == attributeType)
			{
				strType = "Vector4";
			}
			this->attributeTypes[this->attributesCount->getUInt() - 1]->setListSelectedValue(strType);
			return true;
		}

		bool removeAttribute(const Ogre::String& attributeName);

		template <typename T>
		bool changeValue(const Ogre::String& attributeName, const T& newAttributeValue)
		{
			if (attributeName.empty())
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AttributesComponent] Could not change Value, because the attribute name is empty for game object: " 
					+ this->gameObjectPtr->getName());
				return false;
			}
			for (auto& attribute : this->attributes)
			{
				if (attribute.first == attributeName)
				{
					attribute.second->setValue(newAttributeValue);
					return true;
				}
			}
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AttributesComponent] Could not change Value, because the attribute value name: '" 
				+ attributeName + "' does not exist for game object: " + this->gameObjectPtr->getName());
			return false;
		}
		
		void changeType(const Ogre::String& attributeName, const Ogre::String& attributeType);

		void saveValues(const Ogre::String& saveName, bool crypted);

		void saveValue(const Ogre::String& saveName, unsigned int index, bool crypted);
		
		void saveValue(const Ogre::String& saveName, Variant* attribute, bool crypted);

		bool loadValues(const Ogre::String& saveName);

		bool loadValue(const Ogre::String& saveName, unsigned int index);
		
		bool loadValue(const Ogre::String& saveName, Variant* attribute);
	public:
		static const Ogre::String AttrAttributesCount(void) { return "Attributes Count"; }
		static const Ogre::String AttrAttributeName(void) { return "Attribute Name "; }
		static const Ogre::String AttrAttributeType(void) { return "Attribute Type "; }
		static const Ogre::String AttrAttributeValue(void) { return "Attribute Value "; }
	private:
		void internalSave(std::ostringstream& outFile);
		void internalSave(Ogre::String& content, unsigned int index);
		bool internalRead(std::istringstream& inFile);
		bool internalRead(std::istringstream& inFile, unsigned int index);
	protected:
		bool cloneWithInitValues;
	private:
		Variant* attributesCount;
		std::vector<Variant*> attributeNames;
		std::vector<Variant*> attributeTypes;
		std::vector<Variant*> attributeValues;
		
		// std::vector<std::pair<Ogre::String, Variant*>> userAttributes;
	};

}; //namespace end

#endif
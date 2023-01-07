#include "NOWAPrecompiled.h"
#include "GameObjectComponent.h"
#include "utilities/XMLConverter.h"
#include "LuaScriptComponent.h"

namespace
{
	Ogre::String getResourceFilePathName(const Ogre::String& resourceName)
	{
		if (true == resourceName.empty())
		{
			return "";
		}

		Ogre::Archive* archive = nullptr;
		try
		{
			Ogre::String resourceGroupName = Ogre::ResourceGroupManager::getSingletonPtr()->findGroupContainingResource(resourceName);
			archive = Ogre::ResourceGroupManager::getSingletonPtr()->_getArchiveToResource(resourceName, resourceGroupName);
		}
		catch (...)
		{
			return "";
		}

		if (nullptr != archive)
		{
			return archive->getName();
		}
		return "";
	}

	Ogre::String getDirectoryNameFromFilePathName(const Ogre::String& filePathName)
	{
		size_t pos = filePathName.find_last_of("\\/");
		return (std::string::npos == pos) ? "" : filePathName.substr(pos + 1, filePathName.size() - 1);
	}
}

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	GameObjectComponent::GameObjectComponent()
		: occurrenceIndex(0),
		index(0),
		bShowDebugData(false),
		bConnectedSuccess(true),
		name(new Variant(GameObjectComponent::AttrName(), "", this->attributes)),
		// Note: referenceId will not be cloned, because its to special. The designer must adapt each time the ids!
		referenceId(new Variant(GameObjectComponent::AttrReferenceId(), static_cast<unsigned long>(0), this->attributes, false))
	{
		// Because of naming collision detection and name adaptation
		this->name->addUserData(GameObject::AttrActionNeedRefresh());
	}

	GameObjectComponent::~GameObjectComponent()
	{
		// Delete all attributes
		auto& it = this->attributes.begin();

		while (it != this->attributes.end())
		{
			Variant* variant = it->second;
			delete variant;
			variant = nullptr;
			++it;
		}
		this->attributes.clear();
		// resets the shared ptr of the private GameObject ptr if a component gets deleted
		this->gameObjectPtr.reset();
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[GameObjectComponent] Reseting gameobject smart pointer");
	}

	bool GameObjectComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& file)
	{
		// One propagation must always be done, else an recursion does occur (if there is a component, that has nothing to load, this default function is called)
		propertyElement = propertyElement->next_sibling("property");

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Name")
		{
			this->name->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ReferenceId")
		{
			this->referenceId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		
		return true;
	}

	bool GameObjectComponent::postInit(void)
	{
		return true;
	}

	void GameObjectComponent::cloneBase(GameObjectCompPtr& otherGameObjectCompPtr)
	{
		otherGameObjectCompPtr->setReferenceId(this->referenceId->getULong());
		otherGameObjectCompPtr->setName(this->name->getString());
	}

	void GameObjectComponent::onOtherComponentRemoved(unsigned int index)
	{
		
	}

	void GameObjectComponent::onOtherComponentAdded(unsigned int index)
	{
	}

	bool GameObjectComponent::connect(void)
	{
		// If this game object has a lua script component, and it could not be compiled, its dangerous to connect this game object
		auto luaScriptComponent = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<LuaScriptComponent>());
		if (nullptr != luaScriptComponent)
		{
			if (true == luaScriptComponent->getLuaScript()->hasCompileError())
			{
				return false;
			}
		}

		return true;
	}

	void GameObjectComponent::onRemoveComponent(void)
	{
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(true);
	}

	void GameObjectComponent::actualizeValue(Variant* attribute)
	{
		if (GameObjectComponent::AttrName() == attribute->getName())
		{
			this->setName(attribute->getString());
		}
		else if (GameObjectComponent::AttrReferenceId() == attribute->getName())
		{
			this->setReferenceId(attribute->getULong());
		}
	}

	void GameObjectComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString("Component" + this->getClassName())));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->getClassName())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Name"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->name->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ReferenceId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->referenceId->getULong())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::Vector3 GameObjectComponent::getPosition(void) const
	{
		return this->gameObjectPtr->getPosition();
	}

	Ogre::Quaternion GameObjectComponent::getOrientation(void) const
	{
		return this->gameObjectPtr->getOrientation();
	}

	unsigned int GameObjectComponent::getClassId(void) const
	{
		return NOWA::getIdFromName(this->getClassName());
	}

	unsigned int GameObjectComponent::getParentClassId(void) const
	{
		return NOWA::getIdFromName(this->getParentClassName());
	}

	unsigned int GameObjectComponent::getParentParentClassId(void) const
	{
		return NOWA::getIdFromName(this->getParentParentClassName());
	}

	void GameObjectComponent::showDebugData(void)
	{
		this->bShowDebugData = !this->bShowDebugData;
	}

	bool GameObjectComponent::getShowDebugData(void) const
	{
		return this->bShowDebugData;
	}
	
	GameObjectPtr GameObjectComponent::getOwner(void) const
	{
		return this->gameObjectPtr;
	}

	void GameObjectComponent::setOwner(GameObjectPtr gameObjectPtr)
	{
		this->gameObjectPtr = gameObjectPtr;
	}

	Variant* GameObjectComponent::getAttribute(const Ogre::String& attributeName)
	{
		for (unsigned int i = 0; i < static_cast<unsigned int>(this->attributes.size()); i++)
		{
			if (this->attributes[i].first == attributeName)
			{
				return this->attributes[i].second;
			}
		}
		return nullptr;
	}

	std::vector<std::pair<Ogre::String, Variant*>>& GameObjectComponent::getAttributes(void)
	{
		return this->attributes;
	}

	unsigned int GameObjectComponent::getOccurrenceIndex(void) const
	{
		return this->occurrenceIndex;
	}

	unsigned int GameObjectComponent::getIndex(void) const
	{
		return this->index;
	}

	void GameObjectComponent::setName(const Ogre::String& name)
	{
		Ogre::String validatedName = name;
		this->getValidatedComponentName(validatedName);
		this->name->setValue(validatedName);
	}

	Ogre::String GameObjectComponent::getName(void) const
	{
		return this->name->getString();
	}

	void GameObjectComponent::setReferenceId(unsigned long referenceId)
	{
		this->referenceId->setValue(referenceId);
	}

	unsigned long GameObjectComponent::getReferenceId(void) const
	{
		return this->referenceId->getULong();
	}

	void GameObjectComponent::eraseVariants(std::vector<Variant*>& container, size_t offset)
	{
		for (auto it = container.begin() + offset; it != container.end();)
		{
			Variant* variant = *it;
			for (size_t i = 0; i < this->attributes.size(); i++)
			{
				// Also erase from attributes list
				if (this->attributes[i].second == variant)
				{
					this->attributes.erase(this->attributes.begin() + i);
					break;
				}
			}
			it = container.erase(it);
			delete variant;
		}
	}

	void GameObjectComponent::eraseVariant(std::vector<Variant*>& container, size_t index)
	{
		Variant* variant = container[index];
		for (size_t i = 0; i < this->attributes.size(); i++)
		{
			// Also erase from attributes list
			if (this->attributes[i].second == variant)
			{
				this->attributes.erase(this->attributes.begin() + i);
				break;
			}
		}
		container.erase(container.begin() + index);
		delete variant;
	}

	void GameObjectComponent::resetVariants()
	{
		for (size_t i = 0; i < this->attributes.size(); i++)
		{
			// Only set old values for all attributes, that have changed in simulation, or if custom data string is set to change all attributes for this component
			if (true == this->attributes[i].second->hasChanged() || customDataString == GameObjectComponent::AttrCustomDataNewCreation())
			{
				this->actualizeValue(this->attributes[i].second);
			}
			this->attributes[i].second->resetChange();
		}
	}

	void GameObjectComponent::resetChanges()
	{
		for (size_t i = 0; i < this->attributes.size(); i++)
		{
			this->attributes[i].second->resetChange();
		}
	}

	void GameObjectComponent::getValidatedComponentName(Ogre::String& componentName)
	{
		auto& gameObjectComponentPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponentFromName<GameObjectComponent>(componentName));
		if (nullptr == gameObjectComponentPtr)
			return;
		else if (gameObjectComponentPtr.get() == this)
			return;
		else
		{
			unsigned int id = 0;
			Ogre::String validatedComponentName = componentName;
			do
			{
				size_t found = validatedComponentName.rfind("_");
				if (Ogre::String::npos != found)
				{
					validatedComponentName = validatedComponentName.substr(0, found + 1);
				}
				else
				{
					validatedComponentName += "_";
				}
				validatedComponentName += Ogre::StringConverter::toString(id++);
			} while (nullptr != NOWA::makeStrongPtr(this->gameObjectPtr->getComponentFromName<GameObjectComponent>(validatedComponentName)));
			componentName = validatedComponentName;
		}
	}

	void GameObjectComponent::addAttributeFilePathData(Variant* attribute)
	{
		Ogre::String textureFilePathName = getResourceFilePathName(attribute->getString());
		Ogre::String folder = getDirectoryNameFromFilePathName(textureFilePathName);
		attribute->setDescription("Texture location: '" + getResourceFilePathName(attribute->getString()) + "' ");
		if (false == attribute->hasUserDataKey(GameObject::AttrActionFileOpenDialog()))
		{
			attribute->setUserData(GameObject::AttrActionFileOpenDialog(), folder);
		}
	}

}; // namespace end
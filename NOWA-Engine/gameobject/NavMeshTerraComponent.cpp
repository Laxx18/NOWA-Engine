#include "NOWAPrecompiled.h"
#include "NavMeshTerraComponent.h"
#include "utilities/XMLConverter.h"
#include "main/AppStateManager.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	NavMeshTerraComponent::NavMeshTerraComponent()
		: GameObjectComponent(),
		activated(new Variant(NavMeshTerraComponent::AttrActivated(), true, this->attributes))
	{
	
	}

	NavMeshTerraComponent::~NavMeshTerraComponent()
	{
		AppStateManager::getSingletonPtr()->getOgreRecastModule()->removeTerra(this->gameObjectPtr->getId());

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[NavMeshTerraComponent] Destructor navigation mesh terra component for game object: "
			+ this->gameObjectPtr->getName());
	}

	bool NavMeshTerraComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	GameObjectCompPtr NavMeshTerraComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		return NavMeshTerraCompPtr();
	}

	bool NavMeshTerraComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[NavMeshTerraComponent] Init navigation mesh terra component for game object: "
			+ this->gameObjectPtr->getName());
		
		this->setActivated(this->activated->getBool());

		return true;
	}

	bool NavMeshTerraComponent::connect(void)
	{
		this->setActivated(this->activated->getBool());
		return true;
	}

	bool NavMeshTerraComponent::disconnect(void)
	{
		
		return true;
	}

	void NavMeshTerraComponent::update(Ogre::Real dt, bool notSimulating)
	{
		
	}

	void NavMeshTerraComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (NavMeshTerraComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
	}

	void NavMeshTerraComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->activated->getBool())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String NavMeshTerraComponent::getClassName(void) const
	{
		return "NavMeshTerraComponent";
	}

	Ogre::String NavMeshTerraComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void NavMeshTerraComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
		
		if (true == this->activated->getBool())
			AppStateManager::getSingletonPtr()->getOgreRecastModule()->addTerra(this->gameObjectPtr->getId());
		else
			AppStateManager::getSingletonPtr()->getOgreRecastModule()->removeTerra(this->gameObjectPtr->getId(), true);
	}

	bool NavMeshTerraComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

}; // namespace end
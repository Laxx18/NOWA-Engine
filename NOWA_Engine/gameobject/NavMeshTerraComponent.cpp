#include "NOWAPrecompiled.h"
#include "NavMeshTerraComponent.h"
#include "utilities/XMLConverter.h"
#include "main/AppStateManager.h"

#include <regex>

namespace
{
	std::vector<Ogre::String> split(const Ogre::String& str)
	{
		std::vector<Ogre::String> values;
		std::regex rgx("([0-9]{1,3})(?:,([0-9]{1,3}))(?:,([0-9]{1,3}))(?:,([0-9]{1,3}))");
		std::smatch baseMatch;
		std::regex_search(str, baseMatch, rgx);
		if (baseMatch.size() == 5)
		{
			for (size_t i = 0; i < 4; i++)
			{
				values.emplace_back(baseMatch[i + 1]);
			}
		}
		return values;
	}

	std::mutex mutex;
}

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	NavMeshTerraComponent::NavMeshTerraComponent()
		: GameObjectComponent(),
		activated(new Variant(NavMeshTerraComponent::AttrActivated(), true, this->attributes)),
		terraLayers(new Variant(NavMeshTerraComponent::AttrTerraLayers(), Ogre::String("255,255,255,255"), this->attributes))
	{
		this->terraLayers->setDescription("Sets the terra layer thresholds, on which no navigation points shall be set. Terra has 4 layers and maximum threshold is 255. E.g. setting to: 255,255,0,255 will set navigation points "
										  "on all layers but layer 3. Setting e.g. 255,122,0,0 would not make sense, as valid values are 0 or 255.");
	}

	NavMeshTerraComponent::~NavMeshTerraComponent()
	{
		AppStateManager::getSingletonPtr()->getOgreRecastModule()->removeTerra(this->gameObjectPtr->getId());

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[NavMeshTerraComponent] Destructor navigation mesh terra component for game object: "
			+ this->gameObjectPtr->getName());
	}

	bool NavMeshTerraComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TerraLayers")
		{
			this->terraLayers->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			checkAndSetTerraLayers(this->terraLayers->getString());
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
		else if (NavMeshTerraComponent::AttrTerraLayers() == attribute->getName())
		{
			this->setTerraLayers(attribute->getString());
		}
	}

	void NavMeshTerraComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("name", "TerraLayers"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->terraLayers->getString())));
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

	void NavMeshTerraComponent::checkAndSetTerraLayers(const Ogre::String& terraLayers)
	{
		this->terraLayerList.clear();
		auto strLayers = split(terraLayers);
		if (true == strLayers.empty())
		{
			this->terraLayerList = { 255, 255, 255, 255 };
		}
		for (size_t i = 0; i < strLayers.size(); i++)
		{
			int value = Ogre::StringConverter::parseInt(strLayers[i]);
			this->terraLayerList.emplace_back(value > 255 ? 255 : value);
		}
	}

	void NavMeshTerraComponent::setTerraLayers(const Ogre::String& terraLayers)
	{
		this->checkAndSetTerraLayers(terraLayers);
		if (4 == this->terraLayerList.size())
		{
			this->terraLayers->setValue(terraLayers);
		}
		else
		{
			this->terraLayers->setValue(Ogre::String("255,255,255,255"));
		}
	}

	Ogre::String NavMeshTerraComponent::getTerraLayers(void) const
	{
		return this->terraLayers->getString();
	}

	std::vector<int> NavMeshTerraComponent::getTerraLayerList(void) const
	{
		return this->terraLayerList;
	}

}; // namespace end
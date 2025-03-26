#include "NOWAPrecompiled.h"
#include "VegetationComponent.h"
#include "utilities/XMLConverter.h"
#include "modules/LuaScriptApi.h"
#include "main/EventManager.h"
#include "main/AppStateManager.h"
#include "gameobject/GameObjectFactory.h"
#include "gameobject/TerraComponent.h"

#include <regex>
#include <mutex>

#include "OgreAbiUtils.h"

namespace
{
	std::vector<Ogre::String> split(const Ogre::String& str)
	{
		std::vector<Ogre::String> values;
		std::regex rgx("\\s*,\\s*"); // Match commas with optional spaces
		std::sregex_token_iterator iter(str.begin(), str.end(), rgx, -1);
		std::sregex_token_iterator end;

		for (; iter != end; ++iter)
		{
			values.emplace_back(*iter);
		}

		return values;
	}
}

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	VegetationComponent::VegetationComponent()
		: GameObjectComponent(),
		name("VegetationComponent"),
		minimumBounds(Ogre::Vector3::ZERO),
		maximumBounds(Ogre::Vector3::ZERO),
		raySceneQuery(nullptr),
		activated(new Variant(VegetationComponent::AttrActivated(), true, this->attributes)),
		targetGameObjectId(new Variant(VegetationComponent::AttrTargetGameObjectId(), static_cast<unsigned long>(0), this->attributes)),
		density(new Variant(VegetationComponent::AttrDensity(), 1.0f, this->attributes)),
		positionXZ(new Variant(VegetationComponent::AttrPositionXZ(), Ogre::Vector2::ZERO, this->attributes)),
		scale(new Variant(VegetationComponent::AttrScale(), 1.0f, this->attributes)),
		autoOrientation(new Variant(VegetationComponent::AttrAutoOrientation(), false, this->attributes)),
		categories(new Variant(VegetationComponent::AttrCategories(), Ogre::String("All"), this->attributes)),
		maxHeight(new Variant(VegetationComponent::AttrMaxHeight(), 0.0f, this->attributes)),
		renderDistance(new Variant(VegetationComponent::AttrRenderDistance(), 30.0f, this->attributes)),
		terraLayers(new Variant(VegetationComponent::AttrTerraLayers(), Ogre::String("255,255,255,255"), this->attributes)),
		vegetationTypesCount(new Variant(VegetationComponent::AttrVegetationTypesCount(), static_cast<unsigned int>(0), this->attributes))
	{
		this->targetGameObjectId->setDescription("Sets the target game object id, which is used for planting vegetation. If not set (id = 0), the whole world is used.");
		this->positionXZ->setDescription("Sets the x-z position of the vegetation. Combining it with scale, its possible to just vegetate a specifig area.");
		this->scale->setDescription("Sets Scale. Combining it with position, its possible to just vegetate a specifig area. Default is 1, which is the whole area of the target game object id.");
		this->autoOrientation->setDescription("Sets whether auto orientate the vegetation according to the slope.");
		this->maxHeight->setDescription("Sets the maximum height in meter, until which the vegetation is generated. 0 means: There is no limit. E.g. a high mountain which has white peaks shall have no grass^^.");
		this->renderDistance->setDescription("Sets render distance in meters, until which the vegetation is visible. 0 means: There is no limit.");
		this->terraLayers->setDescription("Sets the terra layer thresholds, on which the vegetation shall be placed. Terra has 4 layers and maximum threshold is 255. E.g. setting to: 255,255,0,255 will place vegetation "
										  "on all layers but layer 3. Setting e.g. 255,122,0,0 would place vegetation just on layer 1 and 2, but on layer 2 even the layer 2 painting is not that strong (half of max). "
										  "Note: Only usable if terra is involved.");
		this->vegetationTypesCount->setDescription("Sets the vegetation count, which kind of game objects shall be placed.");
		this->vegetationTypesCount->addUserData(GameObject::AttrActionNeedRefresh());
	}

	VegetationComponent::~VegetationComponent(void)
	{

	}

	void VegetationComponent::initialise()
	{
		
	}

	const Ogre::String& VegetationComponent::getName() const
	{
		return this->name;
	}

	void VegetationComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<VegetationComponent>(VegetationComponent::getStaticClassId(), VegetationComponent::getStaticClassName());
	}

	void VegetationComponent::shutdown()
	{
		// Do nothing here, because its called far to late and nothing is there of NOWA-Engine anymore! Use @onRemoveComponent in order to destroy something.
	}

	void VegetationComponent::uninstall()
	{
		// Do nothing here, because its called far to late and nothing is there of NOWA-Engine anymore! Use @onRemoveComponent in order to destroy something.
	}

	void VegetationComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool VegetationComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TargetGameObjectId")
		{
			this->targetGameObjectId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Density")
		{
			this->density->setValue(XMLConverter::getAttribReal(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "PositionXZ")
		{
			this->positionXZ->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Scale")
		{
			this->scale->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AutoOrientation")
		{
			this->autoOrientation->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Categories")
		{
			this->categories->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MaxHeight")
		{
			this->maxHeight->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RenderDistance")
		{
			this->renderDistance->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TerraLayers")
		{
			this->terraLayers->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			checkAndSetTerraLayers(this->terraLayers->getString());
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "VegetationTypesCount")
		{
			this->vegetationTypesCount->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		// Only create new variant, if fresh loading. If snapshot is done, no new variant
		// must be created! Because the algorithm is working changed flag of each existing variant!
		if (this->vegetationMeshNames.size() < this->vegetationTypesCount->getUInt())
		{
			this->vegetationMeshNames.resize(this->vegetationTypesCount->getUInt());
		}

		for (size_t i = 0; i < this->vegetationMeshNames.size(); i++)
		{
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "VegetationMeshName" + Ogre::StringConverter::toString(i))
			{
				Ogre::String generationMeshName = XMLConverter::getAttrib(propertyElement, "data");
				// List will be filled in postInit, in which the entity is available, but set the selected animation name string now, even the list is yet empty
				if (nullptr == this->vegetationMeshNames[i])
				{
					this->vegetationMeshNames[i] = new Variant(VegetationComponent::AttrVegetationMeshName() + Ogre::StringConverter::toString(i), std::vector<Ogre::String>(), this->attributes);
					this->vegetationMeshNames[i]->setValue(generationMeshName);
					this->vegetationMeshNames[i]->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");
					this->vegetationMeshNames[i]->setDescription("Either use the mesh name directly. In this case due to performance reasons no game objects will be created but the entities/items directly. Useful for grass e.g.");
				}
				else
				{
					this->vegetationMeshNames[i]->setValue(generationMeshName);
				}
				propertyElement = propertyElement->next_sibling("property");
			}
		}

		return true;
	}

	GameObjectCompPtr VegetationComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		return GameObjectCompPtr();
	}

	bool VegetationComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[VegetationComponent] Init component for game object: " + this->gameObjectPtr->getName());

		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &VegetationComponent::handleUpdateBounds), EventDataBoundsUpdated::getStaticEventType());
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &VegetationComponent::handleSceneParsed), NOWA::EventDataSceneParsed::getStaticEventType());


		if (nullptr == this->raySceneQuery)
		{
			this->raySceneQuery = this->gameObjectPtr->getSceneManager()->createRayQuery(Ogre::Ray(), GameObjectController::ALL_CATEGORIES_ID);
			this->raySceneQuery->setSortByDistance(true);
		}

		// Not here, because not everything has been loaded yet!
		// this->regenerateVegetation();

		return true;
	}

	bool VegetationComponent::connect(void)
	{


		return true;
	}

	bool VegetationComponent::disconnect(void)
	{

		return true;
	}

	bool VegetationComponent::onCloned(void)
	{

		return true;
	}

	void VegetationComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();

		this->gameObjectPtr->getSceneManager()->destroyQuery(this->raySceneQuery);
		this->clearLists();

		boost::shared_ptr<EventDataRefreshGui> eventDataRefreshGui(new EventDataRefreshGui());
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataRefreshGui);

		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &VegetationComponent::handleSceneParsed), NOWA::EventDataSceneParsed::getStaticEventType());
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &VegetationComponent::handleUpdateBounds), EventDataBoundsUpdated::getStaticEventType());
	}

	void VegetationComponent::onOtherComponentRemoved(unsigned int index)
	{

	}

	void VegetationComponent::onOtherComponentAdded(unsigned int index)
	{

	}

	void VegetationComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
			// Do something
		}
	}

	void VegetationComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (VegetationComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (VegetationComponent::AttrTargetGameObjectId() == attribute->getName())
		{
			this->setTargetGameObjectId(attribute->getULong());
		}
		else if (VegetationComponent::AttrDensity() == attribute->getName())
		{
			this->setDensity(attribute->getReal());
		}
		else if (VegetationComponent::AttrPositionXZ() == attribute->getName())
		{
			this->setPositionXZ(attribute->getVector2());
		}
		else if (VegetationComponent::AttrScale() == attribute->getName())
		{
			this->setScale(attribute->getReal());
		}
		else if (VegetationComponent::AttrAutoOrientation() == attribute->getName())
		{
			this->setAutoOrientation(attribute->getBool());
		}
		else if (VegetationComponent::AttrCategories() == attribute->getName())
		{
			this->setCategories(attribute->getString());
		}
		else if (VegetationComponent::AttrMaxHeight() == attribute->getName())
		{
			this->setMaxHeight(attribute->getReal());
		}
		else if (VegetationComponent::AttrRenderDistance() == attribute->getName())
		{
			this->setRenderDistance(attribute->getReal());
		}
		else if (VegetationComponent::AttrTerraLayers() == attribute->getName())
		{
			this->setTerraLayers(attribute->getString());
		}
		else if(VegetationComponent::AttrVegetationTypesCount() == attribute->getName())
		{
			this->setVegetationTypesCount(attribute->getUInt());
		}
		else
		{
			for (unsigned int i = 0; i < static_cast<unsigned int>(this->vegetationMeshNames.size()); i++)
			{
				if (VegetationComponent::AttrVegetationMeshName() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setVegetationMeshName(i, attribute->getString());
				}
			}
		}
	}

	void VegetationComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TargetGameObjectId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->targetGameObjectId->getULong())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Density"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->density->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "PositionXZ"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->positionXZ->getVector2())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Scale"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->scale->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AutoOrientation"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->autoOrientation->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Categories"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->categories->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MaxHeight"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxHeight->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RenderDistance"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->renderDistance->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TerraLayers"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->terraLayers->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "VegetationTypesCount"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->vegetationTypesCount->getUInt())));
		propertiesXML->append_node(propertyXML);

		for (size_t i = 0; i < this->vegetationMeshNames.size(); i++)
		{
			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "VegetationMeshName" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->vegetationMeshNames[i]->getString())));
			propertiesXML->append_node(propertyXML);
		}
	}

	Ogre::String VegetationComponent::getClassName(void) const
	{
		return "VegetationComponent";
	}

	Ogre::String VegetationComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void VegetationComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
		if (true == activated)
		{
			regenerateVegetation();
		}
		else
		{
			this->clearLists();
		}
	}

	bool VegetationComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void VegetationComponent::setTargetGameObjectId(unsigned long targetGameObjectId)
	{
		this->targetGameObjectId->setValue(targetGameObjectId);

		this->regenerateVegetation();
	}

	unsigned long VegetationComponent::getTargetGameObjectId(void) const
	{
		return this->targetGameObjectId->getULong();
	}

	void VegetationComponent::setDensity(Ogre::Real density)
	{
		if (density < 0.0f)
		{
			density = 0.1f;
		}
		else if (density > 5.0f)
		{
			density = 5.0f;
		}

		this->density->setValue(density);

		this->regenerateVegetation();
	}

	Ogre::Real VegetationComponent::getDensity(void) const
	{
		return this->density->getReal();
	}

	void VegetationComponent::setPositionXZ(const Ogre::Vector2& positionXZ)
	{
		this->positionXZ->setValue(positionXZ);
		this->regenerateVegetation();
	}

	Ogre::Vector2 VegetationComponent::getPositionXZ(void) const
	{
		return this->positionXZ->getVector2();
	}

	void VegetationComponent::setScale(Ogre::Real scale)
	{
		if (scale < 0.1f)
		{
			scale = 0.1f;
		}
		this->scale->setValue(scale);
		this->regenerateVegetation();
	}

	Ogre::Real VegetationComponent::getScale(void) const
	{
		return this->scale->getReal();
	}

	void VegetationComponent::setAutoOrientation(bool autoOrientation)
	{
		this->autoOrientation->setValue(autoOrientation);
		this->regenerateVegetation();
	}

	bool VegetationComponent::getAutoOrientation(void) const
	{
		return this->autoOrientation->getBool();
	}

	void VegetationComponent::setCategories(const Ogre::String& categories)
	{
		this->categories->setValue(categories);
		this->regenerateVegetation();
	}

	Ogre::String VegetationComponent::getCategories(void) const
	{
		return this->categories->getString();
	}

	void VegetationComponent::setMaxHeight(Ogre::Real maxHeight)
	{
		if (maxHeight < 0.0f)
		{
			maxHeight = 0.0f;
		}
		this->maxHeight->setValue(maxHeight);
		this->regenerateVegetation();
	}

	Ogre::Real VegetationComponent::getMaxHeight(void) const
	{
		return this->maxHeight->getReal();
	}

	void VegetationComponent::setRenderDistance(Ogre::Real renderDistance)
	{
		if (renderDistance < 0.0f)
		{
			renderDistance = 0.0f;
		}
		this->renderDistance->setValue(renderDistance);
		this->regenerateVegetation();
	}

	Ogre::Real VegetationComponent::getRenderDistance(void) const
	{
		return this->renderDistance->getReal();
	}

	void VegetationComponent::checkAndSetTerraLayers(const Ogre::String& terraLayers)
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

	void VegetationComponent::setTerraLayers(const Ogre::String& terraLayers)
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
		this->regenerateVegetation();
	}

	Ogre::String VegetationComponent::getTerraLayers(void) const
	{
		return this->terraLayers->getString();
	}

	void VegetationComponent::setVegetationTypesCount(unsigned int vegetationTypesCount)
	{
		this->vegetationTypesCount->setValue(vegetationTypesCount);

		size_t oldSize = this->vegetationMeshNames.size();

		if (vegetationTypesCount > oldSize)
		{
			// Resize the waypoints array for count
			this->vegetationMeshNames.resize(vegetationTypesCount);

			for (size_t i = oldSize; i < vegetationTypesCount; i++)
			{
				this->vegetationMeshNames[i] = new Variant(VegetationComponent::AttrVegetationMeshName() + Ogre::StringConverter::toString(i), Ogre::String(), this->attributes);
				this->vegetationMeshNames[i]->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");
				this->vegetationMeshNames[i]->setDescription("Either use the mesh name directly. In this case due to performance reasons no game objects will be created but the entities/items directly. Useful for grass e.g.");
			}
		}
		else if (vegetationTypesCount < oldSize)
		{
			this->eraseVariants(this->vegetationMeshNames, vegetationTypesCount);
		}

		this->regenerateVegetation();
	}

	unsigned int VegetationComponent::getVegetationTypesCount(void) const
	{
		return this->vegetationTypesCount->getUInt();
	}

	void VegetationComponent::setVegetationMeshName(unsigned int index, const Ogre::String& vegetationMeshName)
	{
		if (index > this->vegetationMeshNames.size())
			index = static_cast<unsigned int>(this->vegetationMeshNames.size()) - 1;

		this->vegetationMeshNames[index]->setValue(vegetationMeshName);

		this->regenerateVegetation();
	}

	Ogre::String VegetationComponent::getVegetationMeshName(unsigned int index) const
	{
		if (index > this->vegetationMeshNames.size())
			return "";
		return this->vegetationMeshNames[index]->getListSelectedValue();
	}

	void VegetationComponent::clearLists(void)
	{
		for (size_t i = 0; i < this->vegetationItemList.size(); i++)
		{
			Ogre::Item* item = this->vegetationItemList[i];
			Ogre::SceneNode* node = item->getParentSceneNode();
			if (nullptr != node)
			{
				auto nodeIt = node->getChildIterator();
				while (nodeIt.hasMoreElements())
				{
					//go through all scenenodes in the scene
					Ogre::Node* subNode = nodeIt.getNext();
					subNode->removeAllChildren();
				}

				node->detachAllObjects();

				this->gameObjectPtr->getSceneManager()->destroySceneNode(node);

				this->gameObjectPtr->getSceneManager()->destroyMovableObject(item);
			}
		}

		this->vegetationItemList.clear();
	}

//	void VegetationComponent::regenerateVegetation()
//	{
//		if (false == this->activated->getBool())
//		{
//			return;
//		}
//
//		if (nullptr == this->gameObjectPtr)
//		{
//			return;
//		}
//
//		if (GameObject::AttrCustomDataSkipCreation() == this->gameObjectPtr->getCustomDataString())
//		{
//		 	return;
//		}
//
//		// Reset variants when stopping simulation, would re-generate everything, which is bad, hence use skip flag
//		if (GameObject::AttrCustomDataSkipCreation() == this->gameObjectPtr->getCustomDataString())
//		{
//			return;
//		}
//
//		// First clear everything, and only regenerate if at least either on veg id, or mesh name is available
//		// e.g. if reducing count or increasing count, only regenerate if there are some game objects or meshes set.
//		this->clearLists();
//
//		if (vegetationTypesCount->getUInt() == 0)
//		{
//			return;
//		}
//
//		bool validData = true;
//
//		for (size_t i = 0; i < this->vegetationTypesCount->getUInt(); i++)
//		{
//			Ogre::String name = this->vegetationMeshNames[i]->getString();
//
//			validData &= (false == name.empty());
//		}
//
//		if (false == validData)
//		{
//			return;
//		}
//
//		unsigned int categoryId = AppStateManager::getSingletonPtr()->getGameObjectController()->generateCategoryId(this->categories->getString());
//		this->raySceneQuery->setQueryMask(categoryId);
//
//		Ogre::Terra* terra = nullptr;
//
//		GameObjectPtr targetGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->targetGameObjectId->getULong());
//		if (nullptr != targetGameObjectPtr)
//		{
//			auto terraCompPtr = NOWA::makeStrongPtr(targetGameObjectPtr->getComponent<TerraComponent>());
//			if (nullptr != terraCompPtr)
//			{
//				terra = terraCompPtr->getTerra();
//			}
//		}
//
//		Ogre::Vector2 bounds = Ogre::Vector2::ZERO;
//
//		if (nullptr != targetGameObjectPtr)
//		{
//			this->minimumBounds = targetGameObjectPtr->getMovableObject()->getWorldAabb().getMinimum();
//			this->maximumBounds = targetGameObjectPtr->getMovableObject()->getWorldAabb().getMaximum();
//		}
//		else
//		{
//
//			auto terraList = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectsFromComponent(TerraComponent::getStaticClassName());
//			if (false == terraList.empty())
//			{
//				// TODO: What if later x-terras?
//				auto terraCompPtr = NOWA::makeStrongPtr(terraList[0]->getComponent<TerraComponent>());
//				if (nullptr != terraCompPtr)
//				{
//					terra = terraCompPtr->getTerra();
//				}
//			}
//		}
//
//		Ogre::Real sizeX = 1.0f;
//		Ogre::Real sizeZ = 1.0f;
//		unsigned long createdCount = 0;
//
//
//		Ogre::Camera* camera = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
//
//		std::vector<Ogre::MeshPtr> meshes;
//
//		for (size_t i = 0; i < this->vegetationMeshNames.size(); i++)
//		{
//			if (false == this->vegetationMeshNames[i]->getString().empty())
//			{
//				// Note: Even meshPtr points to an ogre v1 entity mesh, it will be transformed to ogre v2 item!
//				auto v1Mesh = Ogre::v1::MeshManager::getSingletonPtr()->load(this->vegetationMeshNames[i]->getString(), Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME, Ogre::v1::HardwareBuffer::HBU_STATIC, Ogre::v1::HardwareBuffer::HBU_STATIC);
//
//				// Destroy a potential plane v2, because an error occurs (plane with name ... already exists)
//				Ogre::ResourcePtr resourceV2 = Ogre::MeshManager::getSingletonPtr()->getResourceByName(this->vegetationMeshNames[i]->getString());
//				if (nullptr != resourceV2)
//				{
//					Ogre::MeshManager::getSingletonPtr()->destroyResourcePool(this->vegetationMeshNames[i]->getString());
//					Ogre::MeshManager::getSingletonPtr()->remove(resourceV2->getHandle());
//				}
//
//				auto v2Mesh = Ogre::MeshManager::getSingletonPtr()->createByImportingV1(this->vegetationMeshNames[i]->getString(), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, v1Mesh.get(), true, true, true);
//				v1Mesh->unload();
//				v1Mesh.setNull();
//
//				meshes.emplace_back(v2Mesh);
//			}
//			else
//			{
//				meshes.emplace_back(nullptr);
//			}
//		}
//
//		unsigned int vegetationQueryFlags = AppStateManager::getSingletonPtr()->getGameObjectController()->generateCategoryId(this->gameObjectPtr->getCategory());
//
//		const Ogre::Real size = Ogre::Math::Abs(this->minimumBounds.x) * this->scale->getReal() + this->positionXZ->getVector2().x + Ogre::Math::Abs(this->maximumBounds.x) * this->scale->getReal() + this->positionXZ->getVector2().x;
//		const Ogre::Real step = sizeX / this->density->getReal();
//		const size_t numThreads = std::max<size_t>(1, Ogre::PlatformInformation::getNumLogicalCores());
//		size_t partitionSize = size / numThreads;
//		size_t partitionIncrement = 0;
//		size_t rest = MathHelper::getInstance()->wrappedModulo(size, numThreads);
//
//		Ogre::Real fractionStart = 0.0f;
//		Ogre::Real fractionEnd = 0.0f;
//
//		auto start = std::chrono::high_resolution_clock::now();
//
//		// With threading takes approx. 11 seconds. Half the time in debug mode!
//#if 1
//
//		std::vector<std::thread> threads;
//		for (size_t i = 0; i < numThreads; i++)
//		{
//			Ogre::Real fractionStart = ((this->minimumBounds.x * this->scale->getReal() + this->positionXZ->getVector2().x) + partitionIncrement);
//			Ogre::Real fractionEnd = ((this->minimumBounds.x * this->scale->getReal() + this->positionXZ->getVector2().x) + partitionSize + partitionIncrement);
//
//			if (i == numThreads - 1)
//			{
//				fractionEnd = fractionEnd + rest;
//			}
//
//			threads.emplace_back(std::thread([this, categoryId, terra, &meshes, camera, vegetationQueryFlags, targetGameObjectPtr, &createdCount, fractionStart, fractionEnd, step]()
//								 {
//									 mutex.lock();
//									 Ogre::Real sizeX = 1.0f;
//									 Ogre::Real sizeZ = 1.0f;
//
//									 for (Ogre::Real x = fractionStart; x < fractionEnd; x += sizeX / this->density->getReal())
//									 {
//										 for (Ogre::Real z = this->minimumBounds.z * this->scale->getReal() + this->positionXZ->getVector2().y; z < this->maximumBounds.z * this->scale->getReal() + this->positionXZ->getVector2().y; z += sizeZ / this->density->getReal())
//										 {
//											 Ogre::Vector3 objPosition = Vector3(Math::RangeRandom(-sizeX + x, sizeX + x), 0.0f, Math::RangeRandom(-sizeZ + z, sizeZ + z));
//
//											 Ogre::MovableObject* hitMovableObject = nullptr;
//											 Ogre::Real closestDistance = 0.0f;
//											 Ogre::Vector3 normal = Ogre::Vector3::ZERO;
//											 Ogre::Vector3 internalHitPoint = Ogre::Vector3::ZERO;
//
//											 // Shoot ray at object position down
//											 Ogre::Ray hitRay = Ogre::Ray(objPosition + Ogre::Vector3(0.0f, 10000.0f, 0.0f), Ogre::Vector3::NEGATIVE_UNIT_Y);
//											 // Check if there is an hit with an polygon of an entity, item, terra etc.
//											 this->raySceneQuery->setRay(hitRay);
//
//											 std::vector<Ogre::MovableObject*> excludeMovableObjects(0);
//											 /*excludeMovableObjects[0] = gameObject->getMovableObject<Ogre::v1::Entity>();
//											 excludeMovableObjects[1] = this->gizmo->getArrowEntityX();
//											 excludeMovableObjects[2] = this->gizmo->getArrowEntityY();
//											 excludeMovableObjects[3] = this->gizmo->getArrowEntityZ();
//											 excludeMovableObjects[4] = this->gizmo->getSphereEntity();*/
//
//											 MathHelper::getInstance()->getRaycastFromPoint(this->raySceneQuery, camera, internalHitPoint, (size_t&)hitMovableObject, closestDistance, normal, &excludeMovableObjects);
//
//											 objPosition.y += internalHitPoint.y;
//
//											 if (nullptr == hitMovableObject)
//											 {
//												 // Nothing found for given category, skip this quadrant
//												 continue;
//											 }
//
//											 if (0.0f != this->maxHeight->getReal())
//											 {
//												 if (internalHitPoint.y > this->maxHeight->getReal())
//												 {
//													 continue;
//												 }
//											 }
//
//											 if (nullptr != terra)
//											 {
//												 bool layerMatches = true;
//												 // Exclude maybe terra layers from vegetation placing
//												 std::vector<int> layers = terra->getLayerAt(objPosition);
//
//												 // Ogre::String str;
//												 for (size_t i = 0; i < this->terraLayerList.size(); i++)
//												 { 
//													 layerMatches &= layers[i] <= this->terraLayerList[i];
//													 // str += Ogre::StringConverter::toString(layers[i]) + " ";
//												 }
//
//												 //  Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "layers: " + str);
//												 if (false == layerMatches)
//												 {
//													 // Skip this vegetation position
//													 continue;
//												 }
//											 }
//
//											 // Create Mesh
//											 Ogre::Item* item = nullptr;
//
//											 int rndIndexMeshName = MathHelper::getInstance()->getRandomNumber<int>(0, static_cast<unsigned int>(this->vegetationMeshNames.size() - 1));
//
//											 Ogre::MeshPtr meshPtr = meshes[rndIndexMeshName];
//
//											 try
//											 {
//												 item = this->gameObjectPtr->getSceneManager()->createItem(meshPtr, Ogre::SCENE_STATIC);
//												 // After terrain
//												 item->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_MESH);
//												 item->setQueryFlags(vegetationQueryFlags);
//												 item->setCastShadows(false);
//												 if (0.0f != this->renderDistance->getReal())
//												 {
//													 item->setRenderingDistance(this->renderDistance->getReal());
//												 }
//
//
//
//
//#if 0
//
//												 for (size_t i = 0; i < item->getNumSubItems(); i++)
//												 {
//													 const Ogre::String* str = item->getSubItem(i)->getDatablock()->getNameStr();
//													 int a = 0;
//													 a = 1;
//													 item->getSubItem(i)->setDatablock(*str);
//												 }
//												 // Change the addressing mode of the roughness map to wrap via code.
//												// Detail maps default to wrap, but the rest to clamp.
//												 Ogre::HlmsPbsDatablock* datablock = static_cast<Ogre::HlmsPbsDatablock*>(item->getSubItem(0)->getDatablock());
//												 //Make a hard copy of the sampler block
//												 auto sampleBlock = datablock->getSamplerblock(Ogre::PBSM_ROUGHNESS);
//												 if (nullptr != sampleBlock)
//												 {
//													 Ogre::HlmsSamplerblock samplerblockCopy(*sampleBlock);
//													 samplerblockCopy.mU = Ogre::TAM_WRAP;
//													 samplerblockCopy.mV = Ogre::TAM_WRAP;
//													 samplerblockCopy.mW = Ogre::TAM_WRAP;
//													 //Set the new samplerblock. The Hlms system will
//													 //automatically create the API block if necessary
//													 datablock->setSamplerblock(Ogre::PBSM_ROUGHNESS, samplerblockCopy);
//												 }
//
//												 for (size_t i = 0; i < item->getNumSubItems(); i++)
//												 {
//													 auto sourceDataBlock = dynamic_cast<Ogre::HlmsPbsDatablock*>(item->getSubItem(i)->getDatablock());
//													 if (nullptr != sourceDataBlock)
//													 {
//														 // Deactivate fresnel by default, because it looks ugly
//														 if (sourceDataBlock->getWorkflow() != Ogre::HlmsPbsDatablock::SpecularAsFresnelWorkflow && sourceDataBlock->getWorkflow() != Ogre::HlmsPbsDatablock::MetallicWorkflow)
//														 {
//															 sourceDataBlock->setFresnel(Ogre::Vector3(0.01f, 0.01f, 0.01f), false);
//														 }
//													 }
//												 }
//#endif
//
//												 sizeX = item->getWorldAabb().getSize().x;
//												 sizeZ = item->getWorldAabb().getSize().z;
//											 }
//											 catch (...)
//											 {
//												 Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[VegetationComponent] Could not create item for mesh: " + meshPtr->getName() + " because the mesh is invalid for game object: " + this->gameObjectPtr->getName());
//												 return;
//											 }
//
//											 SceneNode* node = nullptr;
//
//											 if (nullptr != item)
//											 {
//												 if (nullptr == targetGameObjectPtr)
//												 {
//													 node = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode(Ogre::SCENE_STATIC);
//												 }
//												 else
//												 {
//													 node = targetGameObjectPtr->getSceneNode()->createChildSceneNode(Ogre::SCENE_STATIC);
//												 }
//												 node->attachObject(item);
//
//												 this->vegetationItemList.emplace_back(item);
//
//												 // VegetationScaleMin/Max as attribute?
//												 //  scale
//												 Ogre::Real s = Math::RangeRandom(0.5f, 1.2f);
//												 node->scale(s, s, s);
//
//												 // objPosition.y += std::min(item->getLocalAabb().getMinimum().y, Real(0.0f)) * -0.1f /*+ lay.down*/;  //par
//
//												 node->setPosition(objPosition);
//
//												 if (true == this->autoOrientation->getBool())
//												 {
//													 node->setDirection(normal, Ogre::Node::TS_PARENT, Ogre::Vector3::NEGATIVE_UNIT_Y);
//												 }
//
//												 Ogre::Degree angle(Math::RangeRandom(0.0f, 360.f));
//												 Quaternion p = node->getOrientation();
//												 Quaternion q;
//												 q.FromAngleAxis(angle, Vector3::UNIT_Y);
//												 node->setOrientation(p * q);
//
//											 }
//											
//											 createdCount++;
//										 }
//									 }
//									 mutex.unlock();
//								 }));
//								 partitionIncrement += partitionSize;
//		}
//
//		for (size_t i = 0; i < numThreads; i++)
//		{
//			threads[i].join();
//		}
//
//#endif
//
//		// Without threading takes approx. 21 seconds
//#if 0
//		for (Ogre::Real x = this->minimumBounds.x * this->scale->getReal() + this->positionXZ->getVector2().x; x < this->maximumBounds.x * this->scale->getReal() + this->positionXZ->getVector2().x; x += sizeX / this->density->getReal())
//		{
//			for (Ogre::Real z = this->minimumBounds.z * this->scale->getReal() + this->positionXZ->getVector2().y; z < this->maximumBounds.z * this->scale->getReal() + this->positionXZ->getVector2().y; z += sizeZ / this->density->getReal())
//			{
//				Ogre::Vector3 objPosition = Vector3(Math::RangeRandom(-sizeX + x, sizeX + x), 0.0f, Math::RangeRandom(-sizeZ + z, sizeZ + z));
//
//				Ogre::MovableObject* hitMovableObject = nullptr;
//				Ogre::Real closestDistance = 0.0f;
//				Ogre::Vector3 normal = Ogre::Vector3::ZERO;
//				Ogre::Vector3 internalHitPoint = Ogre::Vector3::ZERO;
//
//				// Shoot ray at object position down
//				Ogre::Ray hitRay = Ogre::Ray(objPosition + Ogre::Vector3(0.0f, 10000.0f, 0.0f), Ogre::Vector3::NEGATIVE_UNIT_Y);
//				// Check if there is an hit with an polygon of an entity, item, terra etc.
//				this->raySceneQuery->setRay(hitRay);
//				this->raySceneQuery->setQueryMask(categoryId);
//
//				std::vector<Ogre::MovableObject*> excludeMovableObjects(0);
//				/*excludeMovableObjects[0] = gameObject->getMovableObject<Ogre::v1::Entity>();
//				excludeMovableObjects[1] = this->gizmo->getArrowEntityX();
//				excludeMovableObjects[2] = this->gizmo->getArrowEntityY();
//				excludeMovableObjects[3] = this->gizmo->getArrowEntityZ();
//				excludeMovableObjects[4] = this->gizmo->getSphereEntity();*/
//				MathHelper::getInstance()->getRaycastFromPoint(this->raySceneQuery, AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera(), internalHitPoint, (size_t&)hitMovableObject, closestDistance, normal, &excludeMovableObjects);
//
//				objPosition.y += internalHitPoint.y;
//
//				if (nullptr == hitMovableObject)
//				{
//					// Nothing found for given category, skip this quadrant
//					continue;
//				}
//
//				if (0.0f != this->maxHeight->getReal())
//				{
//					if (internalHitPoint.y > this->maxHeight->getReal())
//					{
//						continue;
//					}
//				}
//
//				if (nullptr != terra)
//				{
//					// Exclude maybe terra layers from vegetation placing
//					int* layers = terra->getLayerAt(objPosition);
//					for (size_t i = 0; i < this->terraLayerList.size(); i++)
//					{
//						if (layers[i] < this->terraLayerList[i])
//						{
//							continue;
//						}
//					}
//				}
//
//				// Create Mesh
//				Ogre::Item* item = nullptr;
//
//				int rndIndexMeshName = MathHelper::getInstance()->getRandomNumber<int>(0, static_cast<unsigned int>(this->vegetationMeshNames.size() - 1));
//
//				Ogre::MeshPtr meshPtr = meshes[rndIndexMeshName];
//
//				try
//				{
//					item = this->gameObjectPtr->getSceneManager()->createItem(meshPtr, Ogre::SCENE_STATIC);
//					// After terrain
//					item->setRenderQueueGroup( NOWA::RENDER_QUEUE_V2_MESH);
//					item->setQueryFlags(vegetationQueryFlags);
//					item->setCastShadows(false);
//					if (0.0f != this->renderDistance->getReal())
//					{
//						item->setRenderingDistance(this->renderDistance->getReal());
//					}
//
//#if 0
//					for (size_t i = 0; i < item->getNumSubItems(); i++)
//					{
//						auto sourceDataBlock = dynamic_cast<Ogre::HlmsPbsDatablock*>(item->getSubItem(i)->getDatablock());
//						if (nullptr != sourceDataBlock)
//						{
//							// sourceDataBlock->mShadowConstantBias = 0.0001f;
//							// Deactivate fresnel by default, because it looks ugly
//							if (sourceDataBlock->getWorkflow() != Ogre::HlmsPbsDatablock::SpecularAsFresnelWorkflow && sourceDataBlock->getWorkflow() != Ogre::HlmsPbsDatablock::MetallicWorkflow)
//							{
//								sourceDataBlock->setFresnel(Ogre::Vector3(0.01f, 0.01f, 0.01f), false);
//							}
//						}
//					}
//#endif
//
//					sizeX = item->getWorldAabb().getSize().x;
//					sizeZ = item->getWorldAabb().getSize().z;
//				}
//				catch (...)
//				{
//					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[VegetationComponent] Could not create item for mesh: " + meshPtr->getName() + " because the mesh is invalid for game object: " + this->gameObjectPtr->getName());
//					return;
//				}
//
//				SceneNode* node = nullptr;
//
//				if (nullptr != item)
//				{
//					if (nullptr == targetGameObjectPtr)
//					{
//						node = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode(Ogre::SCENE_STATIC);
//					}
//					else
//					{
//						targetGameObjectPtr->getSceneNode()->createChildSceneNode(Ogre::SCENE_STATIC);
//					}
//					node->attachObject(item);
//
//					this->vegetationItemList.emplace_back(item);
//
//					// VegetationScaleMin/Max as attribute?
//					//  scale
//					Ogre::Real s = Math::RangeRandom(0.5f, 1.2f);
//					node->scale(s, s, s);
//
//					// objPosition.y += std::min(item->getLocalAabb().getMinimum().y, Real(0.0f)) * -0.1f /*+ lay.down*/;  //par
//
//					node->setPosition(objPosition);
//
//					Ogre::Degree angle(Math::RangeRandom(0.0f, 360.f));
//					Quaternion q;
//					q.FromAngleAxis(angle, Vector3::UNIT_Y);
//					node->setOrientation(q);
//
//					if (true == this->autoOrientation->getBool())
//					{
//						node->setDirection(normal, Ogre::Node::TS_PARENT, Ogre::Vector3::NEGATIVE_UNIT_Y);
//					}
//				}
//
//				createdCount++;
//			}
//		}
//#endif
//
//		auto finish = std::chrono::high_resolution_clock::now();
//		std::chrono::duration<double, std::milli> elapsed = finish - start;
//		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[VegetationComponent] Generation took: " + Ogre::StringConverter::toString(elapsed.count() * 0.001) + " seconds.");
//
//		if (true == this->bShowDebugData)
//		{
//			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[VegetationComponent] Created: " + Ogre::StringConverter::toString(createdCount) + " objects.");
//		}
//		else
//		{
//			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[VegetationComponent] Created: " + Ogre::StringConverter::toString(createdCount) + " objects.");
//		}
//
//		boost::shared_ptr<EventDataRefreshGui> eventDataRefreshGui(new EventDataRefreshGui());
//		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataRefreshGui);
//	}

void VegetationComponent::regenerateVegetation()
{
	if (false == this->activated->getBool())
	{
		return;
	}

	if (nullptr == this->gameObjectPtr)
	{
		return;
	}

	if (GameObject::AttrCustomDataSkipCreation() == this->gameObjectPtr->getCustomDataString())
	{
		return;
	}

	// Reset variants when stopping simulation, would re-generate everything, which is bad, hence use skip flag
	if (GameObject::AttrCustomDataSkipCreation() == this->gameObjectPtr->getCustomDataString())
	{
		return;
	}

	// First clear everything, and only regenerate if at least either on veg id, or mesh name is available
	// e.g. if reducing count or increasing count, only regenerate if there are some game objects or meshes set.
	this->clearLists();

	if (vegetationTypesCount->getUInt() == 0)
	{
		return;
	}

	bool validData = true;

	for (size_t i = 0; i < this->vegetationTypesCount->getUInt(); i++)
	{
		Ogre::String name = this->vegetationMeshNames[i]->getString();

		validData &= (false == name.empty());
	}

	if (false == validData)
	{
		return;
	}

	// Early validation and preparation
	if (this->vegetationMeshNames.empty()) {
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[VegetationComponent] No vegetation meshes defined.");
		return;
	}

	// Precompute and cache frequently used values
	unsigned int categoryId = AppStateManager::getSingletonPtr()->getGameObjectController()->generateCategoryId(this->categories->getString());
	this->raySceneQuery->setQueryMask(categoryId);

	Ogre::Terra* terra = nullptr;
	GameObjectPtr targetGameObjectPtr = nullptr;

	// Improved object and terrain retrieval
	targetGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->targetGameObjectId->getULong());
	if (targetGameObjectPtr)
	{
		auto terraCompPtr = NOWA::makeStrongPtr(targetGameObjectPtr->getComponent<TerraComponent>());
		if (terraCompPtr)
		{
			terra = terraCompPtr->getTerra();
		}
	}

	// Fallback terrain retrieval if no target object
	if (!terra)
	{
		auto terraList = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectsFromComponent(TerraComponent::getStaticClassName());
		if (!terraList.empty())
		{
			auto terraCompPtr = NOWA::makeStrongPtr(terraList[0]->getComponent<TerraComponent>());
			if (terraCompPtr)
			{
				terra = terraCompPtr->getTerra();
			}
		}
	}

	// Precompute bounds
	if (targetGameObjectPtr)
	{
		this->minimumBounds = targetGameObjectPtr->getMovableObject()->getWorldAabb().getMinimum();
		this->maximumBounds = targetGameObjectPtr->getMovableObject()->getWorldAabb().getMaximum();
	}

	// Mesh preparation with error handling
	std::vector<Ogre::MeshPtr> meshes;
	meshes.reserve(this->vegetationMeshNames.size());

	for (const auto& meshName : this->vegetationMeshNames)
	{
		if (meshName->getString().empty())
		{
			meshes.emplace_back(nullptr);
			continue;
		}

		try
		{
			// Load V1 mesh and convert to V2
			auto v1Mesh = Ogre::v1::MeshManager::getSingletonPtr()->load(meshName->getString(),
				Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
				Ogre::v1::HardwareBuffer::HBU_STATIC,
				Ogre::v1::HardwareBuffer::HBU_STATIC);

			// Clean up any existing V2 mesh to prevent naming conflicts
			Ogre::ResourcePtr resourceV2 = Ogre::MeshManager::getSingletonPtr()->getResourceByName(meshName->getString());
			if (resourceV2)
			{
				Ogre::MeshManager::getSingletonPtr()->destroyResourcePool(meshName->getString());
				Ogre::MeshManager::getSingletonPtr()->remove(resourceV2->getHandle());
			}

			auto v2Mesh = Ogre::MeshManager::getSingletonPtr()->createByImportingV1(meshName->getString(),
				Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
				v1Mesh.get(),
				true, true, true);

			v1Mesh->unload();
			v1Mesh.setNull();

			meshes.emplace_back(v2Mesh);
		}
		catch (const Ogre::Exception& e)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,"[VegetationComponent] Mesh loading error: " + e.getDescription());
			meshes.emplace_back(nullptr);
		}
	}

	// Compute generation parameters
	unsigned int vegetationQueryFlags = AppStateManager::getSingletonPtr()->getGameObjectController()->generateCategoryId(this->gameObjectPtr->getCategory());
	const Ogre::Real size = Ogre::Math::Abs(this->minimumBounds.x) * this->scale->getReal() +
		this->positionXZ->getVector2().x +
		Ogre::Math::Abs(this->maximumBounds.x) * this->scale->getReal() +
		this->positionXZ->getVector2().x;

	// Parallel generation with improved thread distribution
	const size_t numThreads = std::max<size_t>(1, Ogre::PlatformInformation::getNumLogicalCores());
	const Ogre::Real step = 1.0f / this->density->getReal();
	std::atomic<unsigned long> createdCount{ 0 };
	std::vector<std::thread> threads;
	std::mutex vegetationMutex;

	auto start = std::chrono::high_resolution_clock::now();

	Ogre::Camera* camera = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();

	auto generateVegetation = [&](size_t threadIndex, size_t totalThreads) {
		std::vector<Ogre::Item*> localVegetationItems;

		for (Ogre::Real x = this->minimumBounds.x * this->scale->getReal() + this->positionXZ->getVector2().x +
			(threadIndex * size / totalThreads);
			x < this->minimumBounds.x * this->scale->getReal() + this->positionXZ->getVector2().x +
			((threadIndex + 1) * size / totalThreads);
			x += step)
		{
			for (Ogre::Real z = this->minimumBounds.z * this->scale->getReal() + this->positionXZ->getVector2().y;
				z < this->maximumBounds.z * this->scale->getReal() + this->positionXZ->getVector2().y;
				z += step)
			{
				std::lock_guard<std::mutex> lock(vegetationMutex);
				Ogre::Vector3 objPosition = Vector3(Math::RangeRandom(-size + x, size + x), 0.0f, Math::RangeRandom(-size + z, size + z));
				
				Ogre::MovableObject* hitMovableObject = nullptr;
				Ogre::Real closestDistance = 0.0f;
				Ogre::Vector3 normal = Ogre::Vector3::ZERO;
				Ogre::Vector3 internalHitPoint = Ogre::Vector3::ZERO;
				
				// Shoot ray at object position down
				Ogre::Ray hitRay = Ogre::Ray(objPosition + Ogre::Vector3(0.0f, 10000.0f, 0.0f), Ogre::Vector3::NEGATIVE_UNIT_Y);
				// Check if there is an hit with an polygon of an entity, item, terra etc.
				this->raySceneQuery->setRay(hitRay);
				
				std::vector<Ogre::MovableObject*> excludeMovableObjects(0);
				/*excludeMovableObjects[0] = gameObject->getMovableObject<Ogre::v1::Entity>();
				excludeMovableObjects[1] = this->gizmo->getArrowEntityX();
				excludeMovableObjects[2] = this->gizmo->getArrowEntityY();
				excludeMovableObjects[3] = this->gizmo->getArrowEntityZ();
				excludeMovableObjects[4] = this->gizmo->getSphereEntity();*/
				
				MathHelper::getInstance()->getRaycastFromPoint(this->raySceneQuery, camera, internalHitPoint, (size_t&)hitMovableObject, closestDistance, normal, &excludeMovableObjects);
				
				objPosition.y += internalHitPoint.y;
				
				if (nullptr == hitMovableObject)
				{
					// Nothing found for given category, skip this quadrant
					continue;
				}
				
				if (0.0f != this->maxHeight->getReal())
				{
					if (internalHitPoint.y > this->maxHeight->getReal())
					{
						continue;
					}
				}
				
				if (nullptr != terra)
				{
					bool layerMatches = true;
					// Exclude maybe terra layers from vegetation placing
					std::vector<int> layers = terra->getLayerAt(objPosition);
				
					// Ogre::String str;
					for (size_t i = 0; i < this->terraLayerList.size(); i++)
					{ 
						layerMatches &= layers[i] <= this->terraLayerList[i];
						// str += Ogre::StringConverter::toString(layers[i]) + " ";
					}
				
					//  Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "layers: " + str);
					if (false == layerMatches)
					{
						// Skip this vegetation position
						continue;
					}
				}
				
				// Create Mesh
				Ogre::Item* item = nullptr;
				
				int rndIndexMeshName = MathHelper::getInstance()->getRandomNumber<int>(0, static_cast<unsigned int>(this->vegetationMeshNames.size() - 1));
				
				Ogre::MeshPtr meshPtr = meshes[rndIndexMeshName];
				
				try
				{
					item = this->gameObjectPtr->getSceneManager()->createItem(meshPtr, Ogre::SCENE_STATIC);
					// After terrain
					item->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_MESH);
					item->setQueryFlags(vegetationQueryFlags);
					item->setCastShadows(false);
					if (0.0f != this->renderDistance->getReal())
					{
						item->setRenderingDistance(this->renderDistance->getReal());
					}
				}
				catch (...)
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[VegetationComponent] Could not create item for mesh: " + meshPtr->getName() + " because the mesh is invalid for game object: " + this->gameObjectPtr->getName());
					return;
				}
				
				SceneNode* node = nullptr;
				
				if (nullptr != item)
				{
					if (nullptr == targetGameObjectPtr)
					{
						node = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode(Ogre::SCENE_STATIC);
					}
					else
					{
						node = targetGameObjectPtr->getSceneNode()->createChildSceneNode(Ogre::SCENE_STATIC);
					}
					node->attachObject(item);
				
					this->vegetationItemList.emplace_back(item);
				
					// VegetationScaleMin/Max as attribute?
					//  scale
					Ogre::Real s = Math::RangeRandom(0.5f, 1.2f);
					node->scale(s, s, s);
				
					// objPosition.y += std::min(item->getLocalAabb().getMinimum().y, Real(0.0f)) * -0.1f /*+ lay.down*/;  //par
				
					node->setPosition(objPosition);
				
					if (true == this->autoOrientation->getBool())
					{
						node->setDirection(normal, Ogre::Node::TS_PARENT, Ogre::Vector3::NEGATIVE_UNIT_Y);
					}
				
					Ogre::Degree angle(Math::RangeRandom(0.0f, 360.f));
					Quaternion p = node->getOrientation();
					Quaternion q;
					q.FromAngleAxis(angle, Vector3::UNIT_Y);
					node->setOrientation(p * q);
				
				}
															
				createdCount++;
			}
		}

		// Batch update the shared vegetation list
		{
			std::lock_guard<std::mutex> lock(vegetationMutex);
			this->vegetationItemList.insert(this->vegetationItemList.end(), localVegetationItems.begin(), localVegetationItems.end());
		}
	};

	// Create and start threads
	for (size_t i = 0; i < numThreads; ++i)
	{
		threads.emplace_back(generateVegetation, i, numThreads);
	}

	// Wait for all threads to complete
	for (auto& thread : threads)
	{
		thread.join();
	}

	auto finish = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> elapsed = finish - start;
	Ogre::LogManager::getSingletonPtr()->logMessage(this->bShowDebugData ? Ogre::LML_CRITICAL : Ogre::LML_TRIVIAL, "[VegetationComponent] Generation took: " + Ogre::StringConverter::toString(elapsed.count() * 0.001) + " seconds.");

	Ogre::LogManager::getSingletonPtr()->logMessage(this->bShowDebugData ? Ogre::LML_CRITICAL : Ogre::LML_TRIVIAL, "[VegetationComponent] Created: " + Ogre::StringConverter::toString(createdCount) + " objects.");
}

	void VegetationComponent::handleUpdateBounds(NOWA::EventDataPtr eventData)
	{
		// When a new game object has been added to the scene, update the bounds for vegetation
		boost::shared_ptr<NOWA::EventDataBoundsUpdated> castEventData = boost::static_pointer_cast<EventDataBoundsUpdated>(eventData);
		this->minimumBounds = castEventData->getCalculatedBounds().first;
		this->maximumBounds = castEventData->getCalculatedBounds().second;

		// this->regenerateVegetation();
	}

	void VegetationComponent::handleSceneParsed(NOWA::EventDataPtr eventData)
	{
		this->minimumBounds = Core::getSingletonPtr()->getCurrentSceneBoundLeftNear();
		this->maximumBounds = Core::getSingletonPtr()->getCurrentSceneBoundRightFar();
		this->regenerateVegetation();
	}

	void VegetationComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{

	}

	bool VegetationComponent::canStaticAddComponent(GameObject* gameObject)
	{
		// Can be added several times, in order to populate several areas
		if (gameObject->getId() == GameObjectController::MAIN_GAMEOBJECT_ID)
		{
			return true;
		}
		return false;
	}

}; //namespace end
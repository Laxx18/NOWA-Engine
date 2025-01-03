#include "NOWAPrecompiled.h"
#include "NodeComponent.h"
#include "GameObjectController.h"
#include "utilities/XMLConverter.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	NodeComponent::NodeComponent()
		: GameObjectComponent(),
		dummyEntity(nullptr),
		show(new Variant(NodeComponent::AttrShow(), true, this->attributes))
	{

	}

	NodeComponent::~NodeComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[NodeComponent] Destructor node component for game object: " + this->gameObjectPtr->getName());
		this->dummyEntity = nullptr;
	}

	bool NodeComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Show")
		{
			this->show->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}

		return true;
	}

	GameObjectCompPtr NodeComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		NodeCompPtr clonedCompPtr(boost::make_shared<NodeComponent>());

		
		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		clonedCompPtr->setShow(this->show->getBool());

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool NodeComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[NodeComponent] Init node component for game object: " + this->gameObjectPtr->getName());

		this->dummyEntity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
		
		return true;
	}

	bool NodeComponent::connect(void)
	{
		if (nullptr != dummyEntity)
		{
			if (true == this->dummyEntity->isVisible() && false == this->show->getBool())
			{
				this->dummyEntity->setVisible(false);
			}
			
		}

		return true;
	}

	bool NodeComponent::disconnect(void)
	{
		if (false == this->dummyEntity->isVisible())
		{
			this->dummyEntity->setVisible(true);
		}
		return true;
	}

	void NodeComponent::update(Ogre::Real dt, bool notSimulating)
	{
	}

	void NodeComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (NodeComponent::AttrShow() == attribute->getName())
		{
			this->setShow(attribute->getBool());
		}
	}

	void NodeComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("name", "Show"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->show->getBool())));
		propertiesXML->append_node(propertyXML);
	}

	void NodeComponent::setShow(bool show)
	{
		this->show->setValue(show);
	}

	bool NodeComponent::getShow(void) const
	{
		return this->show->getBool();
	}

	Ogre::String NodeComponent::getClassName(void) const
	{
		return "NodeComponent";
	}

	Ogre::String NodeComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

}; // namespace end
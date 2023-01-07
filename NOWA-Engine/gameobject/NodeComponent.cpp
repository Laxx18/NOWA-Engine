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
		hideEntity(true)
	{

	}

	NodeComponent::~NodeComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[NodeComponent] Destructor node component for game object: " + this->gameObjectPtr->getName());
		this->dummyEntity = nullptr;
	}

	bool NodeComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		return true;
	}

	GameObjectCompPtr NodeComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		NodeCompPtr clonedCompPtr(boost::make_shared<NodeComponent>());

		
		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool NodeComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[NodeComponent] Init node component for game object: " + this->gameObjectPtr->getName());

		this->dummyEntity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
		if ("Node.mesh" == this->dummyEntity->getMesh()->getName())
		{
			this->hideEntity = true;
		}

		return true;
	}

	void NodeComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (nullptr != dummyEntity && true == this->hideEntity)
		{
			if (false == notSimulating && true == this->dummyEntity->isVisible())
			{
				this->dummyEntity->setVisible(false);
			}
			else if (true == notSimulating)
			{
				if (false == this->dummyEntity->isVisible())
				{
					this->dummyEntity->setVisible(true);
				}
			}
		}
		// Here stop simulation is missing, so that the entity may be visible again!, bool simulation stopped
	}

	void NodeComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);
	}

	void NodeComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		GameObjectComponent::writeXML(propertiesXML, doc, filePath);
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
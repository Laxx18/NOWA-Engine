#include "NOWAPrecompiled.h"
#include "PhysicsCompoundConnectionComponent.h"
#include "PhysicsActiveComponent.h"
#include "utilities/XMLConverter.h"
#include "PhysicsActiveComponent.h"
#include "main/AppStateManager.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	typedef boost::shared_ptr<PhysicsActiveComponent> PhysicsActiveCompPtr;

	PhysicsCompoundConnectionComponent::PhysicsCompoundConnectionComponent()
		: GameObjectComponent(),
		priorId(0)
	{
		this->rootId = new Variant(PhysicsCompoundConnectionComponent::AttrRootId(), static_cast<unsigned long>(0), this->attributes, true);
		this->rootId->setDescription("The root id to set for another game objects id, so that the compound can be connected to the root game object for compound collision creation.");
		AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &PhysicsCompoundConnectionComponent::handleRootComponentDeleted), EventDataDeleteComponent::getStaticEventType());
	}

	PhysicsCompoundConnectionComponent::~PhysicsCompoundConnectionComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsCompoundConnectionComponent] Destructor compound connection component for game object: " + this->gameObjectPtr->getName());
		AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &PhysicsCompoundConnectionComponent::handleRootComponentDeleted), EventDataDeleteComponent::getStaticEventType());
	}

	void PhysicsCompoundConnectionComponent::setRootId(unsigned long rootId)
	{
		this->rootId->setValue(rootId);
	}

	unsigned long PhysicsCompoundConnectionComponent::getRootId(void) const
	{
		return this->rootId->getULong();
	}

	GameObjectCompPtr PhysicsCompoundConnectionComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		PhysicsCompoundConnectionCompPtr clonedCompPtr(boost::make_shared<PhysicsCompoundConnectionComponent>());

		
		clonedCompPtr->internalSetPriorId(this->gameObjectPtr->getId());
		clonedCompPtr->setRootId(this->rootId->getULong());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool PhysicsCompoundConnectionComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsCompoundConnectionComponent] Init compound connection component for game object: " + this->gameObjectPtr->getName());

		// Physics active component must be dynamic, else a mess occurs
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);

		return true;
	}

	bool PhysicsCompoundConnectionComponent::connect(void)
	{
		this->physicsActiveComponentList.clear();
		PhysicsActiveCompPtr physicsActiveCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsActiveComponent>());
		if (nullptr != physicsActiveCompPtr)
		{

			// Add the compound to the compound component map, but as weak ptr, because game object controller should not hold the life cycle of this components, because the game objects already do, which are
			// also hold shared by the game object controller
			AppStateManager::getSingletonPtr()->getGameObjectController()->addPhysicsCompoundConnectionComponent(boost::dynamic_pointer_cast<PhysicsCompoundConnectionComponent>(shared_from_this()));

			return true;
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsCompoundConnectionComponent] Error: Final init failed, because the game object: "
				+ this->gameObjectPtr->getName() + " has no kind of physics active component!");
			return false;
		}
		return true;
	}

	bool PhysicsCompoundConnectionComponent::disconnect(void)
	{
		AppStateManager::getSingletonPtr()->getGameObjectController()->removePhysicsCompoundConnectionComponent(this->gameObjectPtr->getId());
		this->destroyCompoundCollision();

		return true;
	}

	bool PhysicsCompoundConnectionComponent::onCloned(void)
	{
		if (0 == this->rootId->getULong())
			return true;

		// Search for the prior id of the cloned game object and set the new id and set the new id, if not found set better 0, else the game objects may be corrupt!
		GameObjectPtr rootGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getClonedGameObjectFromPriorId(this->rootId->getULong());
		if (nullptr != rootGameObjectPtr)
			// Only the id is important!
			this->setRootId(rootGameObjectPtr->getId());
		else
			this->setRootId(0);
		return true;
	}

	Ogre::String PhysicsCompoundConnectionComponent::getClassName(void) const
	{
		return "PhysicsCompoundConnectionComponent";
	}

	Ogre::String PhysicsCompoundConnectionComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	bool PhysicsCompoundConnectionComponent::canStaticAddComponent(GameObject* gameObject)
	{
		// Check if there is a physics active component and no ai lua component (because it already has a moving behavior)
		auto& physicsCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<PhysicsActiveComponent>());
		if (nullptr != physicsCompPtr)
		{
			return true;
		}
		return false;
	}

	bool PhysicsCompoundConnectionComponent::init(xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RootId")
		{
			this->rootId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	void PhysicsCompoundConnectionComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (PhysicsCompoundConnectionComponent::AttrRootId() == attribute->getName())
		{
			this->setRootId(attribute->getULong());
		}
	}

	void PhysicsCompoundConnectionComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("name", "RootId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->rootId->getULong())));
		propertiesXML->append_node(propertyXML);
	}

	unsigned long PhysicsCompoundConnectionComponent::getId(void) const
	{
		return this->gameObjectPtr->getId();
	}

	void PhysicsCompoundConnectionComponent::internalSetPriorId(unsigned long priorId)
	{
		this->priorId = priorId;
	}

	void PhysicsCompoundConnectionComponent::handleRootComponentDeleted(NOWA::EventDataPtr eventData)
	{
		boost::shared_ptr<EventDataDeleteComponent> castEventData = boost::static_pointer_cast<EventDataDeleteComponent>(eventData);
		// Found the game object
		if (this->rootId->getULong() == castEventData->getGameObjectId())
		{
			// Now check if a kind of physics component has been removed
			size_t foundPhysicsComponent = castEventData->getComponentName().find("Physics");
			if (Ogre::String::npos != foundPhysicsComponent)
			{
				this->destroyCompoundCollision();
				AppStateManager::getSingletonPtr()->getGameObjectController()->removePhysicsCompoundConnectionComponent(castEventData->getGameObjectId());
			}
		}
	}

	unsigned long PhysicsCompoundConnectionComponent::getPriorId(void) const
	{
		return this->priorId;
	}

	bool PhysicsCompoundConnectionComponent::createCompoundCollision(void)
	{
		if (0 == this->rootId->getULong())
		{
			for (size_t i = 0; i < this->physicsActiveComponentList.size(); i++)
			{
				auto& physicsActiveCompPtr = this->physicsActiveComponentList[i];
				if (nullptr != physicsActiveCompPtr)
				{
					physicsActiveCompPtr->destroyCollision();
				}
			}

			// First destroy all collisions for this root and all other connected compounds in the list
			auto& physicsActiveRootCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsActiveComponent>());
			if (nullptr != physicsActiveRootCompPtr)
			{
				physicsActiveRootCompPtr->destroyCollision();
			}

			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("PhysicsCompoundConnectionComponent::createCompoundCollision", _1(physicsActiveRootCompPtr),
			{
				physicsActiveRootCompPtr->createCompoundBody(this->physicsActiveComponentList);
			});
		}
		return true;
	}
	
	void PhysicsCompoundConnectionComponent::destroyCompoundCollision(void)
	{
		auto& physicsActiveRootCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsActiveComponent>());
		if (nullptr != physicsActiveRootCompPtr)
		{
			physicsActiveRootCompPtr->destroyCompoundBody(this->physicsActiveComponentList);
			this->physicsActiveComponentList.clear();
		}
	}

	void PhysicsCompoundConnectionComponent::addPhysicsActiveComponent(PhysicsActiveComponent* physicsActiveComponent)
	{
		if (nullptr != physicsActiveComponent)
		{
			this->physicsActiveComponentList.emplace_back(physicsActiveComponent);
		}
	}

}; // namespace end
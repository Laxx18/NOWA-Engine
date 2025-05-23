#include "NOWAPrecompiled.h"
#include "TagChildNodeComponent.h"
#include "PhysicsActiveComponent.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "main/AppStateManager.h"

#include "TagPointComponent.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	TagChildNodeComponent::TagChildNodeComponent()
		: GameObjectComponent(),
		sourceChildNode(nullptr),
		sourceParentOfChildNode(nullptr),
		oldSourceChildPosition(Ogre::Vector3::ZERO),
		oldSourceChildOrientation(Ogre::Quaternion::IDENTITY),
		alreadyConnected(false),
		sourceId(new Variant(TagChildNodeComponent::AttrSourceId(), static_cast<unsigned long>(0), this->attributes, true))
	{
		
	}

	TagChildNodeComponent::~TagChildNodeComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[TagChildNodeComponent] Destructor tag child node component for game object: " + this->gameObjectPtr->getName());
		this->sourceChildNode = nullptr;
		this->sourceParentOfChildNode = nullptr;
	}

	bool TagChildNodeComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SourceId")
		{
			this->sourceId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	GameObjectCompPtr TagChildNodeComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		TagChildNodeCompPtr clonedCompPtr(boost::make_shared<TagChildNodeComponent>());

		
		clonedCompPtr->setSourceId(this->sourceId->getULong());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool TagChildNodeComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[TagChildNodeComponent] Init child node component for game object: " + this->gameObjectPtr->getName());

		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);

		return true;
	}

	bool TagChildNodeComponent::connect(void)
	{
// Here when in post init add child, so that object cann be moved in editor directly
		// if source id = 0 and one was a child, remove the child! resolve groups!
		if (false == alreadyConnected)
		{
			ENQUEUE_RENDER_COMMAND_WAIT("TagChildNodeComponent::connect",
			{
				GameObjectPtr sourceGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->sourceId->getULong());

				if (nullptr == sourceGameObjectPtr)
				{
					return false;
				}

				this->sourceChildNode = sourceGameObjectPtr->getSceneNode();

				// Remember the old position for disconnection
				this->oldSourceChildPosition = this->sourceChildNode->getPosition();
				this->oldSourceChildOrientation = this->sourceChildNode->getOrientation();

				Ogre::Vector3 resultPosition = this->sourceChildNode->getPosition();
				Ogre::Quaternion resultOrientation = this->sourceChildNode->getOrientation();

				// Remove from source
				this->sourceParentOfChildNode = this->sourceChildNode->getParent();
				this->sourceParentOfChildNode->removeChild(this->sourceChildNode);
				// Add as child to this one
				this->gameObjectPtr->getSceneNode()->addChild(this->sourceChildNode);

				// Set the new transform
				this->sourceChildNode->setOrientation(resultOrientation * this->gameObjectPtr->getSceneNode()->getOrientation().Inverse());
				this->sourceChildNode->setPosition(resultPosition - this->gameObjectPtr->getSceneNode()->getPosition());
				this->sourceChildNode->setInheritScale(false);

				resultPosition = this->sourceChildNode->getPosition();
				resultOrientation = this->sourceChildNode->getOrientation();

				this->alreadyConnected = true;
			});
		}
		return true;
	}

	bool TagChildNodeComponent::disconnect(void)
	{
		GameObjectPtr sourceGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->sourceId->getULong());
		if (nullptr != sourceGameObjectPtr)
		{
			if (nullptr != this->sourceChildNode)
			{
				ENQUEUE_RENDER_COMMAND_WAIT("TagChildNodeComponent::disconnect",
				{
					// Remove from this one
					this->sourceChildNode->getParent()->removeChild(this->sourceChildNode);
					// This is tricky: Source was the scene node directly, so get its parent, which sould be the world node and add as child
					this->sourceParentOfChildNode->addChild(this->sourceChildNode);
					this->sourceChildNode->setOrientation(this->oldSourceChildOrientation);
					this->sourceChildNode->setPosition(this->oldSourceChildPosition);

					this->alreadyConnected = false;
				});
			}
		}
		return true;
	}

	bool TagChildNodeComponent::onCloned(void)
	{
		// Search for the prior id of the cloned game object and set the new id and set the new id, if not found set better 0, else the game objects may be corrupt!
		GameObjectPtr sourceGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getClonedGameObjectFromPriorId(this->sourceId->getULong());
		if (nullptr != sourceGameObjectPtr)
			this->setSourceId(sourceGameObjectPtr->getId());
		else
			this->setSourceId(0);
		return true;
	}

	void TagChildNodeComponent::update(Ogre::Real dt, bool notSimulating)
	{
		
	}

	void TagChildNodeComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (TagChildNodeComponent::AttrSourceId() == attribute->getName())
		{
			this->setSourceId(attribute->getULong());
		}
	}

	void TagChildNodeComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SourceId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->sourceId->getULong())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String TagChildNodeComponent::getClassName(void) const
	{
		return "TagChildNodeComponent";
	}

	Ogre::String TagChildNodeComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void TagChildNodeComponent::setActivated(bool activated)
	{

	}

	void TagChildNodeComponent::setSourceId(unsigned long sourceId)
	{
		this->sourceId->setValue(sourceId);
	}

	unsigned long TagChildNodeComponent::getSourceId(void) const
	{
		return this->sourceId->getULong();
	}

}; // namespace end
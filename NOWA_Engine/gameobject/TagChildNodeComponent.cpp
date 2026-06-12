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
        activated(new Variant(TagChildNodeComponent::AttrActivated(), true, this->attributes, true)),
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

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
        {
            this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
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

		clonedCompPtr->setActivated(this->activated->getBool());

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
        GameObjectComponent::connect();
        
		this->setActivated(this->activated->getBool());

        return true;
    }

    bool TagChildNodeComponent::disconnect(void)
    {
        GameObjectComponent::disconnect();
        
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

		if (TagChildNodeComponent::AttrActivated() == attribute->getName())
        {
            this->setActivated(attribute->getBool());
        }
		else if (TagChildNodeComponent::AttrSourceId() == attribute->getName())
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
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
        propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
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
		if (this->activated->getBool() == activated)
		{
            return;
		}

        this->activated->setValue(activated);

        if (true == activated)
        {
            // sourceId points to the parent GameObject (e.g. the spaceship).
            // This component's GameObject (e.g. a spotlight) becomes a child of that parent node.
            // Ogre's scene graph then carries this node along automatically -- no update() required.
            if (false == this->alreadyConnected)
            {
                GameObjectPtr sourceGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->sourceId->getULong());

                if (nullptr == sourceGameObjectPtr)
                {
                    return;
                }

                GraphicsModule::RenderCommand renderCommand = [this, sourceGameObjectPtr]()
                {
                    Ogre::SceneNode* ownNode = this->gameObjectPtr->getSceneNode();
                    Ogre::SceneNode* newParentNode = sourceGameObjectPtr->getSceneNode();

                    // Remember world-space transform so disconnect can restore it exactly
                    this->oldSourceChildPosition = ownNode->_getDerivedPosition();
                    this->oldSourceChildOrientation = ownNode->_getDerivedOrientation();

                    // Remember the original parent so disconnect can re-attach there
                    this->sourceParentOfChildNode = ownNode->getParent();
                    this->sourceChildNode = ownNode;

                    // Compute the local transform relative to the new parent so the node stays
                    // at exactly the world-space position it was placed at in the editor.
                    // E.g. if the spotlight was placed at the cockpit, it stays at the cockpit offset.
                    Ogre::Quaternion parentWorldOrient = newParentNode->_getDerivedOrientation();
                    Ogre::Vector3 parentWorldPos = newParentNode->_getDerivedPosition();

                    Ogre::Vector3 localPos = parentWorldOrient.Inverse() * (this->oldSourceChildPosition - parentWorldPos);
                    Ogre::Quaternion localOrient = parentWorldOrient.Inverse() * this->oldSourceChildOrientation;

                    // Re-parent: detach from current parent, attach under the spaceship node
                    this->sourceParentOfChildNode->removeChild(ownNode);
                    newParentNode->addChild(ownNode);

                    ownNode->setPosition(localPos);
                    ownNode->setOrientation(localOrient);
                    ownNode->setInheritScale(false);

                    this->alreadyConnected = true;
                };
                NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "TagChildNodeComponent::connect");
            }
        }
		else
		{
            if (true == AppStateManager::getSingletonPtr()->getGameObjectController()->getIsDestroying())
            {
                return;
            }

            if (nullptr != this->sourceChildNode && nullptr != this->sourceParentOfChildNode)
            {
                GraphicsModule::RenderCommand renderCommand = [this]()
                {
                    // Detach this node from the spaceship
                    this->sourceChildNode->getParent()->removeChild(this->sourceChildNode);
                    // Re-attach to original parent (e.g. root scene node)
                    this->sourceParentOfChildNode->addChild(this->sourceChildNode);
                    // Restore original world-space transform
                    this->sourceChildNode->setPosition(this->oldSourceChildPosition);
                    this->sourceChildNode->setOrientation(this->oldSourceChildOrientation);
                    this->sourceChildNode->setInheritScale(true);

                    this->alreadyConnected = false;
                };
                NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "TagChildNodeComponent::disconnect");
            }
		}
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
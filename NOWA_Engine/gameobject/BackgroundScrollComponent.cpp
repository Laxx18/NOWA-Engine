#include "NOWAPrecompiled.h"
#include "BackgroundScrollComponent.h"
#include "GameObjectController.h"
#include "WorkspaceComponents.h"
#include "utilities/XMLConverter.h"
#include "main/Events.h"
#include "utilities/MathHelper.h"
#include "main/AppStateManager.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	BackgroundScrollComponent::BackgroundScrollComponent()
		: GameObjectComponent(),
		targetSceneNode(nullptr),
		lastPosition(Ogre::Vector2::ZERO),
		pausedLastPosition(Ogre::Vector2::ZERO),
		firstTimePositionSet(true),
		xScroll(0.0f),
		yScroll(0.0f),
		workspaceBackgroundComponent(nullptr),
		active(new Variant(BackgroundScrollComponent::AttrActive(), true, this->attributes)),
		targetId(new Variant(BackgroundScrollComponent::AttrTargetId(), static_cast<unsigned long>(0), this->attributes, true)),
		moveSpeedX(new Variant(BackgroundScrollComponent::AttrMoveSpeedX(), 0.01f, this->attributes)),
		moveSpeedY(new Variant(BackgroundScrollComponent::AttrMoveSpeedY(), 0.0f, this->attributes)),
		followGameObjectX(new Variant(BackgroundScrollComponent::AttrFollowGameObjectX(), false, this->attributes)),
		followGameObjectY(new Variant(BackgroundScrollComponent::AttrFollowGameObjectY(), false, this->attributes)),
		followGameObjectZ(new Variant(BackgroundScrollComponent::AttrFollowGameObjectZ(), false, this->attributes)),
		backgroundName(new Variant(BackgroundScrollComponent::AttrBackgroundName(), Ogre::String("Sky.png"), this->attributes))
	{
		// moveSpeedX: far = 0.2, middle = 0.5, near = 1.0
		this->active->setDescription("Activates the background scroll operations.");
		this->targetId->setDescription("The target id for the game object the background should follow");
		this->moveSpeedX->setDescription("The move speed x. It can also be a negative value to change the x scroll direction. If set to 0 no x scrolling is done.");
		this->moveSpeedY->setDescription("The move speed y. It can also be a negative value to change the y scroll direction. If set to 0 no y scrolling is done.");
		this->followGameObjectX->setDescription("Whether to follow the game object's x position (if target id is valid).");
		this->followGameObjectY->setDescription("Whether to follow the game object's y position (if target id is valid).");
		this->followGameObjectZ->setDescription("Whether to follow the game object's z position instead of y (if target id is valid).");
		this->backgroundName->setDescription("The background far texture name. If empty, no background far will be rendered.");
	}

	BackgroundScrollComponent::~BackgroundScrollComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[BackgroundScrollComponent] Destructor background scroll component for game object: " + this->gameObjectPtr->getName());

		this->workspaceBackgroundComponent = nullptr;
	}

	bool BackgroundScrollComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Active")
		{
			this->setActivated(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TargetId")
		{
			this->targetId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MoveSpeedX")
		{
			this->moveSpeedX->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MoveSpeedY")
		{
			this->moveSpeedY->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FollowGameObjectX")
		{
			this->followGameObjectX->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FollowGameObjectY")
		{
			this->followGameObjectY->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FollowGameObjectZ")
		{
			this->followGameObjectZ->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "BackgroundName")
		{
			this->backgroundName->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	bool BackgroundScrollComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[BackgroundScrollComponent] Init background scroll component for game object: " + this->gameObjectPtr->getName());

		return this->setupBackground();
	}

	bool BackgroundScrollComponent::setupBackground(void)
	{
		if (nullptr == this->workspaceBackgroundComponent)
		{
			auto& workspaceBackgroundCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<WorkspaceBackgroundComponent>());
			if (nullptr != workspaceBackgroundCompPtr)
			{
				this->workspaceBackgroundComponent = workspaceBackgroundCompPtr.get();
				this->setBackgroundName(this->backgroundName->getString());
			}
			else
			{
				Ogre::String message = "[BackgroundScrollComponent] Could not get prior WorkspaceBackgroundComponent. This component must be set under the WorkspaceBackgroundComponent! Affected game object: "
					+ this->gameObjectPtr->getName();
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, message);

				boost::shared_ptr<EventDataFeedback> eventDataFeedback(new EventDataFeedback(false, message));
				NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataFeedback);
				return false;
			}
		}
		return true;
	}

	GameObjectCompPtr BackgroundScrollComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		// No cloning, this component may only exist once per scene
		return nullptr;
	}

	void BackgroundScrollComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();

		this->workspaceBackgroundComponent = nullptr;
	}

	void BackgroundScrollComponent::onOtherComponentRemoved(unsigned int index)
	{
		auto& gameObjectCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponentByIndex(index));
		if (nullptr != gameObjectCompPtr)
		{
			auto& workspaceBackgroundCompPtr = boost::dynamic_pointer_cast<WorkspaceBackgroundComponent>(gameObjectCompPtr);
			if (nullptr != workspaceBackgroundCompPtr)
			{
				// Remove this component if prior workspace background component has been removed, because this component does not make any sense without workspace background component
				// this->workspaceBackgroundComponent = nullptr;
				this->gameObjectPtr->deleteComponent(this->getClassName());
			}
		}
	}

	bool BackgroundScrollComponent::connect(void)
	{
		GameObjectPtr targetGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->targetId->getULong());
		if (nullptr != targetGameObjectPtr)
		{
			targetGameObjectPtr->setDynamic(true);
			this->targetSceneNode = targetGameObjectPtr->getSceneNode();
		}

		return this->setupBackground();
	}

	bool BackgroundScrollComponent::disconnect(void)
	{
		this->firstTimePositionSet = true;
		this->targetSceneNode = nullptr;

		if (nullptr == this->workspaceBackgroundComponent)
			return true;

		for (size_t i = 0; i < 9; i++)
		{
			// Set uv back to zero
			if (i == this->gameObjectPtr->getOccurrenceIndexFromComponent(this))
			{
				this->workspaceBackgroundComponent->setBackgroundScrollSpeedX(i, 0.0f);
				this->workspaceBackgroundComponent->setBackgroundScrollSpeedY(i, 0.0f);
				// Compiles the materials and its shaders again, so that default uv values are set (uv set to 0)
				this->workspaceBackgroundComponent->compileBackgroundMaterial(i);
				break;
			}
		}
		return true;
	}

	void BackgroundScrollComponent::pause(void)
	{
		// Remember the background scroll position
		this->pausedLastPosition = this->lastPosition;
	}

	void BackgroundScrollComponent::resume(void)
	{
		// Set the remembered background scroll position
		this->lastPosition = this->pausedLastPosition;
	}

	void BackgroundScrollComponent::lateUpdate(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating && true == this->active->getBool() && nullptr != this->workspaceBackgroundComponent)
		{
			bool followGameObjectX = this->followGameObjectX->getBool();
			bool followGameObjectY = this->followGameObjectY->getBool();
			bool followGameObjectZ = this->followGameObjectZ->getBool();

			Ogre::Vector2 absolutePos2D = Ogre::Vector2::ZERO;
			Ogre::Vector2 velocity = Ogre::Vector2::ZERO;

			if (nullptr != this->targetSceneNode)
			{
				Ogre::Vector3 absolutePos3D = this->targetSceneNode->_getDerivedPositionUpdated();
				
				if (true == followGameObjectX || true == followGameObjectY)	
					absolutePos2D = Ogre::Vector2(absolutePos3D.x, absolutePos3D.y);
				else
					absolutePos2D = Ogre::Vector2(absolutePos3D.x, absolutePos3D.z);

				if (this->firstTimePositionSet)
				{
					if (true == followGameObjectX)	
						this->lastPosition.x = absolutePos2D.x;
					if (true == followGameObjectY || true == followGameObjectZ)
						this->lastPosition.y = absolutePos2D.y;

					this->firstTimePositionSet = false;
				}

				if (true == followGameObjectX)
				{
					absolutePos2D.x = NOWA::MathHelper::getInstance()->lowPassFilter(absolutePos2D.x, this->lastPosition.x, 0.1f);
					velocity.x = absolutePos2D.x - this->lastPosition.x;

					this->xScroll += velocity.x * this->moveSpeedX->getReal();

					if (this->xScroll >= 2.0f)
					{
						this->xScroll = 0.0f;
					}
					else if (this->xScroll <= 0.0f)
					{
						this->xScroll = 2.0f;
					}
				}
				if (true == followGameObjectY || true == followGameObjectZ)
				{
					absolutePos2D.y = NOWA::MathHelper::getInstance()->lowPassFilter(absolutePos2D.y, this->lastPosition.y, 0.1f);
					velocity.y = absolutePos2D.y - this->lastPosition.y;

					this->yScroll -= velocity.y * this->moveSpeedY->getReal();

					if (this->yScroll >= 2.0f)
					{
						this->yScroll = 0.0f;
					}
					else if (this->yScroll <= 0.0f)
					{
						this->yScroll = 2.0f;
					}
				}
			}

			if (false == followGameObjectX)
			{
				velocity.x = NOWA::MathHelper::getInstance()->lowPassFilter(this->moveSpeedX->getReal(), this->lastPosition.x, 0.1f);
				this->xScroll += velocity.x;

				if (this->xScroll >= 2.0f)
				{
					this->xScroll = 0.0f;
				}
				else if (this->xScroll <= 0.0f)
				{
					this->xScroll = 2.0f;
				}
			}

			if (false == followGameObjectY && false == followGameObjectZ)
			{
				velocity.y = NOWA::MathHelper::getInstance()->lowPassFilter(this->moveSpeedY->getReal(), this->lastPosition.y, 0.1f);
				this->yScroll -= velocity.y;

				if (this->yScroll >= 2.0f)
				{
					this->yScroll = 0.0f;
				}
				else if (this->yScroll <= 0.0f)
				{
					this->yScroll = 2.0f;
				}
			}

			for (size_t i = 0; i < 9; i++)
			{
				if (i == this->gameObjectPtr->getOccurrenceIndexFromComponent(this))
				{
					this->workspaceBackgroundComponent->setBackgroundScrollSpeedX(i, this->xScroll);
					this->workspaceBackgroundComponent->setBackgroundScrollSpeedY(i, this->yScroll);
					break;
				}
			}

			if (true == followGameObjectX)
				this->lastPosition.x = absolutePos2D.x;
			else
				this->lastPosition.x = velocity.x;

			if (true == followGameObjectY || true == followGameObjectZ)
				this->lastPosition.y = absolutePos2D.y;
			else
				this->lastPosition.y = velocity.y;
		}
	}

	void BackgroundScrollComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (BackgroundScrollComponent::AttrActive() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (BackgroundScrollComponent::AttrTargetId() == attribute->getName())
		{
			this->setTargetId(attribute->getULong());
		}
		else if (BackgroundScrollComponent::AttrMoveSpeedX() == attribute->getName())
		{
			this->setMoveSpeedX(attribute->getReal());
		}
		else if (BackgroundScrollComponent::AttrMoveSpeedY() == attribute->getName())
		{
			this->setMoveSpeedY(attribute->getReal());
		}
		else if (BackgroundScrollComponent::AttrFollowGameObjectX() == attribute->getName())
		{
			this->setFollowGameObjectX(attribute->getBool());
		}
		else if (BackgroundScrollComponent::AttrFollowGameObjectY() == attribute->getName())
		{
			this->setFollowGameObjectY(attribute->getBool());
		}
		else if (BackgroundScrollComponent::AttrFollowGameObjectZ() == attribute->getName())
		{
			this->setFollowGameObjectZ(attribute->getBool());
		}
		else if (BackgroundScrollComponent::AttrBackgroundName() == attribute->getName())
		{
			this->setBackgroundName(attribute->getString());
		}
	}

	void BackgroundScrollComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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
		propertyXML->append_attribute(doc.allocate_attribute("name", "Active"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->active->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TargetId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->targetId->getULong())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MoveSpeedX"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->moveSpeedX->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MoveSpeedY"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->moveSpeedY->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "FollowGameObjectX"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->followGameObjectX->getBool())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "FollowGameObjectY"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->followGameObjectY->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "FollowGameObjectZ"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->followGameObjectZ->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "BackgroundName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->backgroundName->getString())));
		propertiesXML->append_node(propertyXML);
	}

	void BackgroundScrollComponent::setActivated(bool activated)
	{
		this->active->setValue(activated);
	}

	Ogre::String BackgroundScrollComponent::getClassName(void) const
	{
		return "BackgroundScrollComponent";
	}

	Ogre::String BackgroundScrollComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	bool BackgroundScrollComponent::isActivated(void) const
	{
		return this->active->getBool();
	}

	void BackgroundScrollComponent::setTargetId(unsigned long targetId)
	{
		this->targetId->setValue(targetId);
		if (0 == targetId)
		{
			this->setupBackground();
		}
	}

	unsigned long BackgroundScrollComponent::getTargetId(void) const
	{
		return this->targetId->getULong();
	}

	void BackgroundScrollComponent::setMoveSpeedX(Ogre::Real moveSpeedX)
	{
		this->moveSpeedX->setValue(moveSpeedX);
	}

	Ogre::Real BackgroundScrollComponent::getMoveSpeedX(void) const
	{
		return this->moveSpeedX->getReal();
	}

	void BackgroundScrollComponent::setMoveSpeedY(Ogre::Real moveSpeedY)
	{
		this->moveSpeedY->setValue(moveSpeedY);
	}

	Ogre::Real BackgroundScrollComponent::getMoveSpeedY(void) const
	{
		return this->moveSpeedY->getReal();
	}

	void BackgroundScrollComponent::setFollowGameObjectX(bool followGameObjectX)
	{
		this->followGameObjectX->setValue(followGameObjectX);
	}

	bool BackgroundScrollComponent::getFollowGameObjectX(void) const
	{
		return this->followGameObjectX->getBool();
	}

	void BackgroundScrollComponent::setFollowGameObjectY(bool followGameObjectY)
	{
		this->followGameObjectY->setValue(followGameObjectY);
		if (true == followGameObjectY)
		{
			this->followGameObjectZ->setValue(false);
		}
	}

	bool BackgroundScrollComponent::getFollowGameObjectY(void) const
	{
		return this->followGameObjectY->getBool();
	}

	void BackgroundScrollComponent::setFollowGameObjectZ(bool followGameObjectZ)
	{
		this->followGameObjectZ->setValue(followGameObjectZ);
		if (true == followGameObjectZ)
		{
			this->followGameObjectY->setValue(false);
		}
	}

	bool BackgroundScrollComponent::getFollowGameObjectZ(void) const
	{
		return this->followGameObjectZ->getBool();
	}

	void BackgroundScrollComponent::setBackgroundName(const Ogre::String& backgroundName)
	{
		if (false == Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(backgroundName))
		{
			// Sent event with feedback
			boost::shared_ptr<EventDataFeedback> eventDataNavigationMeshFeedback(new EventDataFeedback(false, "#{TextureMissing}"));
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataNavigationMeshFeedback);
			return;
		}

		// Somehow textures with "_" do not work??? like Mountain_Near_2.png
		this->backgroundName->setValue(backgroundName);

		if (nullptr != workspaceBackgroundComponent)
		{
			for (size_t i = 0; i < 9; i++)
			{
				if (i == this->gameObjectPtr->getOccurrenceIndexFromComponent(this))
				{
					this->workspaceBackgroundComponent->changeBackground(i, backgroundName);
					break;
				}
			}
		}
	}

	Ogre::String BackgroundScrollComponent::getBackgroundName(void) const
	{
		return this->backgroundName->getString();
	}

}; // namespace end
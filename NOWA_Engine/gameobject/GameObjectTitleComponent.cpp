#include "NOWAPrecompiled.h"
#include "GameObjectTitleComponent.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "main/AppStateManager.h"

namespace
{
	Ogre::String replaceAll(Ogre::String str, const Ogre::String& from, const Ogre::String& to)
	{
		size_t startPos = 0;
		while ((startPos = str.find(from, startPos)) != Ogre::String::npos)
		{
			str.replace(startPos, from.length(), to);
			startPos += to.length(); // Handles case where 'to' is a substring of 'from'
		}
		return str;
	}
}

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	enum HorizontalAlignment
		{
			H_LEFT, H_CENTER
		};
		enum VerticalAlignment
		{
			V_BELOW, V_ABOVE, V_CENTER
		};

	GameObjectTitleComponent::GameObjectTitleComponent()
		: GameObjectComponent(),
		movableText(nullptr),
		textNode(nullptr),
		orientationTargetGameObject(nullptr),
		fontName(new Variant(GameObjectTitleComponent::AttrFontName(), "BlueHighway", this->attributes)),
		caption(new Variant(GameObjectTitleComponent::AttrCaption(), "MyCaption", this->attributes)),
		charHeight(new Variant(GameObjectTitleComponent::AttrCharHeight(), 0.5f, this->attributes)),
		alwaysPresent(new Variant(GameObjectTitleComponent::AttrAlwaysPresent(), false, this->attributes)),
		offsetPosition(new Variant(GameObjectTitleComponent::AttrOffsetPosition(), Ogre::Vector3::ZERO, this->attributes)),
		offsetOrientation(new Variant(GameObjectTitleComponent::AttrOffsetOrientation(), Ogre::Vector3::ZERO, this->attributes)),
		orientationTargetId(new Variant(GameObjectTitleComponent::AttrOrientationTargetId(), static_cast<unsigned long>(0), this->attributes, true)),
		color(new Variant(GameObjectTitleComponent::AttrColor(), Ogre::Vector4(1.0f, 1.0f, 1.0f, 1.0f), this->attributes)),
		alignment(new Variant(GameObjectTitleComponent::AttrAlignment(), Ogre::Vector2(1.0f, 2.0f), this->attributes))
	{
		this->color->addUserData(GameObject::AttrActionColorDialog());
		this->alignment->setDescription("First value: 0 = Horizontal Left, 1 = Horizontal Center. Second value: 0 = Vertical Below, 1 = Vertical Above, 2 = Vertical Center");
	}

	GameObjectTitleComponent::~GameObjectTitleComponent()
	{
		if (nullptr != this->movableText)
		{
			NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
            {
                NOWA::GraphicsModule::getInstance()->removeTrackedNode(this->textNode);
                this->textNode->detachObject(this->movableText);
                this->gameObjectPtr->getSceneNode()->removeAndDestroyChild(this->textNode);
                this->textNode = nullptr;

                delete this->movableText;
                this->movableText = nullptr;
            };
            GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "GameObjectTitleComponent::~GameObjectTitleComponent");
		}
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[GameObjectTitleComponent] Destructor game object title component for game object: " + this->gameObjectPtr->getName());
	}

	bool GameObjectTitleComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FontName")
		{
			this->fontName->setValue(XMLConverter::getAttrib(propertyElement, "data", "BlueHighway"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Caption")
		{
			this->caption->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CharHeight")
		{
			this->charHeight->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AlwaysPresent")
		{
			this->alwaysPresent->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "YOffset")
		{
			// Does no more exist, but keep the attribute for backward compatibility
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OffsetPosition")
		{
			this->offsetPosition->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OffsetOrientation")
		{
			this->offsetOrientation->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OrientationTargetId")
		{
			this->orientationTargetId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Color")
		{
			this->color->setValue(XMLConverter::getAttribVector4(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Alignment")
		{
			this->alignment->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	bool GameObjectTitleComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[GameObjectTitleComponent] Init game object title component for game object: " + this->gameObjectPtr->getName());

        NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
        {
            Ogre::NameValuePairList params;
            params["name"] = Ogre::StringConverter::toString(this->gameObjectPtr->getId()) + "_MovableText";
            params["Caption"] = this->caption->getString();
            params["fontName"] = this->fontName->getString();

            this->movableText = new MovableText(Ogre::Id::generateNewId<Ogre::MovableObject>(), &this->gameObjectPtr->getSceneManager()->_getEntityMemoryManager(Ogre::SCENE_DYNAMIC), this->gameObjectPtr->getSceneManager(), &params);

            Ogre::String tempCaption = replaceAll(this->caption->getString(), "\\n", "\n");
            if (!tempCaption.empty())
            {
                this->movableText->setCaption(tempCaption);
            }

            this->movableText->setCharacterHeight(this->charHeight->getReal());
            this->movableText->showOnTop(this->alwaysPresent->getBool());

            Ogre::ColourValue colorValue(this->color->getVector4().x, this->color->getVector4().y, this->color->getVector4().z, this->color->getVector4().w);
            this->movableText->setColor(colorValue);

            int h = static_cast<int>(this->alignment->getVector2().x);
            int v = static_cast<int>(this->alignment->getVector2().y);
            this->movableText->setTextAlignment(static_cast<MovableText::HorizontalAlignment>(h), static_cast<MovableText::VerticalAlignment>(v));

            this->movableText->setQueryFlags(0 << 0);

            // textNode is a child of gameObjectPtr->getSceneNode().
            // All positions set on textNode are LOCAL relative to the game object node.
            this->textNode = this->gameObjectPtr->getSceneNode()->createChildSceneNode();
            this->textNode->attachObject(this->movableText);
            NOWA::GraphicsModule::getInstance()->addTrackedNode(this->textNode);

            Ogre::Vector3 sp = this->offsetPosition->getVector3();
            Ogre::Quaternion so = MathHelper::getInstance()->degreesToQuat(this->offsetOrientation->getVector3());

            // Local position = sp only. Parent orientation is inherited automatically.
            // Do NOT multiply by game object orientation o — that would double-rotate.
            this->movableText->getParentSceneNode()->setOrientation(so);
            this->movableText->getParentSceneNode()->setPosition(sp);
        };
        GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "GameObjectTitleComponent::postInit");

        return true;
    }

	bool GameObjectTitleComponent::connect(void)
	{
		bool success = GameObjectComponent::connect();
		if (false == success)
		{
			return success;
		}

		if (0 != this->orientationTargetId->getULong())
		{
			auto gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->orientationTargetId->getULong());
			if (nullptr != gameObjectPtr)
			{
				this->orientationTargetGameObject = gameObjectPtr.get();
			}
		}
		return success;
	}

	bool GameObjectTitleComponent::disconnect(void)
	{
		GameObjectComponent::disconnect();

		// this->textNode->_setDerivedOrientation(this->gameObjectPtr->getOrientation());
		this->orientationTargetGameObject = nullptr;
		return true;
	}

	Ogre::String GameObjectTitleComponent::getClassName(void) const
	{
		return "GameObjectTitleComponent";
	}

	Ogre::String GameObjectTitleComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	GameObjectCompPtr GameObjectTitleComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		GameObjectTitleCompPtr clonedCompPtr(boost::make_shared<GameObjectTitleComponent>());

		clonedCompPtr->setFontName(this->fontName->getString());
		clonedCompPtr->setCaption(this->caption->getString());
		clonedCompPtr->setCharHeight(this->charHeight->getReal());
		clonedCompPtr->setAlwaysPresent(this->alwaysPresent->getBool());
		clonedCompPtr->setColor(this->color->getVector4());
		clonedCompPtr->setAlignment(this->alignment->getVector2());
		clonedCompPtr->setOffsetPosition(this->offsetPosition->getVector3());
		clonedCompPtr->setOffsetOrientation(this->offsetOrientation->getVector3());
		clonedCompPtr->setOrientationTargetId(this->orientationTargetId->getULong());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool GameObjectTitleComponent::onCloned(void)
	{
		// Search for the prior id of the cloned game object and set the new id and set the new id, if not found set better 0, else the game objects may be corrupt!
		// Attention: How about parent id etc. because when a widget is cloned, its internal id will be re-generated, so the parent id from another widget that should point to this widget is no more valid!
		GameObjectPtr gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getClonedGameObjectFromPriorId(this->orientationTargetId->getULong());
		if (nullptr != gameObjectPtr)
		{
			this->orientationTargetId->setValue(gameObjectPtr->getId());
		}
		else
		{
			this->orientationTargetId->setValue(static_cast<unsigned long>(0));
		}

		// Since connect is called during cloning process, it does not make sense to process furher here, but only when simulation started!
		return true;
	}

	void GameObjectTitleComponent::update(Ogre::Real dt, bool notSimulating)
    {
        if (false == notSimulating)
        {
            if (nullptr != this->orientationTargetGameObject)
            {
                Ogre::Vector3 sp = this->offsetPosition->getVector3();
                Ogre::Quaternion so = MathHelper::getInstance()->degreesToQuat(this->offsetOrientation->getVector3());

                // NOTE: getWorldTransforms() in MovableText always billboards to camera,
                // so rotating textNode orientation has no visual effect on text facing.
                // The orientationTargetId feature controls where the text is POSITIONED
                // relative to the game object — it offsets sp in the direction toward
                // the target so the label appears on the side facing the target.
                //
                // If you want the text to appear on the side of the object facing the
                // target, compute a local offset in that direction:
                Ogre::Vector3 toTarget = this->orientationTargetGameObject->getPosition() - this->gameObjectPtr->getPosition();
                toTarget.y = 0.0f;
                if (toTarget.squaredLength() > 1e-6f)
                {
                    toTarget.normalise();
                }

                // Offset the text toward the target in the XZ plane, plus sp.y upward
                Ogre::Vector3 localOffset = toTarget * sp.length() + Ogre::Vector3(0.0f, sp.y, 0.0f);

                // textNode is child of game object node — all coords are local.
                // Convert world-space toTarget direction to local space of parent node.
                Ogre::Quaternion parentWorldOrient = this->gameObjectPtr->getSceneNode()->_getDerivedOrientation();
                Ogre::Vector3 localDir = parentWorldOrient.Inverse() * localOffset;

                NOWA::GraphicsModule::getInstance()->updateNodeOrientation(this->movableText->getParentSceneNode(), so);
                NOWA::GraphicsModule::getInstance()->updateNodePosition(this->movableText->getParentSceneNode(), localDir);
            }
        }
    }

	void GameObjectTitleComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (GameObjectTitleComponent::AttrFontName() == attribute->getName())
		{
			this->setFontName(attribute->getString());
		}
		else if (GameObjectTitleComponent::AttrCaption() == attribute->getName())
		{
			this->setCaption(attribute->getString());
		}
		else if (GameObjectTitleComponent::AttrCharHeight() == attribute->getName())
		{
			this->setCharHeight(attribute->getReal());
		}
		else if (GameObjectTitleComponent::AttrAlwaysPresent() == attribute->getName())
		{
			this->setAlwaysPresent(attribute->getBool());
		}
		else if (GameObjectTitleComponent::AttrOffsetPosition() == attribute->getName())
		{
			this->setOffsetPosition(attribute->getVector3());
		}
		else if (GameObjectTitleComponent::AttrOffsetOrientation() == attribute->getName())
		{
			this->setOffsetOrientation(attribute->getVector3());
		}
		else if (GameObjectTitleComponent::AttrOrientationTargetId() == attribute->getName())
		{
			this->setOrientationTargetId(attribute->getULong());
		}
		else if (GameObjectTitleComponent::AttrColor() == attribute->getName())
		{
			this->setColor(attribute->getVector4());
		}
		else if (GameObjectTitleComponent::AttrAlignment() == attribute->getName())
		{
			this->setAlignment(attribute->getVector2());
		}
	}

	void GameObjectTitleComponent::writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		GameObjectComponent::writeXML(propertiesXML, doc);

		xml_node<>* propertyXML = doc.allocate_node(rapidxml::node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "FontName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->fontName->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(rapidxml::node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Caption"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->caption->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(rapidxml::node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "CharHeight"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->charHeight->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(rapidxml::node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AlwaysPresent"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->alwaysPresent->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OffsetPosition"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->offsetPosition->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OffsetOrientation"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->offsetOrientation->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OrientationTargetId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->orientationTargetId->getULong())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(rapidxml::node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "10"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Color"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->color->getVector4())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(rapidxml::node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Alignment"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->alignment->getVector2())));
		propertiesXML->append_node(propertyXML);
	}

	void GameObjectTitleComponent::setFontName(const Ogre::String& fontName)
	{
		this->fontName->setValue(fontName);
		if (this->movableText)
		{
			NOWA::GraphicsModule::RenderCommand renderCommand = [this, fontName]()
            {
                this->movableText->setFontName(fontName);
            };
            GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "GameObjectTitleComponent::setFontName");
		}
	}

	Ogre::String GameObjectTitleComponent::getFontName(void) const
	{
		return this->fontName->getString();
	}

	void GameObjectTitleComponent::setCaption(const Ogre::String& caption)
	{
		Ogre::String tempCaption = replaceAll(caption, "\\n", "\n");
		this->caption->setValue(tempCaption);
		if (true == tempCaption.empty())
		{
			// Empty string causes crash with vertices in movabletext^^
			tempCaption = " ";
		}
		if (this->movableText)
		{
			this->movableText->setCaption(tempCaption);
		}
	}

	Ogre::String GameObjectTitleComponent::getCaption(void) const
	{
		return this->caption->getString();
	}

	void GameObjectTitleComponent::setAlwaysPresent(bool alwaysPresent)
	{
		this->alwaysPresent->setValue(alwaysPresent);
		Ogre::ColourValue colourValue(this->color->getVector4().x, this->color->getVector4().y, this->color->getVector4().z, this->color->getVector4().w);
		if (this->movableText)
		{
			NOWA::GraphicsModule::RenderCommand renderCommand = [this, alwaysPresent]()
            {
                this->movableText->showOnTop(alwaysPresent);
            };
            GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "GameObjectTitleComponent::setAlwaysPresent");
		}
	}

	bool GameObjectTitleComponent::getAlwaysPresent(void) const
	{
		return this->alwaysPresent->getBool();
	}

	void GameObjectTitleComponent::setCharHeight(Ogre::Real charHeight)
	{
		this->charHeight->setValue(charHeight);
		if (this->movableText)
		{
			this->movableText->setCharacterHeight(this->charHeight->getReal());
		}
	}

	Ogre::Real GameObjectTitleComponent::getCharHeight(void) const
	{
		return this->charHeight->getReal();
	}

	void GameObjectTitleComponent::setOffsetPosition(const Ogre::Vector3& offsetPosition)
    {
        this->offsetPosition->setValue(offsetPosition);

        if (this->movableText)
        {
            Ogre::Vector3 sp = this->offsetPosition->getVector3();
            Ogre::Quaternion so = MathHelper::getInstance()->degreesToQuat(this->offsetOrientation->getVector3());

            // Local position relative to parent — sp only, no world offset.
            NOWA::GraphicsModule::getInstance()->updateNodeOrientation(this->movableText->getParentSceneNode(), so);
            NOWA::GraphicsModule::getInstance()->updateNodePosition(this->movableText->getParentSceneNode(), sp);
        }
    }

	Ogre::Vector3 GameObjectTitleComponent::getOffsetPosition(void) const
	{
		return this->offsetPosition->getVector3();
	}

	void GameObjectTitleComponent::setOffsetOrientation(const Ogre::Vector3& offsetOrientation)
    {
        this->offsetOrientation->setValue(offsetOrientation);

        if (this->movableText)
        {
            Ogre::Vector3 sp = this->offsetPosition->getVector3();
            Ogre::Quaternion so = MathHelper::getInstance()->degreesToQuat(this->offsetOrientation->getVector3());

            // Local position relative to parent — sp only, no world offset.
            NOWA::GraphicsModule::getInstance()->updateNodeOrientation(this->movableText->getParentSceneNode(), so);
            NOWA::GraphicsModule::getInstance()->updateNodePosition(this->movableText->getParentSceneNode(), sp);
        }
    }

	Ogre::Vector3 GameObjectTitleComponent::getOffsetOrientation(void) const
	{
		return this->offsetOrientation->getVector3();
	}

	void GameObjectTitleComponent::setOrientationTargetId(unsigned long targetId)
	{
		this->orientationTargetId->setValue(targetId);
		if (0 == targetId)
		{
			this->orientationTargetGameObject = nullptr;
		}
	}

	unsigned long GameObjectTitleComponent::getOrientationTargetId(unsigned int id) const
	{
		return this->orientationTargetId->getULong();
	}

	Ogre::Quaternion GameObjectTitleComponent::getTargetIdOrientation(void)
	{
		if (nullptr != this->orientationTargetGameObject)
		{
			return this->orientationTargetGameObject->getOrientation();
		}
		if (nullptr != this->gameObjectPtr)
		{
			return this->gameObjectPtr->getOrientation();
		}
		return Ogre::Quaternion::IDENTITY;
	}

	void GameObjectTitleComponent::setColor(const Ogre::Vector4& color)
	{
		this->color->setValue(color);
		if (this->movableText)
		{
			this->movableText->setColor(Ogre::ColourValue(color.x, color.y, color.z, color.w));
		}
	}

	Ogre::Vector4 GameObjectTitleComponent::getColor(void) const
	{
		return this->color->getVector4();
	}

	void GameObjectTitleComponent::setAlignment(const Ogre::Vector2& alignment)
	{
		this->alignment->setValue(alignment);
		if (this->movableText)
		{
			int h = static_cast<int>(this->alignment->getVector2().x);
			int v = static_cast<int>(this->alignment->getVector2().y);
			this->movableText->setTextAlignment(static_cast<MovableText::HorizontalAlignment>(h), static_cast<MovableText::VerticalAlignment>(v));
		}
	}

	Ogre::Vector2 GameObjectTitleComponent::getAlignment(void) const
	{
		return this->alignment->getVector2();
	}

	MovableText* GameObjectTitleComponent::getMovableText(void) const
	{
		return this->movableText;
	}

}; //namespace end
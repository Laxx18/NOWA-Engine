#include "NOWAPrecompiled.h"
#include "ValueBarComponent.h"
#include "GameObjectController.h"
#include "GameObjectTitleComponent.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "LuaScriptComponent.h"
#include "main/AppStateManager.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;
	
	ValueBarComponent::ValueBarComponent()
		: GameObjectComponent(),
		lineNode(nullptr),
		manualObject(nullptr),
		indices(0),
		orientationTargetGameObject(nullptr),
		gameObjectTitleComponent(nullptr),
		isInSimulation(false),
		activated(new Variant(ValueBarComponent::AttrActivated(), true, this->attributes)),
		twoSided(new Variant(ValueBarComponent::AttrTwoSided(), true, this->attributes)),
		innerColor(new Variant(ValueBarComponent::AttrInnerColor(), Ogre::Vector3(1.0f, 0.0f, 0.0f), this->attributes)),
		outerColor(new Variant(ValueBarComponent::AttrOuterColor(), Ogre::Vector3::UNIT_SCALE, this->attributes)),
		borderSize(new Variant(ValueBarComponent::AttrBorderSize(), 0.25f, this->attributes)),
		offsetPosition(new Variant(ValueBarComponent::AttrOffsetPosition(), Ogre::Vector3::ZERO, this->attributes)),
		offsetOrientation(new Variant(ValueBarComponent::AttrOffsetOrientation(), Ogre::Vector3::ZERO, this->attributes)),
		orientationTargetId(new Variant(ValueBarComponent::AttrOrientationTargetId(), static_cast<unsigned long>(0), this->attributes, true)),
		width(new Variant(ValueBarComponent::AttrWidth(), 10.0f, this->attributes)),
		height(new Variant(ValueBarComponent::AttrHeight(), 2.0f, this->attributes)),
		maxValue(new Variant(ValueBarComponent::AttrMaxValue(), static_cast<unsigned int>(100), this->attributes)),
		currentValue(new Variant(ValueBarComponent::AttrCurrentValue(), static_cast<unsigned int>(50), this->attributes))
	{
		this->innerColor->addUserData(GameObject::AttrActionColorDialog());
		this->outerColor->addUserData(GameObject::AttrActionColorDialog());
	}

	ValueBarComponent::~ValueBarComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ValueBarComponent] Destructor value bar component for game object: " + this->gameObjectPtr->getName());
		
		this->destroyValueBar();
	}

	bool ValueBarComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TwoSided")
		{
			this->twoSided->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "InnerColor")
		{
			this->innerColor->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OuterColor")
		{
			this->outerColor->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "BorderSize")
		{
			this->borderSize->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
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
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Width")
		{
			this->width->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Height")
		{
			this->height->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MaxValue")
		{
			this->maxValue->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CurrentValue")
		{
			this->currentValue->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	GameObjectCompPtr ValueBarComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		ValueBarCompPtr clonedCompPtr(boost::make_shared<ValueBarComponent>());

		clonedCompPtr->setTwoSided(this->twoSided->getBool());
		clonedCompPtr->setInnerColor(this->innerColor->getVector3());
		clonedCompPtr->setOuterColor(this->outerColor->getVector3());
		clonedCompPtr->setBorderSize(this->borderSize->getReal());
		clonedCompPtr->setOffsetPosition(this->offsetPosition->getVector3());
		clonedCompPtr->setOffsetOrientation(this->offsetOrientation->getVector3());
		clonedCompPtr->setOrientationTargetId(this->orientationTargetId->getULong());
		clonedCompPtr->setWidth(this->width->getReal());
		clonedCompPtr->setHeight(this->height->getReal());
		clonedCompPtr->setMaxValue(this->maxValue->getUInt());
		clonedCompPtr->setCurrentValue(this->currentValue->getUInt());

		clonedCompPtr->setActivated(this->activated->getBool());
		
		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool ValueBarComponent::onCloned(void)
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

	bool ValueBarComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ValueBarComponent] Init value bar component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool ValueBarComponent::connect(void)
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

		auto gameObjectTitleCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<GameObjectTitleComponent>());
		if (nullptr != gameObjectTitleCompPtr)
		{
			this->gameObjectTitleComponent = gameObjectTitleCompPtr.get();
			this->gameObjectTitleComponent->setLookAtCamera(false);
		}

		this->createValueBar();

		this->isInSimulation = true;
		
		return success;
	}

	bool ValueBarComponent::disconnect(void)
	{
		this->isInSimulation = false;

		this->destroyValueBar();

		this->orientationTargetGameObject = nullptr;
		this->gameObjectTitleComponent = nullptr;
		
		return true;
	}

	void ValueBarComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
			if (nullptr == this->manualObject)
			{
				return;
			}

			this->indices = 0;
			if (this->manualObject->getNumSections() > 0)
			{
				this->manualObject->beginUpdate(0);
			}
			else
			{
				this->manualObject->clear();
				this->manualObject->begin("WhiteNoLightingBackground", Ogre::OT_TRIANGLE_LIST);
			}

			this->drawValueBar();

			// Realllllllyyyyy important! Else the rectangle is a whole mess!
			this->manualObject->index(0);

			this->manualObject->end();
		}
	}

	void ValueBarComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (ValueBarComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (ValueBarComponent::AttrTwoSided() == attribute->getName())
		{
			this->setTwoSided(attribute->getBool());
		}
		else if (ValueBarComponent::AttrInnerColor() == attribute->getName())
		{
			this->setInnerColor(attribute->getVector3());
		}
		else if (ValueBarComponent::AttrOuterColor() == attribute->getName())
		{
			this->setOuterColor(attribute->getVector3());
		}
		else if (ValueBarComponent::AttrOffsetPosition() == attribute->getName())
		{
			this->setOffsetPosition(attribute->getVector3());
		}
		else if (ValueBarComponent::AttrOffsetOrientation() == attribute->getName())
		{
			this->setOffsetOrientation(attribute->getVector3());
		}
		else if (ValueBarComponent::AttrOrientationTargetId() == attribute->getName())
		{
			this->setOrientationTargetId(attribute->getULong());
		}
		else if (ValueBarComponent::AttrBorderSize() == attribute->getName())
		{
			this->setBorderSize(attribute->getReal());
		}
		else if (ValueBarComponent::AttrWidth() == attribute->getName())
		{
			this->setWidth(attribute->getReal());
		}
		else if (ValueBarComponent::AttrHeight() == attribute->getName())
		{
			this->setHeight(attribute->getReal());
		}
		else if (ValueBarComponent::AttrMaxValue() == attribute->getName())
		{
			this->setMaxValue(attribute->getUInt());
		}
		else if (ValueBarComponent::AttrCurrentValue() == attribute->getName())
		{
			this->setCurrentValue(attribute->getUInt());
		}
	}

	void ValueBarComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TwoSided"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->twoSided->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "InnerColor"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->innerColor->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OuterColor"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->outerColor->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "BorderSize"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->borderSize->getReal())));
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

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Width"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->width->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Height"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->height->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MaxValue"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxValue->getUInt())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "CurrentValue"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->currentValue->getUInt())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String ValueBarComponent::getClassName(void) const
	{
		return "ValueBarComponent";
	}

	Ogre::String ValueBarComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void ValueBarComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);

		if (true == this->isInSimulation)
		{
			if (false == activated)
			{
				this->destroyValueBar();
			}
			else
			{
				this->createValueBar();
			}
		}
	}

	bool ValueBarComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void ValueBarComponent::setTwoSided(bool twoSided)
	{
		this->twoSided->setValue(twoSided);
	}

	bool ValueBarComponent::getTwoSided(void) const
	{
		return this->twoSided->getBool();
	}

	void ValueBarComponent::setInnerColor(const Ogre::Vector3& color)
	{
		this->innerColor->setValue(color);
	}

	Ogre::Vector3 ValueBarComponent::getInnerColor(void) const
	{
		return this->innerColor->getVector3();
	}

	void ValueBarComponent::setOuterColor(const Ogre::Vector3& color)
	{
		this->outerColor->setValue(color);
	}

	Ogre::Vector3 ValueBarComponent::getOuterColor(void) const
	{
		return this->outerColor->getVector3();
	}

	void ValueBarComponent::setBorderSize(Ogre::Real borderSize)
	{
		if (borderSize < 0.0f)
			borderSize = 0.0f;
		if (borderSize > 10.0f)
			borderSize = 10.0f;
		this->borderSize->setValue(borderSize);
	}

	Ogre::Real ValueBarComponent::getBorderSize(void) const
	{
		return this->borderSize->getReal();
	}

	void ValueBarComponent::setOffsetPosition(const Ogre::Vector3& offsetPosition)
	{
		this->offsetPosition->setValue(offsetPosition);
	}

	Ogre::Vector3 ValueBarComponent::getOffsetPosition(void) const
	{
		return this->offsetPosition->getVector3();
	}

	void ValueBarComponent::setOffsetOrientation(const Ogre::Vector3& offsetOrientation)
	{
		this->offsetOrientation->setValue(offsetOrientation);
	}

	Ogre::Vector3 ValueBarComponent::getOffsetOrientation(void) const
	{
		return this->offsetOrientation->getVector3();
	}

	void ValueBarComponent::setOrientationTargetId(unsigned long targetId)
	{
		this->orientationTargetId->setValue(targetId);
	}

	unsigned long ValueBarComponent::getOrientationTargetId(unsigned int index) const
	{
		return this->orientationTargetId->getULong();
	}

	void ValueBarComponent::setWidth(Ogre::Real width)
	{
		if (width < 1.0f)
			width = 1.0f;
		this->width->setValue(width);
	}

	Ogre::Real ValueBarComponent::getWidth(void) const
	{
		return this->width->getReal();
	}

	void ValueBarComponent::setHeight(Ogre::Real height)
	{
		if (height < 1.0f)
			height = 1.0f;
		this->height->setValue(height);
	}

	Ogre::Real ValueBarComponent::getHeight(void) const
	{
		return this->height->getReal();
	}

	void ValueBarComponent::setMaxValue(unsigned int maxValue)
	{
		this->maxValue->setValue(maxValue);
	}

	unsigned int ValueBarComponent::getMaxValue(void) const
	{
		return this->maxValue->getUInt();
	}

	void ValueBarComponent::setCurrentValue(unsigned int currentValue)
	{
		if (currentValue > this->maxValue->getUInt())
			currentValue = this->maxValue->getUInt();
		this->currentValue->setValue(currentValue);
	}

	unsigned int ValueBarComponent::getCurrentValue(void) const
	{
		return this->currentValue->getUInt();
	}

	void ValueBarComponent::drawValueBar(void)
	{
		Ogre::Vector3 tempInnerColor = this->innerColor->getVector3();
		Ogre::Vector3 tempOuterColor = this->outerColor->getVector3();

		// Outer Rectangle
		//	//    lto-------rto
		//	//    |lti----rti||
		//	//    ||         || height + borderSize
		//	//    |lbi----rbi||
		//	//    lbo---x---rbo
		//           width + borderSize
		// x = game object position

		Ogre::Vector3 p = this->gameObjectPtr->getPosition();
		Ogre::Quaternion o = this->gameObjectPtr->getOrientation();

		Ogre::Vector3 sp = this->offsetPosition->getVector3();
		Ogre::Quaternion so = Ogre::Quaternion::IDENTITY;

		if (nullptr != this->orientationTargetGameObject)
		{
			Ogre::Vector3 direction = this->gameObjectPtr->getPosition() - this->orientationTargetGameObject->getPosition();
			
			so = MathHelper::getInstance()->lookAt(direction) * MathHelper::getInstance()->degreesToQuat(this->offsetOrientation->getVector3());
			// Remove orientation of game object, so that the value bar will always face the target, no matter how the game object is orientated
			o = Ogre::Quaternion::IDENTITY;
		}
		else
		{
			so = MathHelper::getInstance()->degreesToQuat(this->offsetOrientation->getVector3());
		}

		if (nullptr != this->gameObjectTitleComponent)
		{
			this->gameObjectTitleComponent->getMovableText()->setTextYOffset(-1.0f);
			this->gameObjectTitleComponent->getMovableText()->getParentSceneNode()->_setDerivedPosition(p + (o * (so * sp)));
			this->gameObjectTitleComponent->getMovableText()->getParentSceneNode()->_setDerivedOrientation(so);
		}

		Ogre::Real bs = this->borderSize->getReal();

		// Outer rectangle
		{
			Ogre::Real w = this->width->getReal() * 0.5f;
			Ogre::Real h = this->height->getReal() * 0.5f;

			// Upper half for outer rectangle
			// 1) lt
			this->manualObject->position(p + (o * (so * (sp + Ogre::Vector3(-w - bs, h + bs, 0.0f)))));
			this->manualObject->colour(Ogre::ColourValue(tempOuterColor.x, tempOuterColor.y, tempOuterColor.z, 1.0f));
			this->manualObject->normal(0.0f, 0.0f, 1.0f);
			this->manualObject->textureCoord(0, 0);

			// 2) lb
			this->manualObject->position(p + (o * (so * (sp + Ogre::Vector3(-w - bs, h + bs, 0.0f)))));
			this->manualObject->colour(Ogre::ColourValue(tempOuterColor.x, tempOuterColor.y, tempOuterColor.z, 1.0f));
			this->manualObject->normal(0.0f, 0.0f, 1.0f);
			this->manualObject->textureCoord(0, 1);

			// 3) rb
			this->manualObject->position(p + (o * (so * (sp + Ogre::Vector3(w + bs, h + bs, 0.0f)))));
			this->manualObject->colour(Ogre::ColourValue(tempOuterColor.x, tempOuterColor.y, tempOuterColor.z, 1.0f));
			this->manualObject->normal(0.0f, 0.0f, 1.0f);
			this->manualObject->textureCoord(1, 1);

			// 4) rt
			this->manualObject->position(p + (o * (so * (sp + Ogre::Vector3(w + bs, h + bs, 0.0f)))));
			this->manualObject->colour(Ogre::ColourValue(tempOuterColor.x, tempOuterColor.y, tempOuterColor.z, 1.0f));
			this->manualObject->normal(0.0f, 0.0f, 1.0f);
			this->manualObject->textureCoord(1, 0);

			// Internally constructs 2 triangles
			this->manualObject->quad(this->indices + 0, this->indices + 1, this->indices + 2, this->indices + 3);
			this->indices += 4;

			// Lower half for outher rectangle
			// 1) lt
			this->manualObject->position(p + (o * (so * (sp + Ogre::Vector3(-w - bs, h + bs, 0.0f)))));
			this->manualObject->colour(Ogre::ColourValue(tempOuterColor.x, tempOuterColor.y, tempOuterColor.z, 1.0f));
			this->manualObject->normal(0.0f, 0.0f, 1.0f);
			this->manualObject->textureCoord(0, 0);

			// 2) lb
			this->manualObject->position(p + (o * (so * (sp + Ogre::Vector3(-w - bs, -bs, 0.0f)))));
			this->manualObject->colour(Ogre::ColourValue(tempOuterColor.x, tempOuterColor.y, tempOuterColor.z, 1.0f));
			this->manualObject->normal(0.0f, 0.0f, 1.0f);
			this->manualObject->textureCoord(0, 1);

			// 3) rb
			this->manualObject->position(p + (o * (so * (sp + Ogre::Vector3(w + bs, -bs, 0.0f)))));
			this->manualObject->colour(Ogre::ColourValue(tempOuterColor.x, tempOuterColor.y, tempOuterColor.z, 1.0f));
			this->manualObject->normal(0.0f, 0.0f, 1.0f);
			this->manualObject->textureCoord(1, 1);

			// 4) rt
			this->manualObject->position(p + (o * (so * (sp + Ogre::Vector3(w + bs, h + bs, 0.0f)))));
			this->manualObject->colour(Ogre::ColourValue(tempOuterColor.x, tempOuterColor.y, tempOuterColor.z, 1.0f));
			this->manualObject->normal(0.0f, 0.0f, 1.0f);
			this->manualObject->textureCoord(1, 0);

			this->manualObject->quad(this->indices + 0, this->indices + 1, this->indices + 2, this->indices + 3);
			this->indices += 4;

			if (true == this->twoSided->getBool())
			{
				// Upper half for outer rectangle
				// 1) rt
				this->manualObject->position(p + (o * (so * (sp + Ogre::Vector3(w + bs, h + bs, 0.0f)))));
				this->manualObject->colour(Ogre::ColourValue(tempOuterColor.x, tempOuterColor.y, tempOuterColor.z, 1.0f));
				this->manualObject->normal(0.0f, 0.0f, -1.0f);
				this->manualObject->textureCoord(0, 0);

				// 2) rb
				this->manualObject->position(p + (o * (so * (sp + Ogre::Vector3(w + bs, h + bs, 0.0f)))));
				this->manualObject->colour(Ogre::ColourValue(tempOuterColor.x, tempOuterColor.y, tempOuterColor.z, 1.0f));
				this->manualObject->normal(0.0f, 0.0f, -1.0f);
				this->manualObject->textureCoord(0, 1);

				// 3) lb
				this->manualObject->position(p + (o * (so * (sp + Ogre::Vector3(-w - bs, h + bs, 0.0f)))));
				this->manualObject->colour(Ogre::ColourValue(tempOuterColor.x, tempOuterColor.y, tempOuterColor.z, 1.0f));
				this->manualObject->normal(0.0f, 0.0f, -1.0f);
				this->manualObject->textureCoord(1, 1);

				// 4) lt
				this->manualObject->position(p + (o * (so * (sp + Ogre::Vector3(-w - bs, h + bs, 0.0f)))));
				this->manualObject->colour(Ogre::ColourValue(tempOuterColor.x, tempOuterColor.y, tempOuterColor.z, 1.0f));
				this->manualObject->normal(0.0f, 0.0f, -1.0f);
				this->manualObject->textureCoord(1, 0);

				this->manualObject->quad(this->indices + 0, this->indices + 1, this->indices + 2, this->indices + 3);
				this->indices += 4;

				// Lower half for outer rectangle
				// 1) rt
				this->manualObject->position(p + (o * (so * (sp + Ogre::Vector3(w + bs, h + bs, 0.0f)))));
				this->manualObject->colour(Ogre::ColourValue(tempOuterColor.x, tempOuterColor.y, tempOuterColor.z, 1.0f));
				this->manualObject->normal(0.0f, 0.0f, -1.0f);
				this->manualObject->textureCoord(0, 0);

				// 2) rb
				this->manualObject->position(p + (o * (so * (sp + Ogre::Vector3(w + bs, -bs, 0.0f)))));
				this->manualObject->colour(Ogre::ColourValue(tempOuterColor.x, tempOuterColor.y, tempOuterColor.z, 1.0f));
				this->manualObject->normal(0.0f, 0.0f, -1.0f);
				this->manualObject->textureCoord(0, 1);

				// 3) lb
				this->manualObject->position(p + (o * (so * (sp + Ogre::Vector3(-w - bs, -bs, 0.0f)))));
				this->manualObject->colour(Ogre::ColourValue(tempOuterColor.x, tempOuterColor.y, tempOuterColor.z, 1.0f));
				this->manualObject->normal(0.0f, 0.0f, -1.0f);
				this->manualObject->textureCoord(1, 1);

				// 4) lt
				this->manualObject->position(p + (o * (so * (sp + Ogre::Vector3(-w - bs, h + bs, 0.0f)))));
				this->manualObject->colour(Ogre::ColourValue(tempOuterColor.x, tempOuterColor.y, tempOuterColor.z, 1.0f));
				this->manualObject->normal(0.0f, 0.0f, -1.0f);
				this->manualObject->textureCoord(1, 0);

				this->manualObject->quad(this->indices + 0, this->indices + 1, this->indices + 2, this->indices + 3);
				this->indices += 4;
			}
		}

		// Inner Rectangle
		//	//    lti-------rti 
		//	//    |          |
		//	//    |          | height
		//	//    |          |
		//	//    lbi---x---rbi
		//           width
		// x = game object position
		{
			Ogre::Real w = -(this->width->getReal() * 0.5f) + (this->width->getReal() * static_cast<Ogre::Real>(this->currentValue->getReal() / static_cast<Ogre::Real>(this->maxValue->getReal())));
			Ogre::Real h = this->height->getReal() * 0.5f;

			Ogre::Real v = -this->width->getReal() * 0.5f;


			// Upper half for inner rectangle
			// 1) lt
			this->manualObject->position(p + (o * (so * (sp + ( Ogre::Vector3(v , h, 0.0f))))));
			this->manualObject->colour(Ogre::ColourValue(tempInnerColor.x, tempInnerColor.y, tempInnerColor.z, 1.0f));
			this->manualObject->normal(0.0f, 0.0f, 1.0f);
			this->manualObject->textureCoord(0, 0);

			// 2) lb
			this->manualObject->position(p + (o * (so * (sp + ( Ogre::Vector3(v , h, 0.0f))))));
			this->manualObject->colour(Ogre::ColourValue(tempInnerColor.x, tempInnerColor.y, tempInnerColor.z, 1.0f));
			this->manualObject->normal(0.0f, 0.0f, 1.0f);
			this->manualObject->textureCoord(0, 1);

			// 3) rb
			this->manualObject->position(p + (o * (so * (sp + ( Ogre::Vector3(w, h, 0.0f))))));
			this->manualObject->colour(Ogre::ColourValue(tempInnerColor.x, tempInnerColor.y, tempInnerColor.z, 1.0f));
			this->manualObject->normal(0.0f, 0.0f, 1.0f);
			this->manualObject->textureCoord(1, 1);

			// 4) rt
			this->manualObject->position(p + (o * (so * (sp + (Ogre::Vector3(w, h, 0.0f))))));
			this->manualObject->colour(Ogre::ColourValue(tempInnerColor.x, tempInnerColor.y, tempInnerColor.z, 1.0f));
			this->manualObject->normal(0.0f, 0.0f, 1.0f);
			this->manualObject->textureCoord(1, 0);

			// Internally constructs 2 triangles
			this->manualObject->quad(this->indices + 0, this->indices + 1, this->indices + 2, this->indices + 3);
			this->indices += 4;

			// Lower half for inner rectangle
			// 1) lt
			this->manualObject->position(p + (o * (so * (sp + ( Ogre::Vector3(v , h, 0.0f))))));
			this->manualObject->colour(Ogre::ColourValue(tempInnerColor.x, tempInnerColor.y, tempInnerColor.z, 1.0f));
			this->manualObject->normal(0.0f, 0.0f, 1.0f);
			this->manualObject->textureCoord(0, 0);

			// 2) lb
			this->manualObject->position(p + (o * (so * (sp + ( Ogre::Vector3(v , 0.0f, 0.0f))))));
			this->manualObject->colour(Ogre::ColourValue(tempInnerColor.x, tempInnerColor.y, tempInnerColor.z, 1.0f));
			this->manualObject->normal(0.0f, 0.0f, 1.0f);
			this->manualObject->textureCoord(0, 1);

			// 3) rb
			this->manualObject->position(p + (o * (so * (sp + ( Ogre::Vector3(w, 0.0f, 0.0f))))));
			this->manualObject->colour(Ogre::ColourValue(tempInnerColor.x, tempInnerColor.y, tempInnerColor.z, 1.0f));
			this->manualObject->normal(0.0f, 0.0f, 1.0f);
			this->manualObject->textureCoord(1, 1);

			// 4) rt
			this->manualObject->position(p + (o * (so * (sp + ( Ogre::Vector3(w, h, 0.0f))))));
			this->manualObject->colour(Ogre::ColourValue(tempInnerColor.x, tempInnerColor.y, tempInnerColor.z, 1.0f));
			this->manualObject->normal(0.0f, 0.0f, 1.0f);
			this->manualObject->textureCoord(1, 0);

			this->manualObject->quad(this->indices + 0, this->indices + 1, this->indices + 2, this->indices + 3);
			this->indices += 4;

			if (true == this->twoSided->getBool())
			{
				Ogre::Real w = this->width->getReal() * 0.5f;
				Ogre::Real h = this->height->getReal() * 0.5f;

				Ogre::Real v = (this->width->getReal() * 0.5f) - (this->width->getReal() * static_cast<Ogre::Real>(this->currentValue->getReal() / static_cast<Ogre::Real>(this->maxValue->getReal())));

				// Upper half for inner rectangle
				// 1) rt
				this->manualObject->position(p + (o * (so * (sp + ( Ogre::Vector3(w, h, 0.0f))))));
				this->manualObject->colour(Ogre::ColourValue(tempInnerColor.x, tempInnerColor.y, tempInnerColor.z, 1.0f));
				this->manualObject->normal(0.0f, 0.0f, -1.0f);
				this->manualObject->textureCoord(0, 0);

				// 2) rb
				this->manualObject->position(p + (o * (so * (sp + ( Ogre::Vector3(w, h, 0.0f))))));
				this->manualObject->colour(Ogre::ColourValue(tempInnerColor.x, tempInnerColor.y, tempInnerColor.z, 1.0f));
				this->manualObject->normal(0.0f, 0.0f, -1.0f);
				this->manualObject->textureCoord(0, 1);

				// 3) lb
				this->manualObject->position(p + (o * (so * (sp + (Ogre::Vector3(v , h, 0.0f))))));
				this->manualObject->colour(Ogre::ColourValue(tempInnerColor.x, tempInnerColor.y, tempInnerColor.z, 1.0f));
				this->manualObject->normal(0.0f, 0.0f, -1.0f);
				this->manualObject->textureCoord(1, 1);

				// 4) lt
				this->manualObject->position(p + (o * (so * (sp + (Ogre::Vector3(v , h, 0.0f))))));
				this->manualObject->colour(Ogre::ColourValue(tempInnerColor.x, tempInnerColor.y, tempInnerColor.z, 1.0f));
				this->manualObject->normal(0.0f, 0.0f, -1.0f);
				this->manualObject->textureCoord(1, 0);

				this->manualObject->quad(this->indices + 0, this->indices + 1, this->indices + 2, this->indices + 3);
				this->indices += 4;

				// Lower half for inner rectangle
				// 1) rt
				this->manualObject->position(p + (o * (so * (sp + (Ogre::Vector3(w, h, 0.0f))))));
				this->manualObject->colour(Ogre::ColourValue(tempInnerColor.x, tempInnerColor.y, tempInnerColor.z, 1.0f));
				this->manualObject->normal(0.0f, 0.0f, -1.0f);
				this->manualObject->textureCoord(0, 0);

				// 2) rb
				this->manualObject->position(p + (o * (so * (sp + ( Ogre::Vector3(w, 0.0f, 0.0f))))));
				this->manualObject->colour(Ogre::ColourValue(tempInnerColor.x, tempInnerColor.y, tempInnerColor.z, 1.0f));
				this->manualObject->normal(0.0f, 0.0f, -1.0f);
				this->manualObject->textureCoord(0, 1);

				// 3) lb
				this->manualObject->position(p + (o * (so * (sp + ( Ogre::Vector3(v , 0.0f, 0.0f))))));
				this->manualObject->colour(Ogre::ColourValue(tempInnerColor.x, tempInnerColor.y, tempInnerColor.z, 1.0f));
				this->manualObject->normal(0.0f, 0.0f, -1.0f);
				this->manualObject->textureCoord(1, 1);

				// 4) lt
				this->manualObject->position(p + (o * (so * (sp + ( Ogre::Vector3(v , h, 0.0f))))));
				this->manualObject->colour(Ogre::ColourValue(tempInnerColor.x, tempInnerColor.y, tempInnerColor.z, 1.0f));
				this->manualObject->normal(0.0f, 0.0f, -1.0f);
				this->manualObject->textureCoord(1, 0);

				this->manualObject->quad(this->indices + 0, this->indices + 1, this->indices + 2, this->indices + 3);
				this->indices += 4;
			}
		}
	}

	void ValueBarComponent::createValueBar(void)
	{
		if (nullptr == this->manualObject)
		{
			if (nullptr == this->lineNode)
			{
				this->lineNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode();
			}
			this->manualObject = this->gameObjectPtr->getSceneManager()->createManualObject();
			this->manualObject->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_MESH);
			this->manualObject->setName("ValueBar_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()) + "_" + Ogre::StringConverter::toString(index));
			this->manualObject->setQueryFlags(0 << 0);
			this->lineNode->attachObject(this->manualObject);
			this->manualObject->setCastShadows(false);
		}
	}

	void ValueBarComponent::destroyValueBar(void)
	{
		if (this->lineNode != nullptr)
		{
			this->lineNode->detachAllObjects();
			this->gameObjectPtr->getSceneManager()->destroyManualObject(this->manualObject);
			this->manualObject = nullptr;
			this->lineNode->getParentSceneNode()->removeAndDestroyChild(this->lineNode);
			this->lineNode = nullptr;
		}
	}

}; // namespace end
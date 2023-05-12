#include "NOWAPrecompiled.h"
#include "RectangleComponent.h"
#include "GameObjectController.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "LuaScriptComponent.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;
	
	RectangleComponent::RectangleComponent()
		: GameObjectComponent(),
		lineNode(nullptr),
		dummyEntity(nullptr),
		manualObject(nullptr),
		indices(0),
		castShadows(new Variant(RectangleComponent::AttrCastShadows(), false, this->attributes)),
		twoSided(new Variant(RectangleComponent::AttrTwoSided(), true, this->attributes)),
		rectanglesCount(new Variant(RectangleComponent::AttrRectanglesCount(), static_cast<unsigned int>(0), this->attributes))
	{
		this->rectanglesCount->addUserData(GameObject::AttrActionNeedRefresh());
	}

	RectangleComponent::~RectangleComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[RectangleComponent] Destructor rectangle component for game object: " + this->gameObjectPtr->getName());
		
		this->destroyRectangles();
		this->dummyEntity = nullptr;
	}

	bool RectangleComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CastShadows")
		{
			this->castShadows->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TwoSided")
		{
			this->twoSided->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RectanglesCount")
		{
			this->rectanglesCount->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		// Only create new variant, if fresh loading. If snapshot is done, no new variant
		// must be created! Because the algorithm is working changed flag of each existing variant!
		if (this->colors1.size() < this->rectanglesCount->getUInt())
		{
			this->colors1.resize(this->rectanglesCount->getUInt());
			this->colors2.resize(this->rectanglesCount->getUInt());
			this->startPositions.resize(this->rectanglesCount->getUInt());
			this->startOrientations.resize(this->rectanglesCount->getUInt());
			this->widths.resize(this->rectanglesCount->getUInt());
			this->heights.resize(this->rectanglesCount->getUInt());
		}

		for (unsigned int i = 0; i < this->rectanglesCount->getUInt(); i++)
		{
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "color1" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->colors1[i])
				{
					this->colors1[i] = new Variant(RectangleComponent::AttrColor1() + Ogre::StringConverter::toString(i),
						XMLConverter::getAttribVector3(propertyElement, "data"), this->attributes);
				}
				else
				{
					this->colors1[i]->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
				this->colors1[i]->addUserData(GameObject::AttrActionColorDialog());
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "color2" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->colors2[i])
				{
					this->colors2[i] = new Variant(RectangleComponent::AttrColor2() + Ogre::StringConverter::toString(i),
						XMLConverter::getAttribVector3(propertyElement, "data"), this->attributes);
				}
				else
				{
					this->colors2[i]->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
				this->colors2[i]->addUserData(GameObject::AttrActionColorDialog());
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "StartPosition" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->startPositions[i])
				{
					this->startPositions[i] = new Variant(RectangleComponent::AttrStartPosition() + Ogre::StringConverter::toString(i),
						XMLConverter::getAttribVector3(propertyElement, "data"), this->attributes);
				}
				else
				{
					this->startPositions[i]->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "StartOrientation" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->startOrientations[i])
				{
					this->startOrientations[i] = new Variant(RectangleComponent::AttrStartOrientation() + Ogre::StringConverter::toString(i),
						XMLConverter::getAttribVector3(propertyElement, "data"), this->attributes);
				}
				else
				{
					this->startOrientations[i]->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
				this->startOrientations[i]->setDescription("Orientation is set in the form: (degreeX, degreeY, degreeZ)");
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Width" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->widths[i])
				{
					this->widths[i] = new Variant(RectangleComponent::AttrWidth() + Ogre::StringConverter::toString(i),
						XMLConverter::getAttribReal(propertyElement, "data"), this->attributes);
				}
				else
				{
					this->widths[i]->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Height" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->heights[i])
				{
					this->heights[i] = new Variant(RectangleComponent::AttrHeight() + Ogre::StringConverter::toString(i),
						XMLConverter::getAttribReal(propertyElement, "data"), this->attributes);
				}
				else
				{
					this->heights[i]->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
				this->heights[i]->addUserData(GameObject::AttrActionSeparator());
			}
		}
		return true;
	}

	GameObjectCompPtr RectangleComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		RectangleCompPtr clonedCompPtr(boost::make_shared<RectangleComponent>());

		
		clonedCompPtr->setCastShadows(this->castShadows->getBool());
		clonedCompPtr->setTwoSided(this->twoSided->getBool());
		clonedCompPtr->setRectanglesCount(this->rectanglesCount->getUInt());

		for (unsigned int i = 0; i < this->rectanglesCount->getUInt(); i++)
		{
			clonedCompPtr->setColor1(i, this->colors1[i]->getVector3());
			clonedCompPtr->setColor2(i, this->colors2[i]->getVector3());
			clonedCompPtr->setStartPosition(i, this->startPositions[i]->getVector3());
			clonedCompPtr->setStartOrientation(i, this->startOrientations[i]->getVector3());
			clonedCompPtr->setWidth(i, this->widths[i]->getReal());
			clonedCompPtr->setHeight(i, this->heights[i]->getReal());
		}
		
		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool RectangleComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[RectangleComponent] Init rectangles component for game object: " + this->gameObjectPtr->getName());

		// Borrow the entity from the game object
		this->dummyEntity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();

		return true;
	}

	bool RectangleComponent::connect(void)
	{
		bool success = GameObjectComponent::connect();
		if (false == success)
		{
			return success;
		}

		this->dummyEntity->setVisible(false);

		if (nullptr == this->lineNode)
		{
			this->lineNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode();
		}
		this->manualObject = this->gameObjectPtr->getSceneManager()->createManualObject();
		this->manualObject->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_MESH);
		this->manualObject->setName("Rectangle_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()) + "_" + Ogre::StringConverter::toString(index));
		this->manualObject->setQueryFlags(0 << 0);
		this->lineNode->attachObject(this->manualObject);
		this->manualObject->setCastShadows(this->castShadows->getBool());
		
		return success;
	}

	bool RectangleComponent::disconnect(void)
	{
		this->dummyEntity->setVisible(true);

		this->destroyRectangles();
		
		return true;
	}

	void RectangleComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
			if (nullptr == this->manualObject || 0 == this->rectanglesCount->getUInt())
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

			for (unsigned int i = 0; i < this->rectanglesCount->getUInt(); i++)
			{
				this->drawRectangle(i);
			}

			// Realllllllyyyyy important! Else the rectangle is a whole mess!
			this->manualObject->index(0);

			this->manualObject->end();
		}
	}

	void RectangleComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (RectangleComponent::AttrCastShadows() == attribute->getName())
		{
			this->setCastShadows(attribute->getBool());
		}
		else if (RectangleComponent::AttrTwoSided() == attribute->getName())
		{
			this->setTwoSided(attribute->getBool());
		}
		else if (RectangleComponent::AttrRectanglesCount() == attribute->getName())
		{
			this->setRectanglesCount(attribute->getUInt());
		}
		else
		{
			for (unsigned int i = 0; i < this->rectanglesCount->getUInt(); i++)
			{
				if (RectangleComponent::AttrColor1() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setColor1(i, attribute->getVector3());
				}
				else if (RectangleComponent::AttrColor2() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setColor2(i, attribute->getVector3());
				}
				else if (RectangleComponent::AttrStartPosition() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setStartPosition(i, attribute->getVector3());
				}
				else if (RectangleComponent::AttrStartOrientation() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setStartOrientation(i, attribute->getVector3());
				}
				else if (RectangleComponent::AttrWidth() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setWidth(i, attribute->getReal());
				}
				else if (RectangleComponent::AttrHeight() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setHeight(i, attribute->getReal());
				}
			}
		}
	}

	void RectangleComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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
		propertyXML->append_attribute(doc.allocate_attribute("name", "CastShadows"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->castShadows->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TwoSided"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->twoSided->getBool())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RectanglesCount"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->rectanglesCount->getUInt())));
		propertiesXML->append_node(propertyXML);

		for (size_t i = 0; i < this->rectanglesCount->getUInt(); i++)
		{
			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "color1" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->colors1[i]->getVector3())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "color2" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->colors2[i]->getVector3())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "StartPosition" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->startPositions[i]->getVector3())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "StartOrientation" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->startOrientations[i]->getVector3())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "Width" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->widths[i]->getReal())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "Height" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->heights[i]->getReal())));
			propertiesXML->append_node(propertyXML);
		}
	}

	void RectangleComponent::setCastShadows(bool castShadows)
	{
		this->castShadows->setValue(castShadows);
	}

	bool RectangleComponent::getCastShadows(void) const
	{
		return this->castShadows->getBool();
	}

	void RectangleComponent::setTwoSided(bool twoSided)
	{
		this->twoSided->setValue(twoSided);
	}

	bool RectangleComponent::getTwoSided(void) const
	{
		return this->twoSided->getBool();
	}

	void RectangleComponent::setRectanglesCount(unsigned int rectanglesCount)
	{
		this->rectanglesCount->setValue(rectanglesCount);
		unsigned int oldSize = static_cast<unsigned int>(this->colors1.size());

		if (rectanglesCount > oldSize)
		{
			// Resize the waypoints array for count
			this->colors1.resize(rectanglesCount);
			this->colors2.resize(rectanglesCount);
			this->startPositions.resize(rectanglesCount);
			this->startOrientations.resize(rectanglesCount);
			this->widths.resize(rectanglesCount);
			this->heights.resize(rectanglesCount);

			for (unsigned int i = oldSize; i < this->rectanglesCount->getUInt(); i++)
			{
				this->colors1[i] = new Variant(RectangleComponent::AttrColor1() + Ogre::StringConverter::toString(i), Ogre::Vector3(1.0f, 0.0f, 0.0f), this->attributes);
				this->colors2[i] = new Variant(RectangleComponent::AttrColor2() + Ogre::StringConverter::toString(i), Ogre::Vector3(1.0f, 0.0f, 0.0f), this->attributes);
				this->startPositions[i] = new Variant(RectangleComponent::AttrStartPosition() + Ogre::StringConverter::toString(i), Ogre::Vector3(0.5f * i, 0.0f, 0.0f), this->attributes);
				this->startOrientations[i] = new Variant(RectangleComponent::AttrStartOrientation() + Ogre::StringConverter::toString(i), Ogre::Vector3::ZERO, this->attributes);
				this->widths[i] = new Variant(RectangleComponent::AttrWidth() + Ogre::StringConverter::toString(i), 1.0f, this->attributes);
				this->heights[i] = new Variant(RectangleComponent::AttrHeight() + Ogre::StringConverter::toString(i), 1.0f, this->attributes);
				this->heights[i]->addUserData(GameObject::AttrActionSeparator());
				this->colors1[i]->addUserData(GameObject::AttrActionColorDialog());
				this->colors2[i]->addUserData(GameObject::AttrActionColorDialog());
				this->startOrientations[i]->setDescription("Orientation is set in the form: (degreeX, degreeY, degreeZ)");

				this->setColor1(i, this->colors1[i]->getVector3());
				this->setColor2(i, this->colors2[i]->getVector3());
				this->setStartPosition(i, this->startPositions[i]->getVector3());
				this->setStartOrientation(i, this->startOrientations[i]->getVector3());
				this->setWidth(i, this->widths[i]->getReal());
				this->setHeight(i, this->heights[i]->getReal());
			}
		}
		else if (rectanglesCount < oldSize)
		{
			this->eraseVariants(this->colors1, rectanglesCount);
			this->eraseVariants(this->colors2, rectanglesCount);
			this->eraseVariants(this->startPositions, rectanglesCount);
			this->eraseVariants(this->startOrientations, rectanglesCount);
			this->eraseVariants(this->widths, rectanglesCount);
			this->eraseVariants(this->heights, rectanglesCount);
		}
	}

	unsigned int RectangleComponent::getRectanglesCount(void) const
	{
		return this->rectanglesCount->getUInt();
	}

	void RectangleComponent::setColor1(unsigned int index, const Ogre::Vector3& color)
	{
		if (index > this->colors1.size())
			index = static_cast<unsigned int>(this->colors1.size()) - 1;

		this->colors1[index]->setValue(color);
	}

	Ogre::Vector3 RectangleComponent::getColor1(unsigned int index) const
	{
		if (index > this->colors1.size())
			return Ogre::Vector3::ZERO;
		return this->colors1[index]->getVector3();
	}

	void RectangleComponent::setColor2(unsigned int index, const Ogre::Vector3& color)
	{
		if (index > this->colors2.size())
			index = static_cast<unsigned int>(this->colors2.size()) - 1;

		this->colors2[index]->setValue(color);
	}

	Ogre::Vector3 RectangleComponent::getColor2(unsigned int index) const
	{
		if (index > this->colors2.size())
			return Ogre::Vector3::ZERO;
		return this->colors2[index]->getVector3();
	}

	void RectangleComponent::setStartPosition(unsigned int index, const Ogre::Vector3& startPosition)
	{
		if (index > this->startPositions.size())
			index = static_cast<unsigned int>(this->startPositions.size()) - 1;

		this->startPositions[index]->setValue(startPosition);
	}

	Ogre::Vector3 RectangleComponent::getStartPosition(unsigned int index) const
	{
		if (index > this->startPositions.size())
			return Ogre::Vector3::ZERO;
		return this->startPositions[index]->getVector3();
	}

	void RectangleComponent::setStartOrientation(unsigned int index, const Ogre::Vector3& startOrientation)
	{
		if (index > this->startOrientations.size())
			index = static_cast<unsigned int>(this->startOrientations.size()) - 1;

		this->startOrientations[index]->setValue(startOrientation);
	}

	Ogre::Vector3 RectangleComponent::getStartOrientation(unsigned int index) const
	{
		if (index > this->startOrientations.size())
			return Ogre::Vector3::ZERO;
		return this->startOrientations[index]->getVector3();
	}

	void RectangleComponent::setWidth(unsigned int index, Ogre::Real width)
	{
		if (index > this->widths.size())
			index = static_cast<unsigned int>(this->widths.size()) - 1;

		Ogre::Real tempWidth = width;

		if (tempWidth < 0.0f)
		{
			tempWidth = 1.0f;
		}
		this->widths[index]->setValue(tempWidth);
	}

	Ogre::Real RectangleComponent::getWidth(unsigned int index) const
	{
		if (index > this->widths.size())
			return 0.0f;
		return this->widths[index]->getReal();
	}

	void RectangleComponent::setHeight(unsigned int index, Ogre::Real height)
	{
		if (index > this->heights.size())
			index = static_cast<unsigned int>(this->heights.size()) - 1;

		Ogre::Real tempHeight = height;

		if (tempHeight < 0.0f)
		{
			tempHeight = 1.0f;
		}

		this->heights[index]->setValue(tempHeight);
	}

	Ogre::Real RectangleComponent::getHeight(unsigned int index) const
	{
		if (index > this->heights.size())
			return 0.0f;
		return this->heights[index]->getReal();
	}

	void RectangleComponent::drawRectangle(unsigned int index)
	{
		Ogre::Vector3 color1 = this->colors1[index]->getVector3();
		Ogre::Vector3 color2 = this->colors2[index]->getVector3();

		//	//    lt-------rt 
		//	//    |         |
		//	//    |         | height
		//	//    |         |
		//	//    lb---x---rb
		//           width
		// x = start position

		Ogre::Vector3 p = this->gameObjectPtr->getPosition();
		Ogre::Quaternion o = this->gameObjectPtr->getOrientation();
		Ogre::Vector3 sp = this->startPositions[index]->getVector3();
		Ogre::Quaternion so = MathHelper::getInstance()->degreesToQuat(this->startOrientations[index]->getVector3());
		Ogre::Real w = this->widths[index]->getReal();
		Ogre::Real h = this->heights[index]->getReal();
		
		// Upper half for color gradient
		// 1) lt
		this->manualObject->position(p + (o * (so * (sp + (so * Ogre::Vector3(-w * 0.5f, h, 0.0f))))));
		this->manualObject->colour(Ogre::ColourValue(color2.x, color2.y, color2.z, 1.0f));
		this->manualObject->normal(0.0f, 0.0f, 1.0f);
		this->manualObject->textureCoord(0, 0);

		// 2) lb
		this->manualObject->position(p + (o * (so * (sp + (so * Ogre::Vector3(-w * 0.5f, h * 0.5f, 0.0f))))));
		this->manualObject->colour(Ogre::ColourValue((color1.x + color2.x) * 0.5f, (color1.y + color2.y) * 0.5f, (color1.z + color2.z) * 0.5f, 1.0f));
		this->manualObject->normal(0.0f, 0.0f, 1.0f);
		this->manualObject->textureCoord(0, 1);

		// 3) rb
		this->manualObject->position(p + (o * (so * (sp + (so * Ogre::Vector3(w * 0.5f, h * 0.5f, 0.0f))))));
		this->manualObject->colour(Ogre::ColourValue((color1.x + color2.x) * 0.5f, (color1.y + color2.y) * 0.5f, (color1.z + color2.z) * 0.5f, 1.0f));
		this->manualObject->normal(0.0f, 0.0f, 1.0f);
		this->manualObject->textureCoord(1, 1);

		// 4) rt
		this->manualObject->position(p + (o * (so * (sp + (so * Ogre::Vector3(w * 0.5f, h, 0.0f))))));
		this->manualObject->colour(Ogre::ColourValue(color2.x, color2.y, color2.z, 1.0f));
		this->manualObject->normal(0.0f, 0.0f, 1.0f);
		this->manualObject->textureCoord(1, 0);

		// Internally constructs 2 triangles
		this->manualObject->quad(this->indices + 0, this->indices + 1, this->indices + 2, this->indices + 3);
		this->indices += 4;

		// Lower half for color gradient
		// 1) lt
		this->manualObject->position(p + (o * (so * (sp + (so * Ogre::Vector3(-w * 0.5f, h * 0.5f, 0.0f))))));
		this->manualObject->colour(Ogre::ColourValue((color1.x + color2.x) * 0.5f, (color1.y + color2.y) * 0.5f, (color1.z + color2.z) * 0.5f, 1.0f));
		this->manualObject->normal(0.0f, 0.0f, 1.0f);
		this->manualObject->textureCoord(0, 0);

		// 2) lb
		this->manualObject->position(p + (o * (so * (sp + (so * Ogre::Vector3(-w * 0.5f, 0.0f, 0.0f))))));
		this->manualObject->colour(Ogre::ColourValue(color1.x, color1.y, color1.z, 1.0f));
		this->manualObject->normal(0.0f, 0.0f, 1.0f);
		this->manualObject->textureCoord(0, 1);

		// 3) rb
		this->manualObject->position(p + (o * (so * (sp + (so * Ogre::Vector3(w * 0.5f, 0.0f, 0.0f))))));
		this->manualObject->colour(Ogre::ColourValue(color1.x, color1.y, color1.z, 1.0f));
		this->manualObject->normal(0.0f, 0.0f, 1.0f);
		this->manualObject->textureCoord(1, 1);

		// 4) rt
		this->manualObject->position(p + (o * (so * (sp + (so * Ogre::Vector3(w * 0.5f, h * 0.5f, 0.0f))))));
		this->manualObject->colour(Ogre::ColourValue((color1.x + color2.x) * 0.5f, (color1.y + color2.y) * 0.5f, (color1.z + color2.z) * 0.5f, 1.0f));
		this->manualObject->normal(0.0f, 0.0f, 1.0f);
		this->manualObject->textureCoord(1, 0);

		this->manualObject->quad(this->indices + 0, this->indices + 1, this->indices + 2, this->indices + 3);
		this->indices += 4;

		if (true == this->twoSided->getBool())
		{
			// Upper half for color gradient
			// 1) rt
			this->manualObject->position(p + (o * (so * (sp + (so * Ogre::Vector3(w * 0.5f, h, 0.0f))))));
			this->manualObject->colour(Ogre::ColourValue(color2.x, color2.y, color2.z, 1.0f));
			this->manualObject->normal(0.0f, 0.0f, -1.0f);
			this->manualObject->textureCoord(0, 0);

			// 2) rb
			this->manualObject->position(p + (o * (so * (sp + (so * Ogre::Vector3(w * 0.5f, h * 0.5f, 0.0f))))));
			this->manualObject->colour(Ogre::ColourValue((color1.x + color2.x) * 0.5f, (color1.y + color2.y) * 0.5f, (color1.z + color2.z) * 0.5f, 1.0f));
			this->manualObject->normal(0.0f, 0.0f, -1.0f);
			this->manualObject->textureCoord(0, 1);

			// 3) lb
			this->manualObject->position(p + (o * (so * (sp + (so * Ogre::Vector3(-w * 0.5f, h * 0.5f, 0.0f))))));
			this->manualObject->colour(Ogre::ColourValue((color1.x + color2.x) * 0.5f, (color1.y + color2.y) * 0.5f, (color1.z + color2.z) * 0.5f, 1.0f));
			this->manualObject->normal(0.0f, 0.0f, -1.0f);
			this->manualObject->textureCoord(1, 1);

			// 4) lt
			this->manualObject->position(p + (o * (so * (sp + (so * Ogre::Vector3(-w * 0.5f, h, 0.0f))))));
			this->manualObject->colour(Ogre::ColourValue(color2.x, color2.y, color2.z, 1.0f));
			this->manualObject->normal(0.0f, 0.0f, -1.0f);
			this->manualObject->textureCoord(1, 0);

			this->manualObject->quad(this->indices + 0, this->indices + 1, this->indices + 2, this->indices + 3);
			this->indices += 4;

			// Lower half for color gradient
			// 1) rt
			this->manualObject->position(p + (o * (so * (sp + (so * Ogre::Vector3(w * 0.5f, h * 0.5f, 0.0f))))));
			this->manualObject->colour(Ogre::ColourValue((color1.x + color2.x) * 0.5f, (color1.y + color2.y) * 0.5f, (color1.z + color2.z) * 0.5f, 1.0f));
			this->manualObject->normal(0.0f, 0.0f, -1.0f);
			this->manualObject->textureCoord(0, 0);

			// 2) rb
			this->manualObject->position(p + (o * (so * (sp + (so * Ogre::Vector3(w * 0.5f, 0.0f, 0.0f))))));
			this->manualObject->colour(Ogre::ColourValue(color1.x, color1.y, color1.z, 1.0f));
			this->manualObject->normal(0.0f, 0.0f, -1.0f);
			this->manualObject->textureCoord(0, 1);

			// 3) lb
			this->manualObject->position(p + (o * (so * (sp + (so * Ogre::Vector3(-w * 0.5f, 0.0f, 0.0f))))));
			this->manualObject->colour(Ogre::ColourValue(color1.x, color1.y, color1.z, 1.0f));
			this->manualObject->normal(0.0f, 0.0f, -1.0f);
			this->manualObject->textureCoord(1, 1);

			// 4) lt
			this->manualObject->position(p + (o * (so * (sp + (so * Ogre::Vector3(-w * 0.5f, h * 0.5f, 0.0f))))));
			this->manualObject->colour(Ogre::ColourValue((color1.x + color2.x) * 0.5f, (color1.y + color2.y) * 0.5f, (color1.z + color2.z) * 0.5f, 1.0f));
			this->manualObject->normal(0.0f, 0.0f, -1.0f);
			this->manualObject->textureCoord(1, 0);

			this->manualObject->quad(this->indices + 0, this->indices + 1, this->indices + 2, this->indices + 3);
			this->indices += 4;
		}
	}

	void RectangleComponent::destroyRectangles(void)
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
	
	Ogre::String RectangleComponent::getClassName(void) const
	{
		return "RectangleComponent";
	}

	Ogre::String RectangleComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

}; // namespace end
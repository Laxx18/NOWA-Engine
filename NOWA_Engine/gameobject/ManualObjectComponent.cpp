#include "NOWAPrecompiled.h"
#include "ManualObjectComponent.h"
#include "GameObjectController.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "LuaScriptComponent.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;
	
	ManualObjectComponent::ManualObjectComponent()
		: GameObjectComponent(),
		lineNode(nullptr),
		dummyEntity(nullptr),
		manualObject(nullptr),
		indices(0),
		newStartPosition(Ogre::Vector3(Ogre::Math::POS_INFINITY, Ogre::Math::POS_INFINITY, Ogre::Math::POS_INFINITY)),
		internalOperationType(Ogre::OperationType::OT_LINE_LIST),
		connected(new Variant(ManualObjectComponent::AttrConnected(), false, this->attributes)),
		operationType(new Variant(ManualObjectComponent::AttrOperationType(), { "OT_POINT_LIST", "OT_LINE_LIST", "OT_LINE_STRIP"
			, "OT_TRIANGLE_LIST", "OT_TRIANGLE_STRIP" }, this->attributes)),
		castShadows(new Variant(ManualObjectComponent::AttrCastShadows(), false, this->attributes)),
		linesCount(new Variant(ManualObjectComponent::AttrLinesCount(), static_cast<unsigned int>(0), this->attributes))
	{
		this->linesCount->addUserData(GameObject::AttrActionNeedRefresh());
		this->operationType->setListSelectedValue("OT_LINE_LIST");
	}

	ManualObjectComponent::~ManualObjectComponent()
	{
		
	}

	bool ManualObjectComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Connected")
		{
			this->connected->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OperationType")
		{
			this->operationType->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CastShadows")
		{
			this->castShadows->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "LinesCount")
		{
			this->linesCount->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		if (this->colors.size() < this->linesCount->getUInt())
		{
			this->colors.resize(this->linesCount->getUInt());
			this->startPositions.resize(this->linesCount->getUInt());
			this->endPositions.resize(this->linesCount->getUInt());
		}

		for (unsigned int i = 0; i < this->linesCount->getUInt(); i++)
		{
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Color" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->colors[i])
				{
					this->colors[i] = new Variant(ManualObjectComponent::AttrColor() + Ogre::StringConverter::toString(i),
						XMLConverter::getAttribVector3(propertyElement, "data"), this->attributes);
				}
				else
				{
					this->colors[i]->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "StartPosition" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->startPositions[i])
				{
					this->startPositions[i] = new Variant(ManualObjectComponent::AttrStartPosition() + Ogre::StringConverter::toString(i),
						XMLConverter::getAttribVector3(propertyElement, "data"), this->attributes);
				}
				else
				{
					this->startPositions[i]->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
			}

			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "EndPosition" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->endPositions[i])
				{
					this->endPositions[i] = new Variant(ManualObjectComponent::AttrEndPosition() + Ogre::StringConverter::toString(i),
						XMLConverter::getAttribVector3(propertyElement, "data"), this->attributes);
				}
				else
				{
					this->endPositions[i]->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
				this->endPositions[i]->addUserData(GameObject::AttrActionSeparator());
			}
		}
		return true;
	}

	GameObjectCompPtr ManualObjectComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		ManualObjectCompPtr clonedCompPtr(boost::make_shared<ManualObjectComponent>());

		
		clonedCompPtr->setConnected(this->connected->getBool());
		clonedCompPtr->setOperationType(this->operationType->getListSelectedValue());
		clonedCompPtr->setCastShadows(this->castShadows->getBool());
		clonedCompPtr->setLinesCount(this->linesCount->getUInt());
		
		for (unsigned int i = 0; i < this->linesCount->getUInt(); i++)
		{
			clonedCompPtr->setColor(i, this->colors[i]->getVector3());
			clonedCompPtr->setStartPosition(i, this->startPositions[i]->getVector3());
			clonedCompPtr->setEndPosition(i, this->endPositions[i]->getVector3());
		}
		
		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool ManualObjectComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ManualObjectComponent] Init manual object component for game object: " + this->gameObjectPtr->getName());

		// Borrow the entity from the game object
		this->dummyEntity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();

		return true;
	}

	void ManualObjectComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ManualObjectComponent] Destructor manual object component for game object: " + this->gameObjectPtr->getName());

		ENQUEUE_RENDER_COMMAND_WAIT("ManualObjectComponent::~ManualObjectComponent",
		{
			this->destroyLines();
			this->dummyEntity = nullptr;
		});
	}

	bool ManualObjectComponent::connect(void)
	{
		bool success = GameObjectComponent::connect();
		if (false == success)
		{
			return success;
		}

		ENQUEUE_RENDER_COMMAND_WAIT("ManualObjectComponent::connect",
		{
			this->dummyEntity->setVisible(false);

			if (nullptr == this->lineNode)
			{
				this->lineNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode();
			}
			this->manualObject = this->gameObjectPtr->getSceneManager()->createManualObject();
			this->manualObject->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_MESH);
			this->manualObject->setName("ManualObject_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()) + "_" + Ogre::StringConverter::toString(index));
			this->manualObject->setQueryFlags(0 << 0);
			this->lineNode->attachObject(this->manualObject);
			this->manualObject->setCastShadows(this->castShadows->getBool());
		});
		
		return success;
	}

	bool ManualObjectComponent::disconnect(void)
	{
		Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
		NOWA::GraphicsModule::getInstance()->removeTrackedClosure(id);

		ENQUEUE_RENDER_COMMAND_WAIT("ManualObjectComponent::disconnect",
		{
			this->dummyEntity->setVisible(true);
			this->destroyLines();
		});
		
		return true;
	}

	// Laser shoot:
	// https://forums.ogre3d.org/viewtopic.php?f=25&t=96299

	void ManualObjectComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
			if (nullptr == this->manualObject || 0 == this->linesCount->getUInt())
			{
				return;
			}

			auto closureFunction = [this](Ogre::Real renderDt)
			{
				this->indices = 0;
				if (this->manualObject->getNumSections() > 0)
				{
					this->manualObject->beginUpdate(0);
				}
				else
				{
					this->manualObject->clear();
					this->manualObject->begin("WhiteNoLightingBackground", this->internalOperationType);
				}

				for (unsigned int i = 0; i < this->linesCount->getUInt(); i++)
				{
					this->drawLine(i);
				}

				this->manualObject->end();
			};
			Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
			NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction, false);
		}
	}

	void ManualObjectComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (ManualObjectComponent::AttrConnected() == attribute->getName())
		{
			this->setConnected(attribute->getBool());
		}
		else if (ManualObjectComponent::AttrOperationType() == attribute->getName())
		{
			this->setOperationType(attribute->getListSelectedValue());
		}
		else if (ManualObjectComponent::AttrCastShadows() == attribute->getName())
		{
			this->setCastShadows(attribute->getBool());
		}
		else if (ManualObjectComponent::AttrLinesCount() == attribute->getName())
		{
			this->setLinesCount(attribute->getUInt());
		}
		else
		{
			for (unsigned int i = 0; i < this->linesCount->getUInt(); i++)
			{
				if (ManualObjectComponent::AttrColor() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setColor(i, attribute->getVector3());
				}
				else if (ManualObjectComponent::AttrStartPosition() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setStartPosition(i, attribute->getVector3());
				}
				else if (ManualObjectComponent::AttrEndPosition() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setEndPosition(i, attribute->getVector3());
				}
			}
		}
	}

	void ManualObjectComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("name", "Connected"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->connected->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OperationType"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->operationType->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "CastShadows"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->castShadows->getBool())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "LinesCount"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->linesCount->getUInt())));
		propertiesXML->append_node(propertyXML);

		for (size_t i = 0; i < this->linesCount->getUInt(); i++)
		{
			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "Color" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->colors[i]->getVector3())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "StartPosition" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->startPositions[i]->getVector3())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "EndPosition" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->endPositions[i]->getVector3())));
			propertiesXML->append_node(propertyXML);
		}
	}

	void ManualObjectComponent::setConnected(bool connected)
	{
		this->connected->setValue(connected);
	}

	bool ManualObjectComponent::getIsConnected(void) const
	{
		return this->connected->getBool();
	}

	void ManualObjectComponent::setOperationType(const Ogre::String& operationType)
	{
		this->operationType->setListSelectedValue(operationType);
		this->internalOperationType = this->mapStringToOperationType(operationType);
	}

	Ogre::String ManualObjectComponent::getOperationType(void) const
	{
		return this->operationType->getListSelectedValue();
	}

	void ManualObjectComponent::setCastShadows(bool castShadows)
	{
		this->castShadows->setValue(castShadows);
	}

	bool ManualObjectComponent::getCastShadows(void) const
	{
		return this->castShadows->getBool();
	}

	void ManualObjectComponent::setLinesCount(unsigned int linesCount)
	{
		this->linesCount->setValue(linesCount);
		unsigned int oldSize = static_cast<unsigned int>(this->colors.size());

		if (linesCount > oldSize)
		{
			// Resize the waypoints array for count
			this->colors.resize(linesCount);
			this->startPositions.resize(linesCount);
			this->endPositions.resize(linesCount);

			for (unsigned int i = oldSize; i < this->linesCount->getUInt(); i++)
			{
				this->colors[i] = new Variant(ManualObjectComponent::AttrColor() + Ogre::StringConverter::toString(i), Ogre::Vector3(1.0f, 0.0f, 0.0f), this->attributes);
				this->startPositions[i] = new Variant(ManualObjectComponent::AttrStartPosition() + Ogre::StringConverter::toString(i), Ogre::Vector3(0.5f * i, 0.0f, 0.0f), this->attributes);
				this->endPositions[i] = new Variant(ManualObjectComponent::AttrEndPosition() + Ogre::StringConverter::toString(i), Ogre::Vector3(0.5f * i, 1.0f, 0.0f), this->attributes);
				this->endPositions[i]->addUserData(GameObject::AttrActionSeparator());

				this->setColor(i, this->colors[i]->getVector3());
				this->setStartPosition(i, this->startPositions[i]->getVector3());
				this->setEndPosition(i, this->endPositions[i]->getVector3());
			}
		}
		else if (linesCount < oldSize)
		{
			this->eraseVariants(this->colors, linesCount);
			this->eraseVariants(this->startPositions, linesCount);
			this->eraseVariants(this->endPositions, linesCount);
		}
	}

	unsigned int ManualObjectComponent::getLinesCount(void) const
	{
		return this->linesCount->getUInt();
	}

	void ManualObjectComponent::setColor(unsigned int index, const Ogre::Vector3& color)
	{
		if (index > this->colors.size())
			index = static_cast<unsigned int>(this->colors.size()) - 1;

		this->colors[index]->setValue(color);
	}

	Ogre::Vector3 ManualObjectComponent::getColor(unsigned int index) const
	{
		if (index > this->colors.size())
			return Ogre::Vector3::ZERO;
		return this->colors[index]->getVector3();
	}

	void ManualObjectComponent::setStartPosition(unsigned int index, const Ogre::Vector3& startPosition)
	{
		if (index > this->startPositions.size())
			index = static_cast<unsigned int>(this->startPositions.size()) - 1;

		this->startPositions[index]->setValue(startPosition);
	}

	Ogre::Vector3 ManualObjectComponent::getStartPosition(unsigned int index) const
	{
		if (index > this->startPositions.size())
			return Ogre::Vector3::ZERO;
		return this->startPositions[index]->getVector3();
	}

	void ManualObjectComponent::setEndPosition(unsigned int index, const Ogre::Vector3& endPosition)
	{
		if (index > this->endPositions.size())
			index = static_cast<unsigned int>(this->endPositions.size()) - 1;

		this->endPositions[index]->setValue(endPosition);
	}

	Ogre::Vector3 ManualObjectComponent::getEndPosition(unsigned int index) const
	{
		if (index > this->endPositions.size())
			return Ogre::Vector3::ZERO;
		return this->endPositions[index]->getVector3();
	}

	void ManualObjectComponent::drawLine(unsigned int index)
	{
		Ogre::Vector3 color = this->colors[index]->getVector3();

		if (Ogre::Vector3(Ogre::Math::POS_INFINITY, Ogre::Math::POS_INFINITY, Ogre::Math::POS_INFINITY) == this->newStartPosition || false == this->connected->getBool())
			this->manualObject->position(this->gameObjectPtr->getPosition() + (this->gameObjectPtr->getOrientation() * this->startPositions[index]->getVector3()));
		else
			this->manualObject->position(this->gameObjectPtr->getPosition() + (this->gameObjectPtr->getOrientation() * this->newStartPosition));

		this->manualObject->colour(Ogre::ColourValue(color.x, color.y, color.z, 1.0f));

		this->manualObject->index(this->indices);
		this->indices++;

		if (true == this->connected->getBool())
		{
			// Connected and last index, set to first index
			if (index == this->linesCount->getUInt() - 1)
			{
				this->manualObject->position(this->gameObjectPtr->getPosition() + (this->gameObjectPtr->getOrientation() * this->startPositions[0]->getVector3()));
			}
		}
		else
		{
			this->manualObject->position(this->gameObjectPtr->getPosition() + (this->gameObjectPtr->getOrientation() * this->endPositions[index]->getVector3()));
		}
		
		this->manualObject->colour(Ogre::ColourValue(color.x, color.y, color.z, 1.0f));

		if (true == this->connected->getBool())
			this->newStartPosition = this->endPositions[index]->getVector3();

		this->manualObject->index(this->indices);
		this->indices++;
	}

	void ManualObjectComponent::destroyLines(void)
	{
		this->newStartPosition = Ogre::Vector3(Ogre::Math::POS_INFINITY, Ogre::Math::POS_INFINITY, Ogre::Math::POS_INFINITY);
		if (this->lineNode != nullptr)
		{
			this->lineNode->detachAllObjects();
			this->gameObjectPtr->getSceneManager()->destroyManualObject(this->manualObject);
			this->manualObject = nullptr;
			this->lineNode->getParentSceneNode()->removeAndDestroyChild(this->lineNode);
			this->lineNode = nullptr;
		}
	}

	Ogre::String ManualObjectComponent::mapOperationTypeToString(Ogre::OperationType operationType)
	{
		Ogre::String strOperationType = "OT_POINT_LIST";
		if (Ogre::OperationType::OT_LINE_LIST == operationType)
			strOperationType = "OT_LINE_LIST";
		else if (Ogre::OperationType::OT_LINE_STRIP == operationType)
			strOperationType = "OT_LINE_STRIP";
		else if (Ogre::OperationType::OT_TRIANGLE_LIST == operationType)
			strOperationType = "OT_TRIANGLE_LIST";
		else if (Ogre::OperationType::OT_TRIANGLE_STRIP == operationType)
			strOperationType = "OT_TRIANGLE_STRIP";
		/*else if (Ogre::OperationType::OT_TRIANGLE_FAN == operationType)
			strOperationType = "OT_TRIANGLE_FAN";*/

		return strOperationType;
	}

	Ogre::OperationType ManualObjectComponent::mapStringToOperationType(const Ogre::String& strOperationType)
	{
		Ogre::OperationType operationType = Ogre::OperationType::OT_POINT_LIST;
		if ("OT_LINE_LIST" == strOperationType)
			operationType = Ogre::OT_LINE_LIST;
		else if ("OT_LINE_STRIP" == strOperationType)
			operationType = Ogre::OT_LINE_STRIP;
		else if ("OT_TRIANGLE_LIST" == strOperationType)
			operationType = Ogre::OT_TRIANGLE_LIST;
		else if ("OT_TRIANGLE_STRIP" == strOperationType)
			operationType = Ogre::OT_TRIANGLE_STRIP;
		/*else if ("OT_TRIANGLE_FAN" == strOperationType)
			operationType = Ogre::OT_TRIANGLE_FAN;*/

		return operationType;
	}
	
	Ogre::String ManualObjectComponent::getClassName(void) const
	{
		return "ManualObjectComponent";
	}

	Ogre::String ManualObjectComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

}; // namespace end
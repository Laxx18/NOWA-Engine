#include "NOWAPrecompiled.h"
#include "LinesComponent.h"
#include "GameObjectController.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;
	
	LinesComponent::LinesComponent()
		: GameObjectComponent(),
		lineNode(nullptr),
		dummyEntity(nullptr),
		internalOperationType(Ogre::OperationType::OT_LINE_LIST),
		connected(new Variant(LinesComponent::AttrConnected(), false, this->attributes)),
		operationType(new Variant(LinesComponent::AttrOperationType(), { "OT_POINT_LIST", "OT_LINE_LIST", "OT_LINE_STRIP"
			, "OT_TRIANGLE_LIST", "OT_TRIANGLE_STRIP" }, this->attributes)),
		castShadows(new Variant(LinesComponent::AttrCastShadows(), false, this->attributes)),
		linesCount(new Variant(LinesComponent::AttrLinesCount(), static_cast<unsigned int>(0), this->attributes))
	{
		this->linesCount->addUserData(GameObject::AttrActionNeedRefresh());
		this->operationType->setListSelectedValue("OT_LINE_LIST");
	}

	LinesComponent::~LinesComponent()
	{
		
	}

	bool LinesComponent::init(rapidxml::xml_node<>*& propertyElement)
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
			this->lineObjects.resize(this->linesCount->getUInt());
		}

		for (unsigned int i = 0; i < this->linesCount->getUInt(); i++)
		{
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Color" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->colors[i])
				{
					this->colors[i] = new Variant(LinesComponent::AttrColor() + Ogre::StringConverter::toString(i),
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
					this->startPositions[i] = new Variant(LinesComponent::AttrStartPosition() + Ogre::StringConverter::toString(i),
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
					this->endPositions[i] = new Variant(LinesComponent::AttrEndPosition() + Ogre::StringConverter::toString(i),
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

	GameObjectCompPtr LinesComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		LinesCompPtr clonedCompPtr(boost::make_shared<LinesComponent>());

		
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

	bool LinesComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[LinesComponent] Init lines component for game object: " + this->gameObjectPtr->getName());

		// Borrow the entity from the game object
		this->dummyEntity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();

		return true;
	}

	void LinesComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[LinesComponent] Destructor lines component for game object: " + this->gameObjectPtr->getName());

		ENQUEUE_RENDER_COMMAND_WAIT("LinesComponent::~LinesComponent",
			{
				this->destroyLines();
				this->dummyEntity = nullptr;
			});
	}

	bool LinesComponent::connect(void)
	{
		// TODO: Wait?
		ENQUEUE_RENDER_COMMAND_WAIT("LinesComponent::connect",
		{
			this->dummyEntity->setVisible(false);
			for (unsigned int i = 0; i < this->linesCount->getUInt(); i++)
			{
				this->createLine(i);
			}
		});
		
		return true;
	}

	bool LinesComponent::disconnect(void)
	{
		Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
		NOWA::GraphicsModule::getInstance()->removeTrackedClosure(id);

		// TODO: Wait?
		ENQUEUE_RENDER_COMMAND_WAIT("LinesComponent::connect",
		{
			this->dummyEntity->setVisible(true);

			this->destroyLines();
		});
		
		return true;
	}

	void LinesComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
			auto closureFunction = [this](Ogre::Real renderDt)
			{
				for (unsigned int i = 0; i < this->linesCount->getUInt(); i++)
				{
					this->drawLine(i);
				}
			};
			Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
			NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction, false);
		}
	}

	void LinesComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (LinesComponent::AttrConnected() == attribute->getName())
		{
			this->setConnected(attribute->getBool());
		}
		else if (LinesComponent::AttrOperationType() == attribute->getName())
		{
			this->setOperationType(attribute->getListSelectedValue());
		}
		else if (LinesComponent::AttrCastShadows() == attribute->getName())
		{
			this->setCastShadows(attribute->getBool());
		}
		else if (LinesComponent::AttrLinesCount() == attribute->getName())
		{
			this->setLinesCount(attribute->getUInt());
		}
		else
		{
			for (unsigned int i = 0; i < this->linesCount->getUInt(); i++)
			{
				if (LinesComponent::AttrColor() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setColor(i, attribute->getVector3());
				}
				else if (LinesComponent::AttrStartPosition() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setStartPosition(i, attribute->getVector3());
				}
				else if (LinesComponent::AttrEndPosition() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setEndPosition(i, attribute->getVector3());
				}
			}
		}
	}

	void LinesComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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

	void LinesComponent::setConnected(bool connected)
	{
		this->connected->setValue(connected);
	}

	bool LinesComponent::getIsConnected(void) const
	{
		return this->connected->getBool();
	}

	void LinesComponent::setOperationType(const Ogre::String& operationType)
	{
		this->operationType->setListSelectedValue(operationType);
		this->internalOperationType = this->mapStringToOperationType(operationType);
	}

	Ogre::String LinesComponent::getOperationType(void) const
	{
		return this->operationType->getListSelectedValue();
	}

	void LinesComponent::setCastShadows(bool castShadows)
	{
		this->castShadows->setValue(castShadows);
	}

	bool LinesComponent::getCastShadows(void) const
	{
		return this->castShadows->getBool();
	}

	void LinesComponent::setLinesCount(unsigned int linesCount)
	{
		this->linesCount->setValue(linesCount);
		unsigned int oldSize = static_cast<unsigned int>(this->colors.size());

		if (linesCount > oldSize)
		{
			// Resize the waypoints array for count
			this->colors.resize(linesCount);
			this->startPositions.resize(linesCount);
			this->endPositions.resize(linesCount);
			this->lineObjects.resize(linesCount);

			for (unsigned int i = oldSize; i < this->linesCount->getUInt(); i++)
			{
				this->colors[i] = new Variant(LinesComponent::AttrColor() + Ogre::StringConverter::toString(i), Ogre::Vector3(1.0f, 0.0f, 0.0f), this->attributes);
				this->startPositions[i] = new Variant(LinesComponent::AttrStartPosition() + Ogre::StringConverter::toString(i), Ogre::Vector3(0.5f * i, 0.0f, 0.0f), this->attributes);
				this->endPositions[i] = new Variant(LinesComponent::AttrEndPosition() + Ogre::StringConverter::toString(i), Ogre::Vector3(0.5f * i, 1.0f, 0.0f), this->attributes);
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
			this->destroyLine(linesCount);
		}
	}

	unsigned int LinesComponent::getLinesCount(void) const
	{
		return this->linesCount->getUInt();
	}

	void LinesComponent::setColor(unsigned int index, const Ogre::Vector3& color)
	{
		if (index > this->colors.size())
			index = static_cast<unsigned int>(this->colors.size()) - 1;

		this->colors[index]->setValue(color);
	}

	Ogre::Vector3 LinesComponent::getColor(unsigned int index) const
	{
		if (index > this->colors.size())
			return Ogre::Vector3::ZERO;
		return this->colors[index]->getVector3();
	}

	void LinesComponent::setStartPosition(unsigned int index, const Ogre::Vector3& startPosition)
	{
		if (index > this->startPositions.size())
			index = static_cast<unsigned int>(this->startPositions.size()) - 1;

		this->startPositions[index]->setValue(startPosition);
	}

	Ogre::Vector3 LinesComponent::getStartPosition(unsigned int index) const
	{
		if (index > this->startPositions.size())
			return Ogre::Vector3::ZERO;
		return this->startPositions[index]->getVector3();
	}

	void LinesComponent::setEndPosition(unsigned int index, const Ogre::Vector3& endPosition)
	{
		if (index > this->endPositions.size())
			index = static_cast<unsigned int>(this->endPositions.size()) - 1;

		this->endPositions[index]->setValue(endPosition);
	}

	Ogre::Vector3 LinesComponent::getEndPosition(unsigned int index) const
	{
		if (index > this->endPositions.size())
			return Ogre::Vector3::ZERO;
		return this->endPositions[index]->getVector3();
	}

	void LinesComponent::createLine(unsigned int index)
	{
		// Threadsafe from the outside
		if (nullptr == this->lineNode)
		{
			this->lineNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode();
		}
		// this->lineObject = new Ogre::v1::ManualObject(0, &this->sceneManager->_getEntityMemoryManager(Ogre::SCENE_DYNAMIC), this->sceneManager);
		this->lineObjects[index] = this->gameObjectPtr->getSceneManager()->createManualObject();
		this->lineObjects[index]->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_MESH);
		this->lineObjects[index]->setName("Lines_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()) + "_" + Ogre::StringConverter::toString(index));
		this->lineObjects[index]->setQueryFlags(0 << 0);
		this->lineNode->attachObject(this->lineObjects[index]);
		this->lineObjects[index]->setCastShadows(this->castShadows->getBool());
	}

	void LinesComponent::drawLine(unsigned int index)
	{
		// Threadsafe from the outside
		// Draw a 3D line between these points for visual effect
		this->lineObjects[index]->clear();
		// Or here via data block??
		this->lineObjects[index]->begin("WhiteNoLightingBackground", this->internalOperationType);
		Ogre::Vector3 color = this->colors[index]->getVector3();

		this->lineObjects[index]->position(this->gameObjectPtr->getPosition() + (this->gameObjectPtr->getOrientation() * this->startPositions[index]->getVector3()));
		this->lineObjects[index]->colour(Ogre::ColourValue(color.x, color.y, color.z, 1.0f));

		if (false == this->connected->getBool())
			this->lineObjects[index]->index(0);
		else
			this->lineObjects[index]->index(2 * index);

		this->lineObjects[index]->position(this->gameObjectPtr->getPosition() + (this->gameObjectPtr->getOrientation() * this->endPositions[index]->getVector3()));
		this->lineObjects[index]->colour(Ogre::ColourValue(color.x, color.y, color.z, 1.0f));

		if (false == this->connected->getBool())
			this->lineObjects[index]->index(1);
		else
			this->lineObjects[index]->index(2 * index + 1);

		this->lineObjects[index]->end();
	}

	void LinesComponent::destroyLine(unsigned int index)
	{
		// Threadsafe from the outside
		if (this->lineNode != nullptr)
		{
			if (this->lineObjects[index] != nullptr)
			{
				this->lineNode->detachObject(this->lineObjects[index]);
			
				this->gameObjectPtr->getSceneManager()->destroyManualObject(this->lineObjects[index]);
				this->lineObjects[index] = nullptr;
			}
			this->lineNode->getParentSceneNode()->removeAndDestroyChild(this->lineNode);
			this->lineNode = nullptr;
		}
	}

	void LinesComponent::destroyLines(void)
	{
		// Threadsafe from the outside
		if (this->lineNode != nullptr)
		{
			this->lineNode->detachAllObjects();
			for (size_t i = 0; i < this->linesCount->getUInt(); i++)
			{
				this->gameObjectPtr->getSceneManager()->destroyManualObject(this->lineObjects[i]);
				this->lineObjects[i] = nullptr;
			}
			this->lineNode->getParentSceneNode()->removeAndDestroyChild(this->lineNode);
			this->lineNode = nullptr;
		}
	}

	Ogre::String LinesComponent::mapOperationTypeToString(Ogre::OperationType operationType)
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

	Ogre::OperationType LinesComponent::mapStringToOperationType(const Ogre::String& strOperationType)
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
	
	Ogre::String LinesComponent::getClassName(void) const
	{
		return "LinesComponent";
	}

	Ogre::String LinesComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

}; // namespace end
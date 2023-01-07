#include "NOWAPrecompiled.h"
#include "LineComponent.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "main/AppStateManager.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;
	
	LineComponent::LineComponent()
		: GameObjectComponent(),
		lineNode(nullptr),
		lineObject(nullptr),
		targetGameObject(nullptr),
		targetId(new Variant(LineComponent::AttrTargetId(), static_cast<unsigned long>(0), this->attributes, true)),
		color(new Variant(LineComponent::AttrColor(), Ogre::Vector3(1.0f, 0.0f, 0.0f), this->attributes)),
		sourceOffsetPosition(new Variant(LineComponent::AttrSourceOffsetPosition(), Ogre::Vector3::ZERO, this->attributes)),
		targetOffsetPosition(new Variant(LineComponent::AttrTargetOffsetPosition(), Ogre::Vector3::ZERO, this->attributes))
	{
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &LineComponent::handleTargetGameObjectDeleted), EventDataDeleteGameObject::getStaticEventType());
	}

	LineComponent::~LineComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[LineComponent] Destructor line component for game object: " + this->gameObjectPtr->getName());
		
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &LineComponent::handleTargetGameObjectDeleted), EventDataDeleteGameObject::getStaticEventType());
		
		this->destroyLine();
		// What if the target GO will be deleted?
		this->targetGameObject = nullptr;
	}
	
	void LineComponent::handleTargetGameObjectDeleted(NOWA::EventDataPtr eventData)
	{
		boost::shared_ptr<NOWA::EventDataDeleteGameObject> castEventData = boost::static_pointer_cast<EventDataDeleteGameObject>(eventData);
		
		if (nullptr != this->targetGameObject)
		{
			if (castEventData->getGameObjectId() == this->targetGameObject->getId())
			{
				this->targetGameObject = nullptr;
				this->destroyLine();
			}
		}
	}

	bool LineComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TargetId")
		{
			this->targetId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Color")
		{
			this->color->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SourceOffsetPosition")
		{
			this->sourceOffsetPosition->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TargetOffsetPosition")
		{
			this->targetOffsetPosition->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	GameObjectCompPtr LineComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		LineCompPtr clonedCompPtr(boost::make_shared<LineComponent>());

		
		clonedCompPtr->setTargetId(this->targetId->getULong());
		clonedCompPtr->setColor(this->color->getVector3());
		clonedCompPtr->setSourceOffsetPosition(this->sourceOffsetPosition->getVector3());
		clonedCompPtr->setTargetOffsetPosition(this->targetOffsetPosition->getVector3());
		
		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool LineComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[LineComponent] Init line component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool LineComponent::connect(void)
	{
		GameObjectPtr targetGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->targetId->getULong());
		if (nullptr == targetGameObjectPtr)
			return true;
		
		this->targetGameObject = targetGameObjectPtr.get();
		
		this->destroyLine();
		this->createLine();
		
		return true;
	}

	bool LineComponent::disconnect(void)
	{
		this->targetGameObject = nullptr;
		
		this->destroyLine();
		
		return true;
	}

	bool LineComponent::onCloned(void)
	{
		// Search for the prior id of the cloned game object and set the new id and set the new id, if not found set better 0, else the game objects may be corrupt!
		GameObjectPtr targetGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getClonedGameObjectFromPriorId(this->targetId->getULong());
		if (nullptr != targetGameObjectPtr)
			// Only the id is important!
			this->setTargetId(targetGameObjectPtr->getId());
		else
			this->setTargetId(0);
		
		return true;
	}

	void LineComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
			if (nullptr != this->targetGameObject)
			{
				// Set at source and target game object and relative offset to its orientations
				this->drawLine(this->gameObjectPtr->getPosition() + (this->gameObjectPtr->getOrientation() * this->sourceOffsetPosition->getVector3()),
								this->targetGameObject->getPosition() + (this->targetGameObject->getOrientation() * this->targetOffsetPosition->getVector3()));
			}
			else
			{
				// Set only relative at source game object and absolute target coordinate
				this->drawLine(this->gameObjectPtr->getPosition() + this->sourceOffsetPosition->getVector3(), this->targetOffsetPosition->getVector3());
			}
		}
	}

	void LineComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (LineComponent::AttrTargetId() == attribute->getName())
		{
			this->setTargetId(attribute->getULong());
		}
		else if (LineComponent::AttrColor() == attribute->getName())
		{
			this->setColor(attribute->getVector3());
		}
		else if (LineComponent::AttrSourceOffsetPosition() == attribute->getName())
		{
			this->setSourceOffsetPosition(attribute->getVector3());
		}
		else if (LineComponent::AttrTargetOffsetPosition() == attribute->getName())
		{
			this->setTargetOffsetPosition(attribute->getVector3());
		}
	}

	void LineComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TargetId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->targetId->getULong())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Color"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->color->getVector3())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SourceOffsetPosition"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->sourceOffsetPosition->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TargetOffsetPosition"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->targetOffsetPosition->getVector3())));
		propertiesXML->append_node(propertyXML);
	}
	
	void LineComponent::createLine(void)
	{
		this->lineNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode();
		// this->lineObject = new Ogre::v1::ManualObject(0, &this->sceneManager->_getEntityMemoryManager(Ogre::SCENE_DYNAMIC), this->sceneManager);
		this->lineObject = this->gameObjectPtr->getSceneManager()->createManualObject();
		this->lineObject->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_MESH);
		this->lineObject->setName("Line_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()));
		this->lineObject->setQueryFlags(0 << 0);
		this->lineNode->attachObject(this->lineObject);
	}

	void LineComponent::drawLine(const Ogre::Vector3& startPosition, const Ogre::Vector3& endPosition)
	{
		if (this->lineNode == nullptr)
		{
			this->createLine();
		}
		// Draw a 3D line between these points for visual effect
		this->lineObject->clear();
		// Or here via data block??
		this->lineObject->begin("WhiteNoLightingBackground", Ogre::OperationType::OT_LINE_LIST);
		Ogre::Vector3 color = this->color->getVector3();
		this->lineObject->position(startPosition);
		this->lineObject->colour(Ogre::ColourValue(color.x, color.y, color.z, 1.0f));
		this->lineObject->index(0);
		this->lineObject->position(endPosition);
		this->lineObject->colour(Ogre::ColourValue(color.x, color.y, color.z, 1.0f));
		this->lineObject->index(1);
		this->lineObject->end();
	}

	void LineComponent::destroyLine()
	{
		if (this->lineNode != nullptr)
		{
			this->lineNode->detachAllObjects();
			if (this->lineObject != nullptr)
			{
				this->gameObjectPtr->getSceneManager()->destroyManualObject(this->lineObject);
				// delete this->lineObject;
				this->lineObject = nullptr;
			}
			this->lineNode->getParentSceneNode()->removeAndDestroyChild(this->lineNode);
			this->lineNode = nullptr;
		}
	}

	Ogre::String LineComponent::getClassName(void) const
	{
		return "LineComponent";
	}

	Ogre::String LineComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void LineComponent::setTargetId(unsigned long targetId)
	{
		this->targetId->setValue(targetId);
	}

	unsigned long LineComponent::getTargetId(void) const
	{
		return this->targetId->getULong();
	}
	
	void LineComponent::setColor(const Ogre::Vector3& color)
	{
		this->color->setValue(color);
	}

	Ogre::Vector3 LineComponent::getColor(void) const
	{
		return this->color->getVector3();
	}

	void LineComponent::setSourceOffsetPosition(const Ogre::Vector3& sourceOffsetPosition)
	{
		this->sourceOffsetPosition->setValue(sourceOffsetPosition);
	}

	Ogre::Vector3 LineComponent::getSourceOffsetPosition(void) const
	{
		return this->sourceOffsetPosition->getVector3();
	}

	void LineComponent::setTargetOffsetPosition(const Ogre::Vector3& targetOffsetPosition)
	{
		this->targetOffsetPosition->setValue(targetOffsetPosition);
	}

	Ogre::Vector3 LineComponent::getTargetOffsetPosition(void) const
	{
		return this->targetOffsetPosition->getVector3();
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////

	LineMeshScaleComponent::LineMeshScaleComponent()
		: GameObjectComponent(),
		startPositionGameObject(nullptr),
		endPositionGameObject(nullptr),
		positionOffsetSnapshot(Ogre::Vector3::ZERO),
		positionSnapshot(Ogre::Vector3::ZERO),
		orientationOffsetSnapshot(Ogre::Quaternion::IDENTITY),
		scaleSnapshot(Ogre::Vector3::ZERO),
		sizeSnapshot(Ogre::Vector3::ZERO),
		startPositionId(new Variant(LineMeshScaleComponent::AttrStartPositionId(), static_cast<unsigned long>(0), this->attributes, true)),
		endPositionId(new Variant(LineMeshScaleComponent::AttrEndPositionId(), static_cast<unsigned long>(0), this->attributes, true)),
		meshScaleAxis(new Variant(LineMeshScaleComponent::AttrMeshScaleAxis(), Ogre::Vector3(0.0f, 1.0f, 0.0f), this->attributes))
	{
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &LineMeshScaleComponent::handleTargetGameObjectDeleted), EventDataDeleteGameObject::getStaticEventType());
	}

	LineMeshScaleComponent::~LineMeshScaleComponent()
	{
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &LineMeshScaleComponent::handleTargetGameObjectDeleted), EventDataDeleteGameObject::getStaticEventType());
	}

	void LineMeshScaleComponent::handleTargetGameObjectDeleted(NOWA::EventDataPtr eventData)
	{
		boost::shared_ptr<NOWA::EventDataDeleteGameObject> castEventData = boost::static_pointer_cast<EventDataDeleteGameObject>(eventData);
		
		if (nullptr != this->startPositionGameObject)
		{
			if (castEventData->getGameObjectId() == this->startPositionGameObject->getId())
			{
				this->endPositionGameObject = nullptr;
				this->positionOffsetSnapshot = Ogre::Vector3::ZERO;
				this->orientationOffsetSnapshot = Ogre::Quaternion::IDENTITY;
				this->scaleSnapshot = Ogre::Vector3::ZERO;
				this->sizeSnapshot = Ogre::Vector3::ZERO;
			}
			else if (castEventData->getGameObjectId() == this->endPositionGameObject->getId())
			{
				this->endPositionGameObject = nullptr;
				this->positionOffsetSnapshot = Ogre::Vector3::ZERO;
				this->orientationOffsetSnapshot = Ogre::Quaternion::IDENTITY;
				this->scaleSnapshot = Ogre::Vector3::ZERO;
				this->sizeSnapshot = Ogre::Vector3::ZERO;
			}
		}
	}

	bool LineMeshScaleComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "StartPositionId")
		{
			this->startPositionId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "EndPositionId")
		{
			this->endPositionId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MeshScaleAxis")
		{
			this->meshScaleAxis->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	GameObjectCompPtr LineMeshScaleComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		LineMeshScaleCompPtr clonedCompPtr(boost::make_shared<LineMeshScaleComponent>());

		
		clonedCompPtr->setStartPositionId(this->startPositionId->getULong());
		clonedCompPtr->setEndPositionId(this->endPositionId->getULong());
		clonedCompPtr->setMeshScaleAxis(this->meshScaleAxis->getVector3());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool LineMeshScaleComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[LineMeshScaleComponent] Init mesh scale component for game object: " + this->gameObjectPtr->getName());

		if (nullptr ==  this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>())
		{
			return false;
		}

		return GameObjectComponent::postInit();
	}

	bool LineMeshScaleComponent::connect(void)
	{
		GameObjectPtr startPositionGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->startPositionId->getULong());
		
		GameObjectPtr endPositionGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->endPositionId->getULong());
		if (nullptr == endPositionGameObjectPtr)
			return true;

		if (nullptr != startPositionGameObjectPtr)
		{
			this->startPositionGameObject = startPositionGameObjectPtr.get();
		}
		else
		{
			this->startPositionGameObject = this->gameObjectPtr.get();
		}
		this->endPositionGameObject = endPositionGameObjectPtr.get();

		// Snapshot where the source game object was placed and how it was orientated, this is the base for calculation
		this->positionOffsetSnapshot = this->startPositionGameObject->getPosition() - this->gameObjectPtr->getPosition();
		this->orientationOffsetSnapshot = this->endPositionGameObject->getOrientation() * this->gameObjectPtr->getOrientation().Inverse();
		this->scaleSnapshot = this->gameObjectPtr->getScale();
		this->sizeSnapshot = this->gameObjectPtr->getSize();

		return true;
	}

	bool LineMeshScaleComponent::disconnect(void)
	{
		this->startPositionGameObject = nullptr;
		this->endPositionGameObject = nullptr;
		this->positionOffsetSnapshot = Ogre::Vector3::ZERO;
		this->orientationOffsetSnapshot = Ogre::Quaternion::IDENTITY;

		if (Ogre::Vector3::ZERO != this->scaleSnapshot)
		{
			this->gameObjectPtr->getSceneNode()->setScale(this->scaleSnapshot);
		}
		this->sizeSnapshot = Ogre::Vector3::ZERO;

		return true;
	}

	bool LineMeshScaleComponent::onCloned(void)
	{
		// Search for the prior id of the cloned game object and set the new id and set the new id, if not found set better 0, else the game objects may be corrupt!
		GameObjectPtr startPositionGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getClonedGameObjectFromPriorId(this->startPositionId->getULong());
		if (nullptr != startPositionGameObjectPtr)
			// Only the id is important!
			this->setStartPositionId(startPositionGameObjectPtr->getId());
		else
			this->setStartPositionId(0);

		GameObjectPtr endPositionGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getClonedGameObjectFromPriorId(this->endPositionId->getULong());
		if (nullptr != endPositionGameObjectPtr)
			// Only the id is important!
			this->setEndPositionId(endPositionGameObjectPtr->getId());
		else
			this->setEndPositionId(0);
		
		return true;
	}

	void LineMeshScaleComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
			if (nullptr == this->startPositionGameObject || nullptr == this->endPositionGameObject)
			{
				return;
			}

			this->gameObjectPtr->getSceneNode()->setPosition(this->startPositionGameObject->getPosition() - (this->startPositionGameObject->getSceneNode()->getOrientation() * this->positionOffsetSnapshot));
			this->gameObjectPtr->getSceneNode()->setOrientation(this->endPositionGameObject->getOrientation() * this->orientationOffsetSnapshot.Inverse());

			Ogre::Real length = (this->endPositionGameObject->getPosition() - this->gameObjectPtr->getSceneNode()->getPosition()).length();
			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "->length: " + Ogre::StringConverter::toString(length));

			Ogre::Real affectiveSize = 1.0f;
			Ogre::Real scale = 1.0f;
			Ogre::Real padding = 0.008f; // Ogre::v1::MeshManager::getSingleton().getBoundsPaddingFactor();

			if (this->meshScaleAxis->getVector3().x == 1.0f)
			{
				affectiveSize = this->sizeSnapshot.x + padding;
				scale = this->scaleSnapshot.x * length / affectiveSize;

				if (scale < 0.05f) scale = 0.05f;

				this->gameObjectPtr->getSceneNode()->setScale(Ogre::Vector3(scale, this->scaleSnapshot.y, this->scaleSnapshot.z));
				// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "->scale: " + Ogre::StringConverter::toString(Ogre::Vector3(scale, this->scaleSnapshot.y, this->scaleSnapshot.z)));
			}
			else if (this->meshScaleAxis->getVector3().y == 1.0f)
			{
				affectiveSize = this->sizeSnapshot.y + padding;
				scale = this->scaleSnapshot.y * length / affectiveSize;
				
				if (scale < 0.05f) scale = 0.05f;

				this->gameObjectPtr->getSceneNode()->setScale(Ogre::Vector3(this->scaleSnapshot.x, scale, this->scaleSnapshot.z));
			}
			else
			{
				affectiveSize = this->sizeSnapshot.z + padding;
				scale = this->scaleSnapshot.z * length / affectiveSize;
				
				if (scale < 0.05f) scale = 0.05f;

				this->gameObjectPtr->getSceneNode()->setScale(Ogre::Vector3(this->scaleSnapshot.x, this->scaleSnapshot.y, scale));
			}

			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "->btoffset: " + Ogre::StringConverter::toString(this->gameObjectPtr->getBottomOffset()));
		}
	}

	void LineMeshScaleComponent::actualizeValue(Variant* attribute)
	{
		if (LineMeshScaleComponent::AttrStartPositionId() == attribute->getName())
		{
			this->setStartPositionId(attribute->getULong());
		}
		else if (LineMeshScaleComponent::AttrEndPositionId() == attribute->getName())
		{
			this->setEndPositionId(attribute->getULong());
		}
		else if (LineMeshScaleComponent::AttrMeshScaleAxis() == attribute->getName())
		{
			this->setMeshScaleAxis(attribute->getVector3());
		}
	}

	void LineMeshScaleComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "StartPositionId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->startPositionId->getULong())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "EndPositionId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->endPositionId->getULong())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MeshScaleAxis"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->meshScaleAxis->getVector3())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String LineMeshScaleComponent::getClassName(void) const
	{
		return "LineMeshScaleComponent";
	}

	Ogre::String LineMeshScaleComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void LineMeshScaleComponent::setStartPositionId(unsigned long startPositionId)
	{
		this->startPositionId->setValue(startPositionId);
	}

	unsigned long LineMeshScaleComponent::getStartPositionId(void) const
	{
		return this->startPositionId->getULong();
	}

	void LineMeshScaleComponent::setEndPositionId(unsigned long endPositionId)
	{
		this->endPositionId->setValue(endPositionId);
	}

	unsigned long LineMeshScaleComponent::getEndPositionId(void) const
	{
		return this->endPositionId->getULong();
	}

	void NOWA::LineMeshScaleComponent::setMeshScaleAxis(const Ogre::Vector3& meshScale)
	{
		this->meshScaleAxis->setValue(meshScale);
	}

	Ogre::Vector3 NOWA::LineMeshScaleComponent::getMeshScaleAxis(void) const
	{
		return this->meshScaleAxis->getVector3();
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////

	LineMeshComponent::LineMeshComponent()
		: GameObjectComponent(),
		oldCount(0),
		meshSize(0.0f),
		startPositionGameObject(nullptr),
		endPositionGameObject(nullptr),
		positionOffsetSnapshot(Ogre::Vector3::ZERO),
		positionSnapshot(Ogre::Vector3::ZERO),
		orientationSnapshot(Ogre::Quaternion::IDENTITY),
		orientationOffsetSnapshot(Ogre::Quaternion::IDENTITY),
		scaleSnapshot(Ogre::Vector3::ZERO),
		sizeSnapshot(Ogre::Vector3::ZERO),
		offsetPositionFactor(new Variant(LineMeshComponent::AttrOffsetPositionFactor(), Ogre::Vector3::UNIT_SCALE, this->attributes)),
		offsetOrientation(new Variant(LineMeshComponent::AttrOffsetOrientation(), Ogre::Vector3::ZERO, this->attributes)),
		expandAxis(new Variant(LineMeshComponent::AttrExpandAxis(), Ogre::Vector3::UNIT_X, this->attributes)),
		startPositionId(new Variant(LineMeshComponent::AttrStartPositionId(), static_cast<unsigned long>(0), this->attributes, true)),
		endPositionId(new Variant(LineMeshComponent::AttrEndPositionId(), static_cast<unsigned long>(0), this->attributes, true))
	{
		this->offsetPositionFactor->setDescription("Sets the offset position factor at which the source game object should be attached (e.g. 2 1 1 would attach each 2x multiplicated factor).");
		this->offsetOrientation->setDescription("Orientation is set in the form: (degreeX, degreeY, degreeZ).");
		this->expandAxis->setDescription("Sets the expand axis, at which this game object will be duplicated.");

		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &LineMeshComponent::handleTargetGameObjectDeleted), EventDataDeleteGameObject::getStaticEventType());
	}

	LineMeshComponent::~LineMeshComponent()
	{
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &LineMeshComponent::handleTargetGameObjectDeleted), EventDataDeleteGameObject::getStaticEventType());
	}

	void LineMeshComponent::handleTargetGameObjectDeleted(NOWA::EventDataPtr eventData)
	{
		boost::shared_ptr<NOWA::EventDataDeleteGameObject> castEventData = boost::static_pointer_cast<EventDataDeleteGameObject>(eventData);
		
		if (nullptr != this->startPositionGameObject)
		{
			if (castEventData->getGameObjectId() == this->startPositionGameObject->getId())
			{
				this->endPositionGameObject = nullptr;
				this->positionOffsetSnapshot = Ogre::Vector3::ZERO;
				this->orientationOffsetSnapshot = Ogre::Quaternion::IDENTITY;
				this->scaleSnapshot = Ogre::Vector3::ZERO;
				this->sizeSnapshot = Ogre::Vector3::ZERO;
			}
			else if (castEventData->getGameObjectId() == this->endPositionGameObject->getId())
			{
				this->endPositionGameObject = nullptr;
				this->positionOffsetSnapshot = Ogre::Vector3::ZERO;
				this->orientationOffsetSnapshot = Ogre::Quaternion::IDENTITY;
				this->scaleSnapshot = Ogre::Vector3::ZERO;
				this->sizeSnapshot = Ogre::Vector3::ZERO;
			}
		}
	}

	bool LineMeshComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		bool success = GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OffsetPositionFactor")
		{
			this->offsetPositionFactor->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OffsetOrientation")
		{
			this->offsetOrientation->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ExpandAxis")
		{
			this->expandAxis->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "StartPositionId")
		{
			this->startPositionId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "EndPositionId")
		{
			this->endPositionId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return success;
	}

	GameObjectCompPtr LineMeshComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		LineMeshCompPtr clonedCompPtr(boost::make_shared<LineMeshComponent>());

		
		clonedCompPtr->setOffsetPositionFactor(this->offsetPositionFactor->getVector3());
		clonedCompPtr->setOffsetOrientation(this->offsetOrientation->getVector3());
		clonedCompPtr->setExpandAxis(this->expandAxis->getVector3());
		clonedCompPtr->setStartPositionId(this->startPositionId->getULong());
		clonedCompPtr->setEndPositionId(this->endPositionId->getULong());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool LineMeshComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[LineMeshComponent] Init line mesh component for game object: " + this->gameObjectPtr->getName());

		return GameObjectComponent::postInit();
	}

	bool LineMeshComponent::connect(void)
	{
		GameObjectPtr startPositionGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->startPositionId->getULong());
		GameObjectPtr endPositionGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->endPositionId->getULong());

		if (nullptr == endPositionGameObjectPtr)
			return true;

		if (nullptr != startPositionGameObjectPtr)
		{
			this->startPositionGameObject = startPositionGameObjectPtr.get();
		}
		else
		{
			this->startPositionGameObject = this->gameObjectPtr.get();
		}
		this->endPositionGameObject = endPositionGameObjectPtr.get();

		// Snapshot where the source game object was placed and how it was orientated, this is the base for calculation (local coords)
		this->positionOffsetSnapshot = this->offsetPositionFactor->getVector3() - this->gameObjectPtr->getPosition();
		this->orientationOffsetSnapshot = MathHelper::getInstance()->degreesToQuat(this->offsetOrientation->getVector3()) * this->gameObjectPtr->getOrientation().Inverse();
		this->scaleSnapshot = this->gameObjectPtr->getScale();
		this->sizeSnapshot = this->gameObjectPtr->getSize();

		if (this->expandAxis->getVector3().x != 0.0f)
		{
			this->meshSize = this->offsetPositionFactor->getVector3().x * this->gameObjectPtr->getSceneNode()->getAttachedObject(0)->getLocalAabb().getSize().x * this->gameObjectPtr->getSceneNode()->getScale().x;
		}
		else if (this->expandAxis->getVector3().y != 0.0f)
		{
			this->meshSize = this->offsetPositionFactor->getVector3().y * this->gameObjectPtr->getSceneNode()->getAttachedObject(0)->getLocalAabb().getSize().y * this->gameObjectPtr->getSceneNode()->getScale().y;
		}
		else if (this->expandAxis->getVector3().z != 0.0f)
		{
			this->meshSize = this->offsetPositionFactor->getVector3().z * this->gameObjectPtr->getSceneNode()->getAttachedObject(0)->getLocalAabb().getSize().z * this->gameObjectPtr->getSceneNode()->getScale().z;
		}

		this->positionSnapshot = this->gameObjectPtr->getSceneNode()->getPosition();
		this->orientationSnapshot = this->gameObjectPtr->getSceneNode()->getOrientation();

		Ogre::Vector3 resultDirection = (this->endPositionGameObject->getPosition() - this->startPositionGameObject->getPosition()).normalisedCopy();
		Ogre::Quaternion orientation = MathHelper::getInstance()->faceDirection(this->gameObjectPtr->getSceneNode(), resultDirection, this->expandAxis->getVector3()) * MathHelper::getInstance()->degreesToQuat(this->offsetOrientation->getVector3());
		this->gameObjectPtr->getSceneNode()->setPosition(this->startPositionGameObject->getPosition());
		this->gameObjectPtr->getSceneNode()->setOrientation(orientation);

		return true;
	}

	bool LineMeshComponent::disconnect(void)
	{
		this->startPositionGameObject = nullptr;
		this->endPositionGameObject = nullptr;
		this->positionOffsetSnapshot = Ogre::Vector3::ZERO;
		this->orientationOffsetSnapshot = Ogre::Quaternion::IDENTITY;
		this->sizeSnapshot = Ogre::Vector3::ZERO;

		this->gameObjectPtr->getSceneNode()->setPosition(this->positionSnapshot );
		this->gameObjectPtr->getSceneNode()->setOrientation(this->orientationSnapshot);

		this->clearMeshes();

		return true;
	}

	bool LineMeshComponent::onCloned(void)
	{
		// Search for the prior id of the cloned game object and set the new id and set the new id, if not found set better 0, else the game objects may be corrupt!
		GameObjectPtr startPositionGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getClonedGameObjectFromPriorId(this->startPositionId->getULong());
		if (nullptr != startPositionGameObjectPtr)
			// Only the id is important!
			this->setStartPositionId(startPositionGameObjectPtr->getId());
		else
			this->setStartPositionId(0);

		GameObjectPtr endPositionGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getClonedGameObjectFromPriorId(this->endPositionId->getULong());
		if (nullptr != endPositionGameObjectPtr)
			// Only the id is important!
			this->setEndPositionId(endPositionGameObjectPtr->getId());
		else
			this->setEndPositionId(0);
		
		return true;
	}

	void LineMeshComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
			if (nullptr == this->startPositionGameObject || nullptr == this->endPositionGameObject)
			{
				return;
			}

			Ogre::Vector3 resultDirection = this->endPositionGameObject->getPosition() - this->startPositionGameObject->getPosition();
			Ogre::Real length = resultDirection.length();
			// No normalize call, since a root square would be calculated a second time!
			if (length > 0.0f)
			{
				Ogre::Real fInvLength = 1.0f / length;
				resultDirection.x *= fInvLength;
				resultDirection.y *= fInvLength;
				resultDirection.z *= fInvLength;
			}

			//  * this->meshScale->getVector3().x -> is already included in meshSize
			unsigned int count = static_cast<unsigned int>(length / this->meshSize);

			this->setMeshCount(count, resultDirection);

			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "count: " + Ogre::StringConverter::toString(count));
		}
	}

	void LineMeshComponent::setMeshCount(unsigned int meshCount, const Ogre::Vector3& direction)
	{
		// Add new ones
		if (this->oldCount < meshCount)
		{
			for (size_t i = this->oldCount; i < meshCount; i++)
			{
				this->pushMesh(direction);
			}
		}
		else if (this->oldCount > meshCount)
		{
			// Remove
			for (size_t i = meshCount; i < this->oldCount; i++)
			{
				this->popMesh();
			}
		}
		else
		{
			this->updateMeshes(direction);
		}

		this->oldCount = meshCount;
	}

	void LineMeshComponent::pushMesh(const Ogre::Vector3& direction)
	{
		// See: https://stackoverflow.com/questions/49173095/how-to-move-an-object-along-a-line-given-two-points
		Ogre::SceneNode* sceneNode = this->addSceneNode();

		Ogre::Vector3 position = Ogre::Vector3::ZERO;

		if (true == this->meshes.empty())
		{
			position = this->gameObjectPtr->getPosition() + ((direction * this->offsetPositionFactor->getVector3()) * (this->gameObjectPtr->getSceneNode()->getAttachedObject(0)->getLocalAabb().getSize() * this->gameObjectPtr->getSceneNode()->getScale()));
			sceneNode->setPosition(position);
			sceneNode->setOrientation(this->gameObjectPtr->getSceneNode()->getOrientation());
		}

		unsigned int oldSize = static_cast<unsigned int>(this->meshes.size());

		if (false == this->meshes.empty())
		{
			Ogre::SceneNode* priorSceneNode = this->meshes[oldSize - 1];
			position = priorSceneNode->getPosition() + ((direction * this->offsetPositionFactor->getVector3()) * (priorSceneNode->getAttachedObject(0)->getLocalAabb().getSize() * this->gameObjectPtr->getSceneNode()->getScale()));
			sceneNode->setPosition(position);
			sceneNode->setOrientation(this->gameObjectPtr->getSceneNode()->getOrientation());
		}

		this->meshes.push_back(sceneNode);
	}

	void LineMeshComponent::updateMeshes(const Ogre::Vector3& direction)
	{
		Ogre::SceneNode* priorSceneNode = nullptr;

		Ogre::Quaternion orientation = MathHelper::getInstance()->faceDirection(this->gameObjectPtr->getSceneNode(), direction, this->expandAxis->getVector3()) * MathHelper::getInstance()->degreesToQuat(this->offsetOrientation->getVector3());
		this->gameObjectPtr->getSceneNode()->setPosition(this->startPositionGameObject->getPosition());
		this->gameObjectPtr->getSceneNode()->setOrientation(orientation);

		for (size_t i = 0; i < this->meshes.size(); i++)
		{
			Ogre::SceneNode* sceneNode = this->meshes[i];
			Ogre::Vector3 position = Ogre::Vector3::ZERO;
			if (i == 0)
			{
				position = this->gameObjectPtr->getPosition() + (direction * this->offsetPositionFactor->getVector3()) * (this->gameObjectPtr->getSceneNode()->getAttachedObject(0)->getLocalAabb().getSize() * this->gameObjectPtr->getSceneNode()->getScale());
				sceneNode->setPosition(position);
				sceneNode->setOrientation(this->gameObjectPtr->getSceneNode()->getOrientation());
			}
			else
			{
				priorSceneNode = this->meshes[i - 1];
				position = priorSceneNode->getPosition() + (direction * this->offsetPositionFactor->getVector3()) * (priorSceneNode->getAttachedObject(0)->getLocalAabb().getSize() * this->gameObjectPtr->getSceneNode()->getScale());
				sceneNode->setPosition(position);
				sceneNode->setOrientation(this->gameObjectPtr->getSceneNode()->getOrientation());
			}
		}
	}

	void LineMeshComponent::popMesh(void)
	{
		Ogre::SceneNode* sceneNode = this->meshes.back();

		this->deleteSceneNode(sceneNode);
		this->meshes.pop_back();
	}

	void LineMeshComponent::clearMeshes(void)
	{
		while (this->meshes.size() > 0)
		{
			this->popMesh();
		}
		this->meshes.clear();
		this->oldCount = 0;
	}

	Ogre::SceneNode* LineMeshComponent::addSceneNode(void)
	{
		// Creating out from root, else transform is relative, which is ugly
		Ogre::SceneNode* sceneNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode(Ogre::SCENE_DYNAMIC);
		// sceneNode->setOrientation(this->tagPointNode->getOrientation() * this->gameObjectPtr->getSceneNode()->getOrientation());
		Ogre::v1::Entity* entity = this->gameObjectPtr->getSceneManager()->createEntity(this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>()->getMesh());
		entity->setQueryFlags(0 << 0);
		entity->setCastShadows(this->gameObjectPtr->getMovableObject()->getCastShadows());
		sceneNode->attachObject(entity);
		sceneNode->setScale(this->gameObjectPtr->getScale());

		return sceneNode;
	}

	void LineMeshComponent::deleteSceneNode(Ogre::SceneNode* sceneNode)
	{
		if (nullptr != sceneNode)
		{
			Ogre::MovableObject* movableObject = sceneNode->getAttachedObject(0);

			auto nodeIt = sceneNode->getChildIterator();
			while (nodeIt.hasMoreElements())
			{
				//go through all scenenodes in the scene
				Ogre::Node* subNode = nodeIt.getNext();
				subNode->removeAllChildren();
			}

			sceneNode->detachAllObjects();
			this->gameObjectPtr->getSceneManager()->getRootSceneNode()->removeAndDestroyChild(sceneNode);
			sceneNode = nullptr;

			if (this->gameObjectPtr->getSceneManager()->hasMovableObject(movableObject))
			{
				this->gameObjectPtr->getSceneManager()->destroyMovableObject(movableObject);
				movableObject = nullptr;
			}
		}
	}

	void LineMeshComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (LineMeshComponent::AttrOffsetPositionFactor() == attribute->getName())
		{
			this->setOffsetPositionFactor(attribute->getVector3());
		}
		else if (LineMeshComponent::AttrOffsetOrientation() == attribute->getName())
		{
			this->setOffsetOrientation(attribute->getVector3());
		}
		if (LineMeshScaleComponent::AttrStartPositionId() == attribute->getName())
		{
			this->setStartPositionId(attribute->getULong());
		}
		else if (LineMeshScaleComponent::AttrEndPositionId() == attribute->getName())
		{
			this->setEndPositionId(attribute->getULong());
		}
	}

	void LineMeshComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		GameObjectComponent::writeXML(propertiesXML, doc, filePath);
		
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OffsetPositionFactor"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->offsetPositionFactor->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OffsetOrientation"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->offsetOrientation->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ExpandAxis"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->expandAxis->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "StartPositionId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->startPositionId->getULong())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "EndPositionId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->endPositionId->getULong())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String LineMeshComponent::getClassName(void) const
	{
		return "LineMeshComponent";
	}

	Ogre::String LineMeshComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void NOWA::LineMeshComponent::setOffsetPositionFactor(const Ogre::Vector3& offsetPositionFactor)
	{
		this->offsetPositionFactor->setValue(offsetPositionFactor);
	}

	Ogre::Vector3 NOWA::LineMeshComponent::getOffsetPositionFactor(void) const
	{
		return this->offsetPositionFactor->getVector3();
	}

	void LineMeshComponent::setOffsetOrientation(const Ogre::Vector3& offsetOrientation)
	{
		this->offsetOrientation->setValue(offsetOrientation);
	}

	Ogre::Vector3 LineMeshComponent::getOffsetOrientation(void) const
	{
		return this->offsetOrientation->getVector3();
	}

	void LineMeshComponent::setExpandAxis(const Ogre::Vector3& expandAxis)
	{
		if (expandAxis == Ogre::Vector3::ZERO)
		{
			return;
		}
		this->expandAxis->setValue(expandAxis);
	}

	Ogre::Vector3 LineMeshComponent::getExpandAxis(void) const
	{
		return this->expandAxis->getVector3();
	}

	void LineMeshComponent::setStartPositionId(unsigned long startPositionId)
	{
		this->startPositionId->setValue(startPositionId);
	}

	unsigned long LineMeshComponent::getStartPositionId(void) const
	{
		return this->startPositionId->getULong();
	}

	void LineMeshComponent::setEndPositionId(unsigned long endPositionId)
	{
		this->endPositionId->setValue(endPositionId);
	}

	unsigned long LineMeshComponent::getEndPositionId(void) const
	{
		return this->endPositionId->getULong();
	}

}; // namespace end
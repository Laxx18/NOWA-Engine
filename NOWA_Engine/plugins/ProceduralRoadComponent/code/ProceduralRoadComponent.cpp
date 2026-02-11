/*
Copyright (c) 2026 Lukas Kalinowski

GPL v3
*/

#include "NOWAPrecompiled.h"
#include "ProceduralRoadComponent.h"
#include "gameobject/GameObjectController.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "main/AppStateManager.h"
#include "main/Core.h"
#include "main/InputDeviceCore.h"
#include "gameobject/GameObjectFactory.h"
#include "modules/GraphicsModule.h"
#include "editor/EditorManager.h"

#include "OgreItem.h"
#include "OgreMeshManager2.h"
#include "OgreMesh2.h"
#include "OgreSubMesh2.h"
#include "Vao/OgreVaoManager.h"
#include "Vao/OgreVertexArrayObject.h"
#include "OgreHlmsManager.h"
#include "OgreHlmsPbs.h"
#include "OgreHlmsPbsDatablock.h"
#include "OgreMesh2Serializer.h"

#include "OgreAbiUtils.h"

#include <fstream>

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	ProceduralRoadComponent::ProceduralRoadComponent()
		: GameObjectComponent(),
		name("ProceduralRoadComponent"),
		activated(new Variant(ProceduralRoadComponent::AttrActivated(), true, this->attributes)),
		roadWidth(new Variant(ProceduralRoadComponent::AttrRoadWidth(), 4.0f, this->attributes)),
		edgeWidth(new Variant(ProceduralRoadComponent::AttrEdgeWidth(), 0.5f, this->attributes)),
		roadStyle(new Variant(ProceduralRoadComponent::AttrRoadStyle(), { "Paved", "Highway", "Trail", "Dirt", "Cobblestone" }, this->attributes)),
		snapToGrid(new Variant(ProceduralRoadComponent::AttrSnapToGrid(), false, this->attributes)),
		gridSize(new Variant(ProceduralRoadComponent::AttrGridSize(), 1.0f, this->attributes)),
		adaptToGround(new Variant(ProceduralRoadComponent::AttrAdaptToGround(), true, this->attributes)),
		heightOffset(new Variant(ProceduralRoadComponent::AttrHeightOffset(), 0.1f, this->attributes)),
		maxGradient(new Variant(ProceduralRoadComponent::AttrMaxGradient(), 15.0f, this->attributes)),
		smoothingFactor(new Variant(ProceduralRoadComponent::AttrSmoothingFactor(), 0.5f, this->attributes)),
		enableBanking(new Variant(ProceduralRoadComponent::AttrEnableBanking(), false, this->attributes)),
		bankingAngle(new Variant(ProceduralRoadComponent::AttrBankingAngle(), 5.0f, this->attributes)),
		curveSubdivisions(new Variant(ProceduralRoadComponent::AttrCurveSubdivisions(), 10, this->attributes)),
		centerDatablock(new Variant(ProceduralRoadComponent::AttrCenterDatablock(), Ogre::String("proceduralWall1"), this->attributes)),
		edgeDatablock(new Variant(ProceduralRoadComponent::AttrEdgeDatablock(), Ogre::String("Stone 4"), this->attributes)),
		centerUVTiling(new Variant(ProceduralRoadComponent::AttrCenterUVTiling(), Ogre::Vector2(1.0f, 1.0f), this->attributes)),
		edgeUVTiling(new Variant(ProceduralRoadComponent::AttrEdgeUVTiling(), Ogre::Vector2(1.0f, 1.0f), this->attributes)),
		curbHeight(new Variant(ProceduralRoadComponent::AttrCurbHeight(), 0.15f, this->attributes)),
		terrainSampleInterval(new Variant(ProceduralRoadComponent::AttrTerrainSampleInterval(), 2.0f, this->attributes)),
		buildState(BuildState::IDLE),
		currentCenterVertexIndex(0),
		currentEdgeVertexIndex(0),
		roadItem(nullptr),
		previewItem(nullptr),
		previewNode(nullptr),
		isShiftPressed(false),
		isCtrlPressed(false),
		continuousMode(false),
		hasRoadOrigin(false),
		groundQuery(nullptr),
		canModify(false)
	{
		this->roadStyle->setDescription("Style of the road to generate");
		this->roadWidth->setDescription("Width of the main road surface (meters)");
		this->edgeWidth->setDescription("Width of the curb/edge strips on each side (meters)");
		this->heightOffset->setDescription("Height above terrain surface (meters)");
		this->maxGradient->setDescription("Maximum allowed slope angle (degrees)");
		this->smoothingFactor->setDescription("Amount of height smoothing (0-1, higher = smoother gradients)");
		this->enableBanking->setDescription("Enable road banking on curves");
		this->bankingAngle->setDescription("Maximum banking angle for curves (degrees)");
		this->curveSubdivisions->setDescription("Number of segments for curved roads (higher = smoother)");
		this->curbHeight->setDescription("Height of the curb edges above the road surface (meters). Set 0 for flat edges.");
		this->terrainSampleInterval->setDescription("Distance between terrain height samples along the road (meters). Lower = better terrain following but more geometry.");

		this->roadWidth->addUserData(GameObject::AttrActionNeedRefresh());
		this->edgeWidth->addUserData(GameObject::AttrActionNeedRefresh());
		this->roadStyle->addUserData(GameObject::AttrActionNeedRefresh());
		this->heightOffset->addUserData(GameObject::AttrActionNeedRefresh());
		this->maxGradient->addUserData(GameObject::AttrActionNeedRefresh());
		this->smoothingFactor->addUserData(GameObject::AttrActionNeedRefresh());
		this->centerDatablock->addUserData(GameObject::AttrActionNeedRefresh());
		this->edgeDatablock->addUserData(GameObject::AttrActionNeedRefresh());
		this->curbHeight->addUserData(GameObject::AttrActionNeedRefresh());
		this->terrainSampleInterval->addUserData(GameObject::AttrActionNeedRefresh());
	}

	ProceduralRoadComponent::~ProceduralRoadComponent()
	{
		
	}

	void ProceduralRoadComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<ProceduralRoadComponent>(ProceduralRoadComponent::getStaticClassId(), ProceduralRoadComponent::getStaticClassName());
	}

	void ProceduralRoadComponent::initialise()
	{
	}

	void ProceduralRoadComponent::shutdown()
	{
	}

	void ProceduralRoadComponent::uninstall()
	{
	}

	const Ogre::String& ProceduralRoadComponent::getName() const
	{
		return this->name;
	}

	void ProceduralRoadComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool ProceduralRoadComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RoadWidth")
		{
			this->roadWidth->setValue(XMLConverter::getAttribReal(propertyElement, "data", 4.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "EdgeWidth")
		{
			this->edgeWidth->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.5f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RoadStyle")
		{
			this->roadStyle->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SnapToGrid")
		{
			this->snapToGrid->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "GridSize")
		{
			this->gridSize->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AdaptToGround")
		{
			this->adaptToGround->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "HeightOffset")
		{
			this->heightOffset->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.1f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MaxGradient")
		{
			this->maxGradient->setValue(XMLConverter::getAttribReal(propertyElement, "data", 15.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SmoothingFactor")
		{
			this->smoothingFactor->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.5f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "EnableBanking")
		{
			this->enableBanking->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "BankingAngle")
		{
			this->bankingAngle->setValue(XMLConverter::getAttribReal(propertyElement, "data", 5.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CurveSubdivisions")
		{
			this->curveSubdivisions->setValue(XMLConverter::getAttribInt(propertyElement, "data", 10));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CenterDatablock")
		{
			this->centerDatablock->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "EdgeDatablock")
		{
			this->edgeDatablock->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CenterUVTiling")
		{
			this->centerUVTiling->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "EdgeUVTiling")
		{
			this->edgeUVTiling->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CurbHeight")
		{
			this->curbHeight->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.15f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TerrainSampleInterval")
		{
			this->terrainSampleInterval->setValue(XMLConverter::getAttribReal(propertyElement, "data", 2.0f));
			propertyElement = propertyElement->next_sibling("property");
		}

		return true;
	}

	GameObjectCompPtr ProceduralRoadComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		ProceduralRoadComponentPtr clonedCompPtr(boost::make_shared<ProceduralRoadComponent>());

		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setRoadWidth(this->roadWidth->getReal());
		clonedCompPtr->setEdgeWidth(this->edgeWidth->getReal());
		clonedCompPtr->setRoadStyle(this->roadStyle->getListSelectedValue());
		clonedCompPtr->setSnapToGrid(this->snapToGrid->getBool());
		clonedCompPtr->setGridSize(this->gridSize->getReal());
		clonedCompPtr->setAdaptToGround(this->adaptToGround->getBool());
		clonedCompPtr->setHeightOffset(this->heightOffset->getReal());
		clonedCompPtr->setMaxGradient(this->maxGradient->getReal());
		clonedCompPtr->setSmoothingFactor(this->smoothingFactor->getReal());
		clonedCompPtr->setEnableBanking(this->enableBanking->getBool());
		clonedCompPtr->setBankingAngle(this->bankingAngle->getReal());
		clonedCompPtr->setCurveSubdivisions(this->curveSubdivisions->getInt());
		clonedCompPtr->setCenterDatablock(this->centerDatablock->getString());
		clonedCompPtr->setEdgeDatablock(this->edgeDatablock->getString());
		clonedCompPtr->setCenterUVTiling(this->centerUVTiling->getVector2());
		clonedCompPtr->setEdgeUVTiling(this->edgeUVTiling->getVector2());
		clonedCompPtr->setCurbHeight(this->curbHeight->getReal());
		clonedCompPtr->setTerrainSampleInterval(this->terrainSampleInterval->getReal());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool ProceduralRoadComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralRoadComponent] Init road component for game object: " + this->gameObjectPtr->getName());

		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &ProceduralRoadComponent::handleMeshModifyMode), NOWA::EventDataEditorMode::getStaticEventType());
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &ProceduralRoadComponent::handleGameObjectSelected), NOWA::EventDataGameObjectSelected::getStaticEventType());

		// Create raycast query for ground detection
		this->groundQuery = this->gameObjectPtr->getSceneManager()->createRayQuery(Ogre::Ray(), GameObjectController::ALL_CATEGORIES_ID);
		this->groundQuery->setSortByDistance(true);

		// Setup fallback ground plane at Y=0
		this->groundPlane = Ogre::Plane(Ogre::Vector3::UNIT_Y, 0.0f);

		// Create preview scene node
		this->previewNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode();

		// Load wall data from file
		if (true == this->loadRoadDataFromFile())
		{
			// If we successfully loaded segments, rebuild the mesh
			if (false == this->roadSegments.empty())
			{
				this->rebuildMesh();

				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralRoadComponent] Successfully loaded and rebuilt road with " + Ogre::StringConverter::toString(this->roadSegments.size()) + " segments");
			}
		}

		return true;
	}

	bool ProceduralRoadComponent::connect(void)
	{

		return true;
	}

	bool ProceduralRoadComponent::disconnect(void)
	{
		this->destroyPreviewMesh();
		this->buildState = BuildState::IDLE;

		return true;
	}

	bool ProceduralRoadComponent::onCloned(void)
	{
		return true;
	}

	void ProceduralRoadComponent::onAddComponent(void)
	{
		boost::shared_ptr<EventDataEditorMode> eventDataEditorMode(new EventDataEditorMode(EditorManager::EDITOR_MESH_MODIFY_MODE));
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->threadSafeQueueEvent(eventDataEditorMode);
		this->canModify = true;

		this->addInputListener();
	}

	void ProceduralRoadComponent::onRemoveComponent(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralRoadComponent] Destructor road component for game object: " + this->gameObjectPtr->getName());

		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &ProceduralRoadComponent::handleMeshModifyMode), NOWA::EventDataEditorMode::getStaticEventType());
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &ProceduralRoadComponent::handleGameObjectSelected), NOWA::EventDataGameObjectSelected::getStaticEventType());

		if (nullptr != this->groundQuery)
		{
			this->gameObjectPtr->getSceneManager()->destroyQuery(this->groundQuery);
			this->groundQuery = nullptr;
		}

		InputDeviceCore::getSingletonPtr()->removeKeyListener(ProceduralRoadComponent::getStaticClassName() + "_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()));
		InputDeviceCore::getSingletonPtr()->removeMouseListener(ProceduralRoadComponent::getStaticClassName() + "_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()));

		this->destroyRoadMesh();
		this->destroyPreviewMesh();

		if (this->previewNode)
		{
			this->gameObjectPtr->getSceneManager()->destroySceneNode(this->previewNode);
			this->previewNode = nullptr;
		}

		GameObjectComponent::onRemoveComponent();
	}

	void ProceduralRoadComponent::onOtherComponentRemoved(unsigned int index)
	{
	}

	void ProceduralRoadComponent::onOtherComponentAdded(unsigned int index)
	{
	}

	void ProceduralRoadComponent::update(Ogre::Real dt, bool notSimulating)
	{
		
	}

	void ProceduralRoadComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (ProceduralRoadComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (ProceduralRoadComponent::AttrRoadWidth() == attribute->getName())
		{
			this->setRoadWidth(attribute->getReal());
		}
		else if (ProceduralRoadComponent::AttrEdgeWidth() == attribute->getName())
		{
			this->setEdgeWidth(attribute->getReal());
		}
		else if (ProceduralRoadComponent::AttrRoadStyle() == attribute->getName())
		{
			this->setRoadStyle(attribute->getListSelectedValue());
		}
		else if (ProceduralRoadComponent::AttrSnapToGrid() == attribute->getName())
		{
			this->setSnapToGrid(attribute->getBool());
		}
		else if (ProceduralRoadComponent::AttrGridSize() == attribute->getName())
		{
			this->setGridSize(attribute->getReal());
		}
		else if (ProceduralRoadComponent::AttrAdaptToGround() == attribute->getName())
		{
			this->setAdaptToGround(attribute->getBool());
		}
		else if (ProceduralRoadComponent::AttrHeightOffset() == attribute->getName())
		{
			this->setHeightOffset(attribute->getReal());
		}
		else if (ProceduralRoadComponent::AttrMaxGradient() == attribute->getName())
		{
			this->setMaxGradient(attribute->getReal());
		}
		else if (ProceduralRoadComponent::AttrSmoothingFactor() == attribute->getName())
		{
			this->setSmoothingFactor(attribute->getReal());
		}
		else if (ProceduralRoadComponent::AttrEnableBanking() == attribute->getName())
		{
			this->setEnableBanking(attribute->getBool());
		}
		else if (ProceduralRoadComponent::AttrBankingAngle() == attribute->getName())
		{
			this->setBankingAngle(attribute->getReal());
		}
		else if (ProceduralRoadComponent::AttrCurveSubdivisions() == attribute->getName())
		{
			this->setCurveSubdivisions(attribute->getInt());
		}
		else if (ProceduralRoadComponent::AttrCenterDatablock() == attribute->getName())
		{
			this->setCenterDatablock(attribute->getString());
		}
		else if (ProceduralRoadComponent::AttrEdgeDatablock() == attribute->getName())
		{
			this->setEdgeDatablock(attribute->getString());
		}
		else if (ProceduralRoadComponent::AttrCenterUVTiling() == attribute->getName())
		{
			this->setCenterUVTiling(attribute->getVector2());
		}
		else if (ProceduralRoadComponent::AttrEdgeUVTiling() == attribute->getName())
		{
			this->setEdgeUVTiling(attribute->getVector2());
		}
		else if (ProceduralRoadComponent::AttrCurbHeight() == attribute->getName())
		{
			this->setCurbHeight(attribute->getReal());
		}
		else if (ProceduralRoadComponent::AttrTerrainSampleInterval() == attribute->getName())
		{
			this->setTerrainSampleInterval(attribute->getReal());
		}
	}

	void ProceduralRoadComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		GameObjectComponent::writeXML(propertiesXML, doc);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralRoadComponent::AttrActivated().c_str())));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralRoadComponent::AttrRoadWidth().c_str())));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->roadWidth->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralRoadComponent::AttrEdgeWidth().c_str())));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->edgeWidth->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralRoadComponent::AttrRoadStyle().c_str())));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->roadStyle->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralRoadComponent::AttrSnapToGrid().c_str())));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->snapToGrid->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralRoadComponent::AttrGridSize().c_str())));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->gridSize->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralRoadComponent::AttrAdaptToGround().c_str())));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->adaptToGround->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralRoadComponent::AttrHeightOffset().c_str())));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->heightOffset->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralRoadComponent::AttrMaxGradient().c_str())));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxGradient->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralRoadComponent::AttrSmoothingFactor().c_str())));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->smoothingFactor->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralRoadComponent::AttrEnableBanking().c_str())));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->enableBanking->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralRoadComponent::AttrBankingAngle().c_str())));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->bankingAngle->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralRoadComponent::AttrCurveSubdivisions().c_str())));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->curveSubdivisions->getInt())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralRoadComponent::AttrCenterDatablock().c_str())));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->centerDatablock->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralRoadComponent::AttrEdgeDatablock().c_str())));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->edgeDatablock->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralRoadComponent::AttrCenterUVTiling().c_str())));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->centerUVTiling->getVector2())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralRoadComponent::AttrEdgeUVTiling().c_str())));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->edgeUVTiling->getVector2())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralRoadComponent::AttrCurbHeight().c_str())));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->curbHeight->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralRoadComponent::AttrTerrainSampleInterval().c_str())));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->terrainSampleInterval->getReal())));
		propertiesXML->append_node(propertyXML);

		this->saveRoadDataToFile();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////

	bool ProceduralRoadComponent::canStaticAddComponent(GameObject* gameObject)
	{
		return false;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Input Handling
	///////////////////////////////////////////////////////////////////////////////////////////////

	bool ProceduralRoadComponent::mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
	{
		if (false == this->activated->getBool())
		{
			return true; // not handled -> bubble
		}

		// Check if we can modify
		if (false == this->canModify)
		{
			return true; // not handled -> bubble
		}

		if (id != OIS::MB_Left)
		{
			return true; // not handled -> bubble
		}

		// Check if clicking on GUI
		if (MyGUI::InputManager::getInstance().getMouseFocusWidget() != nullptr)
		{
			return true; // not handled -> bubble
		}

		Ogre::Camera* camera = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
		if (nullptr == camera)
		{
			return true; // not handled -> bubble
		}

		Ogre::Vector3 internalHitPoint = Ogre::Vector3::ZERO;
		Ogre::MovableObject* hitMovableObject = nullptr;
		Ogre::Real closestDistance = 0.0f;
		Ogre::Vector3 normal = Ogre::Vector3::ZERO;

		// Build exclusion list - exclude our own road and preview items
		std::vector<Ogre::MovableObject*> excludeMovableObjects;

		// Check if mouse hit already created road, -> then skip
		const OIS::MouseState& ms = NOWA::InputDeviceCore::getSingletonPtr()->getMouse()->getMouseState();
		bool hitFound = MathHelper::getInstance()->getRaycastFromPoint(ms.X.abs, ms.Y.abs, camera, Core::getSingletonPtr()->getOgreRenderWindow(), this->groundQuery, internalHitPoint, (size_t&)hitMovableObject, closestDistance, normal, &excludeMovableObjects, false);

		if (true == hitFound && this->buildState == BuildState::IDLE)
		{
			if (this->roadItem == hitMovableObject)
			{
				return true; // not handled -> bubble
			}
			if (this->previewItem == hitMovableObject)
			{
				return true; // not handled -> bubble
			}
		}

		// Get normalized screen coordinates
		Ogre::Real screenX = 0.0f;
		Ogre::Real screenY = 0.0f;
		MathHelper::getInstance()->mouseToViewPort(evt.state.X.abs, evt.state.Y.abs, screenX, screenY, Core::getSingletonPtr()->getOgreRenderWindow());

		// Raycast to ground
		Ogre::Vector3 hitPosition;

		if (this->raycastGround(screenX, screenY, hitPosition))
		{
			if (this->snapToGrid->getBool())
			{
				hitPosition = this->snapToGridFunc(hitPosition);
			}

			if (this->buildState == BuildState::IDLE)
			{
				// Start new road
				this->startRoadPlacement(hitPosition);
			}
			else if (this->buildState == BuildState::DRAGGING)
			{
				// Confirm current road
				this->confirmRoad();

				// NOTE: confirmRoad() handles starting next segment if shift is pressed
				// DON'T call startRoadPlacement() here - it's already handled in confirmRoad()
			}

			return false; // handled -> do not bubble
		}

		return false; // not handled -> bubble
	}

	bool ProceduralRoadComponent::mouseMoved(const OIS::MouseEvent& evt)
	{
		if (false == this->activated->getBool())
		{
			return true; // not handled -> bubble
		}

		// Only update preview if we're actively dragging
		if (this->buildState != BuildState::DRAGGING)
		{
			return true; // not handled -> bubble
		}

		// Also check if we can still modify
		if (false == this->canModify)
		{
			return true; // not handled -> bubble
		}

		// Get normalized screen coordinates
		Ogre::Real screenX = 0.0f;
		Ogre::Real screenY = 0.0f;
		MathHelper::getInstance()->mouseToViewPort(evt.state.X.abs, evt.state.Y.abs, screenX, screenY, Core::getSingletonPtr()->getOgreRenderWindow());

		// Raycast to ground
		Ogre::Vector3 hitPosition;
		if (this->raycastGround(screenX, screenY, hitPosition))
		{
			if (true == this->snapToGrid->getBool())
			{
				hitPosition = this->snapToGridFunc(hitPosition);
			}

			this->updateRoadPreview(hitPosition);
		}

		return true; // not handled -> bubble
	}

	bool ProceduralRoadComponent::mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
	{
		if (false == this->activated->getBool())
		{
			return true; // not handled -> bubble
		}

		if (id == OIS::MB_Right)
		{
			// Cancel current road segment in progress
			this->cancelRoad();

			// Remove listeners - user wants to stop building
			this->removeInputListener();

			return false;
		}

		return true; // not handled -> bubble
	}

	bool ProceduralRoadComponent::keyPressed(const OIS::KeyEvent& evt)
	{
		if (false == this->activated->getBool())
		{
			return true; // not handled -> bubble
		}

		if (evt.key == OIS::KC_LSHIFT || evt.key == OIS::KC_RSHIFT)
		{
			this->isShiftPressed = true;
			return false;
		}
		else if (evt.key == OIS::KC_LCONTROL || evt.key == OIS::KC_RCONTROL)
		{
			this->isCtrlPressed = true;
			return false;
		}
		else if (evt.key == OIS::KC_Z && this->isCtrlPressed)
		{
			this->removeLastSegment();
			return false;
		}
		else if (evt.key == OIS::KC_ESCAPE)
		{
			this->cancelRoad();
			this->removeInputListener();
			return false; // consume -> STOP DesignState ESC
		}

		return true; // not handled -> bubble
	}

	bool ProceduralRoadComponent::keyReleased(const OIS::KeyEvent& evt)
	{
		if (false == this->activated->getBool())
		{
			return true;
		}

		if (evt.key == OIS::KC_LSHIFT || evt.key == OIS::KC_RSHIFT)
		{
			this->isShiftPressed = false;
		}
		else if (evt.key == OIS::KC_LCONTROL || evt.key == OIS::KC_RCONTROL)
		{
			this->isCtrlPressed = false;
		}

		return false;  // handled -> do not bubble
	}

	///////////////////////////////////////////////////////////////////////////////////////////////

	void ProceduralRoadComponent::startRoadPlacement(const Ogre::Vector3& worldPosition)
	{
		Ogre::Vector3 startPos = this->snapToGrid->getBool() ? this->snapToGridFunc(worldPosition) : worldPosition;

		RoadControlPoint startPoint;
		startPoint.position = startPos;
		startPoint.position.y = 0.0f;  // XZ only, height is separate
		startPoint.groundHeight = this->getGroundHeight(startPos);
		startPoint.smoothedHeight = startPoint.groundHeight;
		startPoint.bankingAngle = 0.0f;
		startPoint.distFromStart = 0.0f;

		this->currentSegment.controlPoints.clear();
		this->currentSegment.controlPoints.push_back(startPoint);
		this->currentSegment.isCurved = false;
		this->currentSegment.curvature = 0.0f;

		if (false == this->hasRoadOrigin)
		{
			this->roadOrigin = startPoint.position;
			this->roadOrigin.y = startPoint.groundHeight;
			this->hasRoadOrigin = true;

			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
				"[ProceduralRoadComponent] Set road origin: " + Ogre::StringConverter::toString(this->roadOrigin));
		}

		this->buildState = BuildState::DRAGGING;
		this->lastValidPosition = startPoint.position;

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
			"[ProceduralRoadComponent] Started road placement at: " + Ogre::StringConverter::toString(startPos)
			+ ", ground height: " + Ogre::StringConverter::toString(startPoint.groundHeight));
	}

	void ProceduralRoadComponent::updateRoadPreview(const Ogre::Vector3& worldPosition)
	{
		if (true == this->currentSegment.controlPoints.empty())
		{
			return;
		}

		Ogre::Vector3 currentPos = this->snapToGrid->getBool() ? this->snapToGridFunc(worldPosition) : worldPosition;

		// Constrain to axis if ctrl is held (like wall component)
		if (true == this->isCtrlPressed)
		{
			Ogre::Vector3 delta = currentPos - this->currentSegment.controlPoints.front().position;
			if (std::abs(delta.x) > std::abs(delta.z))
			{
				currentPos.z = this->currentSegment.controlPoints.front().position.z;
			}
			else
			{
				currentPos.x = this->currentSegment.controlPoints.front().position.x;
			}
		}

		RoadControlPoint endPoint;
		endPoint.position = currentPos;
		endPoint.position.y = 0.0f;
		endPoint.groundHeight = this->getGroundHeight(currentPos);
		endPoint.smoothedHeight = endPoint.groundHeight;
		endPoint.bankingAngle = 0.0f;
		endPoint.distFromStart = this->currentSegment.controlPoints.front().position.distance(currentPos);

		// Update or add the end point
		if (this->currentSegment.controlPoints.size() == 1)
		{
			this->currentSegment.controlPoints.push_back(endPoint);
		}
		else
		{
			this->currentSegment.controlPoints.back() = endPoint;
		}

		this->lastValidPosition = currentPos;

		// Create visual preview
		this->updatePreviewMesh();
	}

	void ProceduralRoadComponent::confirmRoad(void)
	{
		if (this->buildState != BuildState::DRAGGING)
		{
			return;
		}

		if (this->currentSegment.controlPoints.size() < 2)
		{
			return;
		}

		// Don't add zero-length segments
		Ogre::Real length = this->currentSegment.controlPoints.front().position.distance(
			this->currentSegment.controlPoints.back().position);
		if (length < 0.1f)
		{
			this->cancelRoad();
			return;
		}

		// Apply height smoothing to this segment
		this->smoothHeightTransitions(this->currentSegment.controlPoints);

		// Add segment to the collection
		this->roadSegments.push_back(this->currentSegment);

		// Clean up preview BEFORE rebuilding final mesh
		this->destroyPreviewMesh();

		// Reset preview node position
		if (this->previewNode)
		{
			this->previewNode->setPosition(Ogre::Vector3::ZERO);
		}

		// Rebuild the entire road mesh
		this->rebuildMesh();

		// If shift is held, start next segment from END of current segment
		// (same pattern as wall component)
		if (true == this->isShiftPressed)
		{
			Ogre::Vector3 chainEndPos = this->currentSegment.controlPoints.back().position;
			this->startRoadPlacement(chainEndPos);
		}
		else
		{
			// Road building complete - remove listeners
			this->buildState = BuildState::IDLE;
			this->currentSegment.controlPoints.clear();
			this->removeInputListener();
		}

		boost::shared_ptr<NOWA::EventDataGeometryModified> eventDataGeometryModified(new NOWA::EventDataGeometryModified());
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataGeometryModified);

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
			"[ProceduralRoadComponent] Confirmed road segment, total segments: "
			+ Ogre::StringConverter::toString(this->roadSegments.size()));
	}

	void ProceduralRoadComponent::cancelRoad(void)
	{
		this->destroyPreviewMesh();
		this->buildState = BuildState::IDLE;
		this->currentSegment.controlPoints.clear();

		// Reset modifier key flags when canceling
		this->isShiftPressed = false;
		this->isCtrlPressed = false;

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralRoadComponent] Cancelled road placement");
	}

	void ProceduralRoadComponent::addRoadSegment(const std::vector<Ogre::Vector3>& controlPoints, bool curved)
	{
		if (controlPoints.size() < 2)
			return;

		RoadSegment segment;
		segment.isCurved = curved;
		segment.curvature = curved ? 0.5f : 0.0f;

		for (const auto& pos : controlPoints)
		{
			RoadControlPoint point;
			point.position = pos;
			point.position.y = 0.0f;
			point.groundHeight = this->getGroundHeight(pos);
			point.smoothedHeight = point.groundHeight;
			segment.controlPoints.push_back(point);
		}

		this->smoothHeightTransitions(segment.controlPoints);
		this->roadSegments.push_back(segment);
		this->rebuildMesh();
	}

	void ProceduralRoadComponent::removeLastSegment(void)
	{
		if (false == this->roadSegments.empty())
		{
			this->roadSegments.pop_back();
			this->rebuildMesh();
		}
	}

	void ProceduralRoadComponent::clearAllSegments(void)
	{
		this->roadSegments.clear();
		this->destroyRoadMesh();
		this->hasRoadOrigin = false;
	}

	void ProceduralRoadComponent::rebuildMesh(void)
	{
		this->centerVertices.clear();
		this->centerIndices.clear();
		this->currentCenterVertexIndex = 0;

		this->edgeVertices.clear();
		this->edgeIndices.clear();
		this->currentEdgeVertexIndex = 0;

		if (true == this->roadSegments.empty())
		{
			return;
		}

		Ogre::Vector3 originToUse = this->roadOrigin;

		// ---- Step 1: Detect connected chains ----
		// Segments sharing endpoints (from shift-chaining) form chains.
		// Within a chain, we collect all unique waypoints.
		std::vector<bool> processed(this->roadSegments.size(), false);

		for (size_t i = 0; i < this->roadSegments.size(); ++i)
		{
			if (processed[i])
				continue;

			// Start a new chain from segment i
			std::vector<size_t> chainIndices;
			chainIndices.push_back(i);
			processed[i] = true;

			// Look forward for connected segments
			size_t current = i;
			bool foundNext = true;
			while (foundNext)
			{
				foundNext = false;
				const Ogre::Vector3& lastEnd = this->roadSegments[current].controlPoints.back().position;

				for (size_t j = 0; j < this->roadSegments.size(); ++j)
				{
					if (processed[j])
						continue;

					const Ogre::Vector3& nextStart = this->roadSegments[j].controlPoints.front().position;
					if (lastEnd.squaredDistance(nextStart) < 0.01f)
					{
						chainIndices.push_back(j);
						processed[j] = true;
						current = j;
						foundNext = true;
						break;
					}
				}
			}

			// ---- Step 2: Collect waypoints from this chain ----
			std::vector<RoadControlPoint> chainWaypoints;

			for (size_t ci = 0; ci < chainIndices.size(); ++ci)
			{
				const RoadSegment& seg = this->roadSegments[chainIndices[ci]];

				for (size_t pi = 0; pi < seg.controlPoints.size(); ++pi)
				{
					// Skip duplicate shared endpoint between segments
					if (ci > 0 && pi == 0)
						continue;

					chainWaypoints.push_back(seg.controlPoints[pi]);
				}
			}

			// ---- Step 3: Build final path ----
			std::vector<RoadControlPoint> finalPath;

			if (chainWaypoints.size() >= 3)
			{
				// 3+ waypoints: generate smooth Catmull-Rom spline curve
				// This is what creates curves when user shift-chains segments at angles
				int subdivisions = this->curveSubdivisions->getInt();

				for (size_t pi = 0; pi < chainWaypoints.size() - 1; ++pi)
				{
					for (int j = 0; j < subdivisions; ++j)
					{
						Ogre::Real t = static_cast<Ogre::Real>(j) / subdivisions;
						Ogre::Vector3 pos = this->evaluateCatmullRom(chainWaypoints, static_cast<Ogre::Real>(pi) + t);

						RoadControlPoint cp;
						cp.position = pos;
						cp.position.y = 0.0f;
						// Sample terrain in WORLD space
						cp.groundHeight = this->getGroundHeight(pos);
						cp.smoothedHeight = cp.groundHeight;
						finalPath.push_back(cp);
					}
				}

				// Add final point
				RoadControlPoint lastCp = chainWaypoints.back();
				lastCp.position.y = 0.0f;
				finalPath.push_back(lastCp);
			}
			else
			{
				// 2 waypoints: subdivide for terrain following
				finalPath = this->subdivideForTerrain(chainWaypoints);
			}

			// ---- Step 4: Height smoothing (world space) ----
			this->smoothHeightTransitions(finalPath);

			// ---- Step 5: Banking for curves ----
			if (this->enableBanking->getBool())
			{
				for (size_t pi = 0; pi < finalPath.size(); ++pi)
				{
					if (pi > 0 && pi < finalPath.size() - 1)
					{
						finalPath[pi].bankingAngle = this->calculateBanking(
							finalPath[pi - 1].position, finalPath[pi].position, finalPath[pi + 1].position);
					}
					else
					{
						finalPath[pi].bankingAngle = 0.0f;
					}
				}
			}

			// ---- Step 6: Accumulated distance for UV mapping ----
			Ogre::Real accumDist = 0.0f;
			for (size_t pi = 0; pi < finalPath.size(); ++pi)
			{
				if (pi > 0)
				{
					accumDist += finalPath[pi].position.distance(finalPath[pi - 1].position);
				}
				finalPath[pi].distFromStart = accumDist;
			}

			// ---- Step 7: Transform to LOCAL space ----
			std::vector<RoadControlPoint> localPath;
			for (const auto& cp : finalPath)
			{
				RoadControlPoint lp;
				lp.position = cp.position - originToUse;
				lp.position.y = 0.0f;
				lp.groundHeight = cp.groundHeight - originToUse.y;
				lp.smoothedHeight = cp.smoothedHeight - originToUse.y;
				lp.bankingAngle = cp.bankingAngle;
				lp.distFromStart = cp.distFromStart;
				localPath.push_back(lp);
			}

			// ---- Step 8: Generate road geometry ----
			this->generateStraightRoad(localPath);
		}

		// Create the actual mesh
		if (this->currentCenterVertexIndex > 0 || this->currentEdgeVertexIndex > 0)
		{
			this->createRoadMesh();
		}
	}

	Ogre::Real ProceduralRoadComponent::getGroundHeight(const Ogre::Vector3& position)
	{
		if (false == this->adaptToGround->getBool())
		{
			return position.y;
		}

		Ogre::Vector3 rayOrigin = Ogre::Vector3(position.x, position.y + 1000.0f, position.z);
		Ogre::Ray downRay(rayOrigin, Ogre::Vector3::NEGATIVE_UNIT_Y);

		this->groundQuery->setRay(downRay);
		this->groundQuery->setSortByDistance(true);

		Ogre::Vector3 internalHitPoint = Ogre::Vector3::ZERO;
		Ogre::MovableObject* hitMovableObject = nullptr;
		Ogre::Real closestDistance = 0.0f;
		Ogre::Vector3 normal = Ogre::Vector3::ZERO;

		std::vector<Ogre::MovableObject*> excludeMovableObjects;
		if (this->roadItem)
		{
			excludeMovableObjects.emplace_back(this->roadItem);
		}
		if (this->previewItem)
		{
			excludeMovableObjects.emplace_back(this->previewItem);
		}

		bool hitFound = MathHelper::getInstance()->getRaycastFromPoint(this->groundQuery, AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera(), internalHitPoint, (size_t&)hitMovableObject,
			closestDistance, normal, &excludeMovableObjects, false);

		if (hitFound && hitMovableObject != nullptr)
		{
			return internalHitPoint.y + this->heightOffset->getReal();
		}

		std::pair<bool, Ogre::Real> planeResult = downRay.intersects(this->groundPlane);
		if (planeResult.first && planeResult.second > 0.0f)
		{
			Ogre::Real groundHeight = rayOrigin.y - planeResult.second;
			return groundHeight + this->heightOffset->getReal();
		}

		return position.y + this->heightOffset->getReal();
	}

	bool ProceduralRoadComponent::raycastGround(Ogre::Real screenX, Ogre::Real screenY, Ogre::Vector3& hitPosition)
	{
		Ogre::Camera* camera = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
		if (nullptr == camera)
		{
			return false;
		}

		const OIS::MouseState& ms = NOWA::InputDeviceCore::getSingletonPtr()->getMouse()->getMouseState();

		Ogre::Vector3 internalHitPoint = Ogre::Vector3::ZERO;
		Ogre::MovableObject* hitMovableObject = nullptr;
		Ogre::Real closestDistance = 0.0f;
		Ogre::Vector3 normal = Ogre::Vector3::ZERO;

		std::vector<Ogre::MovableObject*> excludeMovableObjects;
		if (this->roadItem)
		{
			excludeMovableObjects.emplace_back(this->roadItem);
		}
		if (this->previewItem)
		{
			excludeMovableObjects.emplace_back(this->previewItem);
		}

		bool hitFound = MathHelper::getInstance()->getRaycastFromPoint(ms.X.abs, ms.Y.abs, camera, Core::getSingletonPtr()->getOgreRenderWindow(), this->groundQuery, internalHitPoint, (size_t&)hitMovableObject, closestDistance, normal, &excludeMovableObjects, false);

		if (hitFound && hitMovableObject != nullptr)
		{
			hitPosition = internalHitPoint;
			return true;
		}

		Ogre::Ray ray = camera->getCameraToViewportRay(screenX, screenY);
		std::pair<bool, Ogre::Real> result = ray.intersects(this->groundPlane);
		if (result.first && result.second > 0.0f)
		{
			hitPosition = ray.getPoint(result.second);
			return true;
		}

		return false;
	}

	///////////////////////////////////////////////////////////////////////////
	// subdivideForTerrain - Inserts intermediate points along a path
	//      so the road follows terrain undulations
	///////////////////////////////////////////////////////////////////////////

	std::vector<ProceduralRoadComponent::RoadControlPoint> ProceduralRoadComponent::subdivideForTerrain(
		const std::vector<RoadControlPoint>& points)
	{
		if (points.size() < 2)
			return points;

		std::vector<RoadControlPoint> result;
		Ogre::Real interval = this->terrainSampleInterval->getReal();
		if (interval < 0.5f)
			interval = 0.5f;

		for (size_t i = 0; i < points.size() - 1; ++i)
		{
			const RoadControlPoint& p0 = points[i];
			const RoadControlPoint& p1 = points[i + 1];

			Ogre::Real segLength = p0.position.distance(p1.position);
			int numSamples = std::max(2, static_cast<int>(segLength / interval) + 1);

			for (int j = 0; j < numSamples; ++j)
			{
				// Skip last sample on non-final segments (next segment starts with it)
				if (i < points.size() - 2 && j == numSamples - 1)
					continue;

				Ogre::Real t = static_cast<Ogre::Real>(j) / (numSamples - 1);

				RoadControlPoint cp;
				cp.position = p0.position * (1.0f - t) + p1.position * t;
				cp.position.y = 0.0f;
				// Sample terrain in WORLD space
				cp.groundHeight = this->getGroundHeight(cp.position);
				cp.smoothedHeight = cp.groundHeight;
				cp.bankingAngle = 0.0f;
				cp.distFromStart = 0.0f; // Will be computed later
				result.push_back(cp);
			}
		}

		// Add final point
		RoadControlPoint lastCp;
		lastCp.position = points.back().position;
		lastCp.position.y = 0.0f;
		lastCp.groundHeight = points.back().groundHeight;
		lastCp.smoothedHeight = points.back().smoothedHeight;
		lastCp.bankingAngle = 0.0f;
		lastCp.distFromStart = 0.0f;
		result.push_back(lastCp);

		return result;
	}

	///////////////////////////////////////////////////////////////////////////
// generateStraightRoad - With curb geometry
//
// Cross-section (when curbHeight > 0):
//
//   outerL_top ---- curbL_top    cL ---- cR    curbR_top ---- outerR_top
//       |                |                        |                |
//   outerL_bot ----  (road edge)    CENTER    (road edge) ---- outerR_bot
//
//   Left outer   Left curb   Left inner  |  Right inner  Right curb  Right outer
//     wall       top surface    wall      |     wall     top surface    wall
//
// When curbHeight == 0: just flat edge strips (like original)
///////////////////////////////////////////////////////////////////////////

	void ProceduralRoadComponent::generateStraightRoad(const std::vector<RoadControlPoint>& points)
	{
		if (points.size() < 2)
			return;

		Ogre::Real halfWidth = this->roadWidth->getReal() * 0.5f;
		Ogre::Real edgeW = this->edgeWidth->getReal();
		Ogre::Real curbH = this->curbHeight->getReal();
		Ogre::Real totalHalfWidth = halfWidth + edgeW;

		Ogre::Vector2 centerUV = this->centerUVTiling->getVector2();
		Ogre::Vector2 edgeUV = this->edgeUVTiling->getVector2();

		for (size_t i = 0; i < points.size() - 1; ++i)
		{
			const RoadControlPoint& p0 = points[i];
			const RoadControlPoint& p1 = points[i + 1];

			// Direction and perpendicular
			Ogre::Vector3 dir = p1.position - p0.position;
			Ogre::Real segLength = dir.length();
			if (segLength < 0.001f)
				continue;

			dir.normalise();
			Ogre::Vector3 perp = Ogre::Vector3::UNIT_Y.crossProduct(dir);
			if (perp.squaredLength() < 0.001f)
			{
				perp = Ogre::Vector3::UNIT_X;
			}
			perp.normalise();

			// Banking
			Ogre::Quaternion bankRot0 = Ogre::Quaternion(Ogre::Radian(p0.bankingAngle), dir);
			Ogre::Quaternion bankRot1 = Ogre::Quaternion(Ogre::Radian(p1.bankingAngle), dir);

			// Road surface height at start and end
			Ogre::Vector3 base0 = p0.position + Ogre::Vector3(0, p0.smoothedHeight, 0);
			Ogre::Vector3 base1 = p1.position + Ogre::Vector3(0, p1.smoothedHeight, 0);

			// Center road edge vertices
			Ogre::Vector3 cL0 = base0 + bankRot0 * (perp * -halfWidth);
			Ogre::Vector3 cR0 = base0 + bankRot0 * (perp * halfWidth);
			Ogre::Vector3 cL1 = base1 + bankRot1 * (perp * -halfWidth);
			Ogre::Vector3 cR1 = base1 + bankRot1 * (perp * halfWidth);

			// UV coordinates - V accumulates along road length for continuous texturing
			Ogre::Real v0 = p0.distFromStart / std::max(centerUV.y, 0.001f);
			Ogre::Real v1 = p1.distFromStart / std::max(centerUV.y, 0.001f);
			Ogre::Real ev0 = p0.distFromStart / std::max(edgeUV.y, 0.001f);
			Ogre::Real ev1 = p1.distFromStart / std::max(edgeUV.y, 0.001f);

			// ==== CENTER ROAD SURFACE ====
			this->addRoadQuad(cL0, cR0, cR1, cL1,
				Ogre::Vector3::UNIT_Y, 0.0f, 1.0f, v0, v1, true);

			// ==== EDGE GEOMETRY ====
			if (curbH > 0.001f)
			{
				// -- Curb top inner edge vertices (at road edge, raised) --
				Ogre::Vector3 curbL0_top = cL0 + Ogre::Vector3(0, curbH, 0);
				Ogre::Vector3 curbR0_top = cR0 + Ogre::Vector3(0, curbH, 0);
				Ogre::Vector3 curbL1_top = cL1 + Ogre::Vector3(0, curbH, 0);
				Ogre::Vector3 curbR1_top = cR1 + Ogre::Vector3(0, curbH, 0);

				// -- Curb top outer edge vertices --
				Ogre::Vector3 eL0_top = base0 + bankRot0 * (perp * -totalHalfWidth) + Ogre::Vector3(0, curbH, 0);
				Ogre::Vector3 eR0_top = base0 + bankRot0 * (perp * totalHalfWidth) + Ogre::Vector3(0, curbH, 0);
				Ogre::Vector3 eL1_top = base1 + bankRot1 * (perp * -totalHalfWidth) + Ogre::Vector3(0, curbH, 0);
				Ogre::Vector3 eR1_top = base1 + bankRot1 * (perp * totalHalfWidth) + Ogre::Vector3(0, curbH, 0);

				// -- Curb bottom outer vertices (ground level at outer edge) --
				Ogre::Vector3 eL0_bot = base0 + bankRot0 * (perp * -totalHalfWidth);
				Ogre::Vector3 eR0_bot = base0 + bankRot0 * (perp * totalHalfWidth);
				Ogre::Vector3 eL1_bot = base1 + bankRot1 * (perp * -totalHalfWidth);
				Ogre::Vector3 eR1_bot = base1 + bankRot1 * (perp * totalHalfWidth);

				// LEFT SIDE:
				// Inner curb wall (vertical face between road edge and curb top, faces RIGHT toward road)
				this->addRoadQuad(cL0, curbL0_top, curbL1_top, cL1,
					perp, 0.0f, curbH, ev0, ev1, false);

				// Curb top surface (horizontal, faces UP)
				this->addRoadQuad(curbL0_top, eL0_top, eL1_top, curbL1_top,
					Ogre::Vector3::UNIT_Y, 0.0f, 1.0f, ev0, ev1, false);

				// Outer curb wall (vertical face on outside, faces LEFT away from road)
				this->addRoadQuad(eL0_top, eL0_bot, eL1_bot, eL1_top,
					-perp, 0.0f, curbH, ev0, ev1, false);

				// RIGHT SIDE:
				// Inner curb wall (faces LEFT toward road)
				this->addRoadQuad(curbR0_top, cR0, cR1, curbR1_top,
					-perp, 0.0f, curbH, ev0, ev1, false);

				// Curb top surface (faces UP)
				this->addRoadQuad(eR0_top, curbR0_top, curbR1_top, eR1_top,
					Ogre::Vector3::UNIT_Y, 0.0f, 1.0f, ev0, ev1, false);

				// Outer curb wall (faces RIGHT away from road)
				this->addRoadQuad(eR0_bot, eR0_top, eR1_top, eR1_bot,
					perp, 0.0f, curbH, ev0, ev1, false);
			}
			else
			{
				// No curb height: flat edge strips (original behavior)
				Ogre::Vector3 flatEL0 = base0 + bankRot0 * (perp * -totalHalfWidth);
				Ogre::Vector3 flatEL1 = base1 + bankRot1 * (perp * -totalHalfWidth);
				Ogre::Vector3 flatER0 = base0 + bankRot0 * (perp * totalHalfWidth);
				Ogre::Vector3 flatER1 = base1 + bankRot1 * (perp * totalHalfWidth);

				this->addRoadQuad(flatEL0, cL0, cL1, flatEL1,
					Ogre::Vector3::UNIT_Y, 0.0f, 1.0f, ev0, ev1, false);
				this->addRoadQuad(cR0, flatER0, flatER1, cR1,
					Ogre::Vector3::UNIT_Y, 0.0f, 1.0f, ev0, ev1, false);
			}
		}
	}

	///////////////////////////////////////////////////////////////////////////
	// CHANGED: addRoadQuad - FIXED winding order + per-face normal computation
	///////////////////////////////////////////////////////////////////////////

	void ProceduralRoadComponent::addRoadQuad(const Ogre::Vector3& v0, const Ogre::Vector3& v1,
		const Ogre::Vector3& v2, const Ogre::Vector3& v3,
		const Ogre::Vector3& normal, Ogre::Real u0, Ogre::Real u1,
		Ogre::Real v0Val, Ogre::Real v1Val, bool isCenter)
	{
		std::vector<float>& verts = isCenter ? this->centerVertices : this->edgeVertices;
		std::vector<Ogre::uint32>& inds = isCenter ? this->centerIndices : this->edgeIndices;
		Ogre::uint32& currentIdx = isCenter ? this->currentCenterVertexIndex : this->currentEdgeVertexIndex;

		// Compute face normal from default winding (0-1-2)
		Ogre::Vector3 edge1 = v1 - v0;
		Ogre::Vector3 edge2 = v2 - v0;
		Ogre::Vector3 triNormal = edge1.crossProduct(edge2);

		// Determine if we need to flip winding
		// If triNormal agrees with hint normal -> default winding is correct
		// If triNormal opposes hint normal -> flip winding
		bool flipWinding = false;
		if (triNormal.squaredLength() > 0.0001f)
		{
			triNormal.normalise();
			if (triNormal.dotProduct(normal) < 0.0f)
			{
				flipWinding = true;
				triNormal = -triNormal; // Flip so we store the correct normal
			}
		}
		else
		{
			triNormal = normal; // Degenerate quad, use hint
		}

		// Vertex format: position (3) + normal (3) + UV (2) = 8 floats
		auto addVertex = [&](const Ogre::Vector3& pos, Ogre::Real u, Ogre::Real v)
			{
				verts.push_back(pos.x);
				verts.push_back(pos.y);
				verts.push_back(pos.z);
				verts.push_back(triNormal.x);
				verts.push_back(triNormal.y);
				verts.push_back(triNormal.z);
				verts.push_back(u);
				verts.push_back(v);
			};

		Ogre::uint32 baseIdx = currentIdx;
		addVertex(v0, u0, v0Val);
		addVertex(v1, u1, v0Val);
		addVertex(v2, u1, v1Val);
		addVertex(v3, u0, v1Val);

		if (false == flipWinding)
		{
			// Default winding (CCW in this vertex layout)
			inds.push_back(baseIdx + 0);
			inds.push_back(baseIdx + 1);
			inds.push_back(baseIdx + 2);

			inds.push_back(baseIdx + 0);
			inds.push_back(baseIdx + 2);
			inds.push_back(baseIdx + 3);
		}
		else
		{
			// Flipped winding
			inds.push_back(baseIdx + 0);
			inds.push_back(baseIdx + 2);
			inds.push_back(baseIdx + 1);

			inds.push_back(baseIdx + 0);
			inds.push_back(baseIdx + 3);
			inds.push_back(baseIdx + 2);
		}

		currentIdx += 4;
	}

	void ProceduralRoadComponent::generateCurvedRoad(const std::vector<RoadControlPoint>& points, Ogre::Real curvature)
	{
		if (points.size() < 2)
			return;

		// Generate smooth curve points using spline interpolation
		int subdivisions = this->curveSubdivisions->getInt();
		std::vector<Ogre::Vector3> curvePoints = this->generateCurvePoints(points, subdivisions);

		// Create control points for the curved path with terrain sampling in WORLD space
		std::vector<RoadControlPoint> curvedControlPoints;
		for (const auto& pos : curvePoints)
		{
			RoadControlPoint cp;
			cp.position = pos;
			cp.position.y = 0.0f;
			// NOTE: Heights should already be computed before local-space transform.
			// If called after transform, these heights will be wrong.
			// The improved rebuildMesh handles this correctly by pre-computing heights.
			cp.groundHeight = pos.y; // Use pre-computed Y if available
			cp.smoothedHeight = cp.groundHeight;
			curvedControlPoints.push_back(cp);
		}

		this->smoothHeightTransitions(curvedControlPoints);
		this->generateStraightRoad(curvedControlPoints);
	}

	///////////////////////////////////////////////////////////////////////////
	// generateRoadSegment, generateCurvePoints, evaluateCatmullRom -
	//   UNCHANGED from original (kept for compatibility)
	///////////////////////////////////////////////////////////////////////////

	void ProceduralRoadComponent::generateRoadSegment(const RoadSegment& segment)
	{
		if (segment.controlPoints.size() < 2)
			return;

		if (segment.isCurved)
		{
			this->generateCurvedRoad(segment.controlPoints, segment.curvature);
		}
		else
		{
			this->generateStraightRoad(segment.controlPoints);
		}
	}

	std::vector<Ogre::Vector3> ProceduralRoadComponent::generateCurvePoints(const std::vector<RoadControlPoint>& controlPoints, int subdivisions)
	{
		std::vector<Ogre::Vector3> result;

		if (controlPoints.size() < 2)
			return result;

		if (controlPoints.size() == 2)
		{
			for (int i = 0; i <= subdivisions; ++i)
			{
				Ogre::Real t = static_cast<Ogre::Real>(i) / subdivisions;
				result.push_back(controlPoints[0].position * (1.0f - t) + controlPoints[1].position * t);
			}
			return result;
		}

		for (size_t i = 0; i < controlPoints.size() - 1; ++i)
		{
			for (int j = 0; j < subdivisions; ++j)
			{
				Ogre::Real t = static_cast<Ogre::Real>(j) / subdivisions;
				Ogre::Vector3 point = this->evaluateCatmullRom(controlPoints, i + t);
				result.push_back(point);
			}
		}

		result.push_back(controlPoints.back().position);
		return result;
	}

	Ogre::Vector3 ProceduralRoadComponent::evaluateCatmullRom(const std::vector<RoadControlPoint>& points, Ogre::Real t)
	{
		int segment = static_cast<int>(t);
		Ogre::Real localT = t - segment;

		int p0 = Ogre::Math::Clamp(segment - 1, 0, static_cast<int>(points.size()) - 1);
		int p1 = Ogre::Math::Clamp(segment, 0, static_cast<int>(points.size()) - 1);
		int p2 = Ogre::Math::Clamp(segment + 1, 0, static_cast<int>(points.size()) - 1);
		int p3 = Ogre::Math::Clamp(segment + 2, 0, static_cast<int>(points.size()) - 1);

		Ogre::Vector3 v0 = points[p0].position;
		Ogre::Vector3 v1 = points[p1].position;
		Ogre::Vector3 v2 = points[p2].position;
		Ogre::Vector3 v3 = points[p3].position;

		Ogre::Real t2 = localT * localT;
		Ogre::Real t3 = t2 * localT;

		Ogre::Vector3 result = 0.5f * ((2.0f * v1) +
			(-v0 + v2) * localT +
			(2.0f * v0 - 5.0f * v1 + 4.0f * v2 - v3) * t2 +
			(-v0 + 3.0f * v1 - 3.0f * v2 + v3) * t3);

		return result;
	}

	///////////////////////////////////////////////////////////////////////////
// smoothHeightTransitions - Wider kernel, bidirectional gradient clamping
///////////////////////////////////////////////////////////////////////////

	void ProceduralRoadComponent::smoothHeightTransitions(std::vector<RoadControlPoint>& points)
	{
		if (points.size() < 3)
			return;

		Ogre::Real smoothing = this->smoothingFactor->getReal();
		Ogre::Real maxGrad = Ogre::Math::Tan(Ogre::Math::DegreesToRadians(this->maxGradient->getReal()));

		// ---- Gaussian-weighted smoothing passes ----
		int smoothPasses = static_cast<int>(smoothing * 8.0f) + 1;

		for (int pass = 0; pass < smoothPasses; ++pass)
		{
			std::vector<Ogre::Real> newHeights(points.size());

			for (size_t i = 0; i < points.size(); ++i)
			{
				if (i == 0 || i == points.size() - 1)
				{
					// Endpoints stay pinned to terrain
					newHeights[i] = points[i].groundHeight;
					continue;
				}

				// 5-point weighted average
				const Ogre::Real weights[5] = { 0.1f, 0.2f, 0.4f, 0.2f, 0.1f };
				Ogre::Real weightSum = 0.0f;
				Ogre::Real heightSum = 0.0f;

				for (int k = -2; k <= 2; ++k)
				{
					int idx = Ogre::Math::Clamp(static_cast<int>(i) + k, 0, static_cast<int>(points.size()) - 1);
					Ogre::Real w = weights[k + 2];
					heightSum += points[idx].smoothedHeight * w;
					weightSum += w;
				}

				newHeights[i] = Ogre::Math::lerp(points[i].groundHeight, heightSum / weightSum, smoothing);
			}

			for (size_t i = 1; i < points.size() - 1; ++i)
			{
				points[i].smoothedHeight = newHeights[i];
			}
		}

		// ---- Forward gradient clamping ----
		for (size_t i = 1; i < points.size(); ++i)
		{
			Ogre::Vector3 dir = points[i].position - points[i - 1].position;
			Ogre::Real horizontalDist = Ogre::Vector2(dir.x, dir.z).length();
			if (horizontalDist < 0.001f)
				continue;

			Ogre::Real heightDiff = points[i].smoothedHeight - points[i - 1].smoothedHeight;
			Ogre::Real maxHeightDiff = horizontalDist * maxGrad;

			if (heightDiff > maxHeightDiff)
				points[i].smoothedHeight = points[i - 1].smoothedHeight + maxHeightDiff;
			else if (heightDiff < -maxHeightDiff)
				points[i].smoothedHeight = points[i - 1].smoothedHeight - maxHeightDiff;
		}

		// ---- Backward gradient clamping (prevents kinks) ----
		for (int i = static_cast<int>(points.size()) - 2; i >= 0; --i)
		{
			Ogre::Vector3 dir = points[i + 1].position - points[i].position;
			Ogre::Real horizontalDist = Ogre::Vector2(dir.x, dir.z).length();
			if (horizontalDist < 0.001f)
				continue;

			Ogre::Real heightDiff = points[i].smoothedHeight - points[i + 1].smoothedHeight;
			Ogre::Real maxHeightDiff = horizontalDist * maxGrad;

			if (heightDiff > maxHeightDiff)
				points[i].smoothedHeight = points[i + 1].smoothedHeight + maxHeightDiff;
			else if (heightDiff < -maxHeightDiff)
				points[i].smoothedHeight = points[i + 1].smoothedHeight - maxHeightDiff;
		}
	}

	Ogre::Real ProceduralRoadComponent::calculateSmoothedHeight(const std::vector<RoadControlPoint>& points, int index)
	{
		if (index == 0 || index == static_cast<int>(points.size()) - 1)
		{
			return points[index].groundHeight;
		}

		Ogre::Real sum = points[index].groundHeight;
		int count = 1;

		if (index > 0)
		{
			sum += points[index - 1].groundHeight;
			count++;
		}
		if (index < static_cast<int>(points.size()) - 1)
		{
			sum += points[index + 1].groundHeight;
			count++;
		}

		return sum / count;
	}

	Ogre::Real ProceduralRoadComponent::calculateBanking(const Ogre::Vector3& p0, const Ogre::Vector3& p1, const Ogre::Vector3& p2)
	{
		Ogre::Vector3 dir1 = (p1 - p0).normalisedCopy();
		Ogre::Vector3 dir2 = (p2 - p1).normalisedCopy();

		Ogre::Real dot = Ogre::Math::Clamp(dir1.dotProduct(dir2), -1.0f, 1.0f);
		Ogre::Real angle = Ogre::Math::ACos(dot).valueDegrees();

		Ogre::Vector3 cross = dir1.crossProduct(dir2);
		Ogre::Real sign = cross.y > 0 ? 1.0f : -1.0f;

		Ogre::Real maxBank = Ogre::Math::DegreesToRadians(this->bankingAngle->getReal());
		Ogre::Real banking = sign * maxBank * (angle / 90.0f);

		return Ogre::Math::Clamp(banking, -maxBank, maxBank);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Mesh Creation
	///////////////////////////////////////////////////////////////////////////////////////////////

	bool ProceduralRoadComponent::exportMesh(const Ogre::String& filename)
	{
		if (nullptr == this->roadMesh)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralRoadComponent] No mesh to export!");
			return false;
		}

		try
		{
			Ogre::MeshSerializer serializer(Ogre::Root::getSingletonPtr()->getRenderSystem()->getVaoManager());
			serializer.exportMesh(this->roadMesh.get(), filename);
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralRoadComponent] Exported mesh to: " + filename);
			return true;
		}
		catch (Ogre::Exception& e)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralRoadComponent] Export failed: " + e.getFullDescription());
			return false;
		}
	}

	void ProceduralRoadComponent::createRoadMesh(void)
	{
		this->destroyRoadMesh();

		if (this->currentCenterVertexIndex == 0 && this->currentEdgeVertexIndex == 0)
		{
			return;
		}

		// ---- CACHE CPU DATA HERE (before anything goes to GPU) ----
		this->cachedCenterVertices = this->centerVertices;
		this->cachedCenterIndices = this->centerIndices;
		this->cachedNumCenterVertices = this->currentCenterVertexIndex;

		this->cachedEdgeVertices = this->edgeVertices;
		this->cachedEdgeIndices = this->edgeIndices;
		this->cachedNumEdgeVertices = this->currentEdgeVertexIndex;

		this->cachedRoadOrigin = this->roadOrigin;

		// Create Ogre mesh - copy both center and edge data
		std::vector<float> centerVerticesCopy = this->centerVertices;
		std::vector<Ogre::uint32> centerIndicesCopy = this->centerIndices;
		size_t numCenterVertices = this->currentCenterVertexIndex;

		std::vector<float> edgeVerticesCopy = this->edgeVertices;
		std::vector<Ogre::uint32> edgeIndicesCopy = this->edgeIndices;
		size_t numEdgeVertices = this->currentEdgeVertexIndex;

		Ogre::Vector3 roadOriginCopy = this->roadOrigin;

		// Execute mesh creation on render thread
		GraphicsModule::RenderCommand renderCommand = [this, centerVerticesCopy, centerIndicesCopy, numCenterVertices, edgeVerticesCopy, edgeIndicesCopy, numEdgeVertices, roadOriginCopy]()
		{
			this->createRoadMeshInternal(centerVerticesCopy, centerIndicesCopy, numCenterVertices, edgeVerticesCopy, edgeIndicesCopy, numEdgeVertices, roadOriginCopy);
		};
		NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralRoadComponent::createRoadMesh");

		if (nullptr == this->roadMesh || nullptr == this->roadItem)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralRoadComponent] No mesh/item to debug");
			return;
		}

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralRoadComponent] === MESH DEBUG INFO ===");
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "Mesh name: " + this->roadMesh->getName());
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "Num submeshes: " + Ogre::StringConverter::toString(this->roadMesh->getNumSubMeshes()));
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "Num subitems: " + Ogre::StringConverter::toString(this->roadItem->getNumSubItems()));

		for (size_t i = 0; i < this->roadItem->getNumSubItems(); ++i)
		{
			Ogre::SubItem* subItem = this->roadItem->getSubItem(i);
			Ogre::HlmsDatablock* db = subItem->getDatablock();
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "SubItem " + Ogre::StringConverter::toString(i) + " datablock: " + (db ? db->getName().getFriendlyText() : "NULL"));
		}

		// Clear temporary data
		this->centerVertices.clear();
		this->centerIndices.clear();
		this->edgeVertices.clear();
		this->edgeIndices.clear();
	}

	void ProceduralRoadComponent::createRoadMeshInternal(const std::vector<float>& centerVerts, const std::vector<Ogre::uint32>& centerInds, size_t numCenterVerts,
		const std::vector<float>& edgeVerts, const std::vector<Ogre::uint32>& edgeInds, size_t numEdgeVerts, const Ogre::Vector3& origin)
	{
		Ogre::Root* root = Ogre::Root::getSingletonPtr();
		Ogre::RenderSystem* renderSystem = root->getRenderSystem();
		Ogre::VaoManager* vaoManager = renderSystem->getVaoManager();

		Ogre::String meshName = this->gameObjectPtr->getName() + "_Road_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
		const Ogre::String groupName = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;

		// Remove existing mesh
		{
			Ogre::MeshManager& meshMgr = Ogre::MeshManager::getSingleton();
			Ogre::MeshPtr existing = meshMgr.getByName(meshName, groupName);
			if (false == existing.isNull())
			{
				meshMgr.remove(existing->getHandle());
			}
		}

		this->roadMesh = Ogre::MeshManager::getSingleton().createManual(meshName, groupName);

		// Vertex elements with tangents for normal mapping
		Ogre::VertexElement2Vec vertexElements;
		vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_POSITION));
		vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_NORMAL));
		vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT4, Ogre::VES_TANGENT));
		vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES));

		// Current vertex data has: pos(3) + normal(3) + uv(2) = 8 floats
		// We need to add tangent(4), so 12 floats per vertex
		const size_t srcFloatsPerVertex = 8;
		const size_t dstFloatsPerVertex = 12;

		Ogre::Vector3 minBounds(std::numeric_limits<float>::max());
		Ogre::Vector3 maxBounds(std::numeric_limits<float>::lowest());

		// ==================== SUBMESH 0: CENTER ====================
		// ALWAYS create center submesh, even if empty
		Ogre::SubMesh* centerSubMesh = this->roadMesh->createSubMesh();

		if (numCenterVerts > 0)
		{
			// Allocate and convert vertex data
			const size_t centerVertexDataSize = numCenterVerts * dstFloatsPerVertex * sizeof(float);
			float* centerVertexData = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(centerVertexDataSize, Ogre::MEMCATEGORY_GEOMETRY));

			for (size_t i = 0; i < numCenterVerts; ++i)
			{
				size_t srcOffset = i * srcFloatsPerVertex;
				size_t dstOffset = i * dstFloatsPerVertex;

				// Position
				centerVertexData[dstOffset + 0] = centerVerts[srcOffset + 0];
				centerVertexData[dstOffset + 1] = centerVerts[srcOffset + 1];
				centerVertexData[dstOffset + 2] = centerVerts[srcOffset + 2];

				// Update bounds
				Ogre::Vector3 pos(centerVerts[srcOffset + 0], centerVerts[srcOffset + 1], centerVerts[srcOffset + 2]);
				minBounds.makeFloor(pos);
				maxBounds.makeCeil(pos);

				// Normal
				Ogre::Vector3 normal(centerVerts[srcOffset + 3], centerVerts[srcOffset + 4], centerVerts[srcOffset + 5]);
				centerVertexData[dstOffset + 3] = normal.x;
				centerVertexData[dstOffset + 4] = normal.y;
				centerVertexData[dstOffset + 5] = normal.z;

				// Calculate tangent
				Ogre::Vector3 tangent;
				if (std::abs(normal.y) < 0.9f)
				{
					tangent = Ogre::Vector3::UNIT_Y.crossProduct(normal);
				}
				else
				{
					tangent = normal.crossProduct(Ogre::Vector3::UNIT_X);
				}
				tangent.normalise();

				centerVertexData[dstOffset + 6] = tangent.x;
				centerVertexData[dstOffset + 7] = tangent.y;
				centerVertexData[dstOffset + 8] = tangent.z;
				centerVertexData[dstOffset + 9] = 1.0f;

				// UV
				centerVertexData[dstOffset + 10] = centerVerts[srcOffset + 6];
				centerVertexData[dstOffset + 11] = centerVerts[srcOffset + 7];
			}

			// Create vertex buffer
			Ogre::VertexBufferPacked* centerVertexBuffer = nullptr;
			try
			{
				centerVertexBuffer = vaoManager->createVertexBuffer(vertexElements, numCenterVerts, Ogre::BT_IMMUTABLE, centerVertexData, true);
			}
			catch (Ogre::Exception& e)
			{
				OGRE_FREE_SIMD(centerVertexData, Ogre::MEMCATEGORY_GEOMETRY);
				throw e;
			}

			// Allocate index data
			const size_t centerIndexDataSize = centerInds.size() * sizeof(Ogre::uint32);
			Ogre::uint32* centerIndexData = reinterpret_cast<Ogre::uint32*>(OGRE_MALLOC_SIMD(centerIndexDataSize, Ogre::MEMCATEGORY_GEOMETRY));
			memcpy(centerIndexData, centerInds.data(), centerIndexDataSize);

			// Create index buffer
			Ogre::IndexBufferPacked* centerIndexBuffer = nullptr;
			try
			{
				centerIndexBuffer = vaoManager->createIndexBuffer(Ogre::IndexBufferPacked::IT_32BIT, centerInds.size(), Ogre::BT_IMMUTABLE, centerIndexData, true);
			}
			catch (Ogre::Exception& e)
			{
				OGRE_FREE_SIMD(centerIndexData, Ogre::MEMCATEGORY_GEOMETRY);
				throw e;
			}

			// Create VAO
			Ogre::VertexBufferPackedVec centerVertexBuffers;
			centerVertexBuffers.push_back(centerVertexBuffer);

			Ogre::VertexArrayObject* centerVao = vaoManager->createVertexArrayObject(centerVertexBuffers, centerIndexBuffer, Ogre::OT_TRIANGLE_LIST);

			centerSubMesh->mVao[Ogre::VpNormal].push_back(centerVao);
			centerSubMesh->mVao[Ogre::VpShadow].push_back(centerVao);
		}
		else
		{
			// Create empty dummy VAO for center submesh
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralRoadComponent] Creating empty center submesh");

			// Create minimal dummy vertex buffer (1 vertex)
			const size_t dummyVertexDataSize = 1 * dstFloatsPerVertex * sizeof(float);
			float* dummyVertexData = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(dummyVertexDataSize, Ogre::MEMCATEGORY_GEOMETRY));

			// Position (0,0,0)
			dummyVertexData[0] = 0.0f;
			dummyVertexData[1] = 0.0f;
			dummyVertexData[2] = 0.0f;

			// Normal (0,1,0) - pointing up
			dummyVertexData[3] = 0.0f;
			dummyVertexData[4] = 1.0f;
			dummyVertexData[5] = 0.0f;

			// Tangent (1,0,0,1) - pointing along X axis with handedness 1
			dummyVertexData[6] = 1.0f;
			dummyVertexData[7] = 0.0f;
			dummyVertexData[8] = 0.0f;
			dummyVertexData[9] = 1.0f;

			// UV (0,0)
			dummyVertexData[10] = 0.0f;
			dummyVertexData[11] = 0.0f;

			Ogre::VertexBufferPacked* dummyVertexBuffer = vaoManager->createVertexBuffer(vertexElements, 1, Ogre::BT_IMMUTABLE, dummyVertexData, true);

			// Create dummy index buffer (empty - 0 indices)
			Ogre::uint32* dummyIndexData = reinterpret_cast<Ogre::uint32*>(OGRE_MALLOC_SIMD(sizeof(Ogre::uint32), Ogre::MEMCATEGORY_GEOMETRY));
			dummyIndexData[0] = 0;

			Ogre::IndexBufferPacked* dummyIndexBuffer = vaoManager->createIndexBuffer(Ogre::IndexBufferPacked::IT_32BIT, 0, Ogre::BT_IMMUTABLE, dummyIndexData, true);

			Ogre::VertexBufferPackedVec dummyVertexBuffers;
			dummyVertexBuffers.push_back(dummyVertexBuffer);

			Ogre::VertexArrayObject* dummyVao = vaoManager->createVertexArrayObject(dummyVertexBuffers, dummyIndexBuffer, Ogre::OT_TRIANGLE_LIST);

			centerSubMesh->mVao[Ogre::VpNormal].push_back(dummyVao);
			centerSubMesh->mVao[Ogre::VpShadow].push_back(dummyVao);
		}

		// ==================== SUBMESH 1: EDGES ====================
		// ALWAYS create edge submesh, even if empty
		Ogre::SubMesh* edgeSubMesh = this->roadMesh->createSubMesh();

		if (numEdgeVerts > 0)
		{
			// Allocate and convert vertex data
			const size_t edgeVertexDataSize = numEdgeVerts * dstFloatsPerVertex * sizeof(float);
			float* edgeVertexData = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(edgeVertexDataSize, Ogre::MEMCATEGORY_GEOMETRY));

			for (size_t i = 0; i < numEdgeVerts; ++i)
			{
				size_t srcOffset = i * srcFloatsPerVertex;
				size_t dstOffset = i * dstFloatsPerVertex;

				// Position
				edgeVertexData[dstOffset + 0] = edgeVerts[srcOffset + 0];
				edgeVertexData[dstOffset + 1] = edgeVerts[srcOffset + 1];
				edgeVertexData[dstOffset + 2] = edgeVerts[srcOffset + 2];

				// Update bounds
				Ogre::Vector3 pos(edgeVerts[srcOffset + 0], edgeVerts[srcOffset + 1], edgeVerts[srcOffset + 2]);
				minBounds.makeFloor(pos);
				maxBounds.makeCeil(pos);

				// Normal
				Ogre::Vector3 normal(edgeVerts[srcOffset + 3], edgeVerts[srcOffset + 4], edgeVerts[srcOffset + 5]);
				edgeVertexData[dstOffset + 3] = normal.x;
				edgeVertexData[dstOffset + 4] = normal.y;
				edgeVertexData[dstOffset + 5] = normal.z;

				// Calculate tangent
				Ogre::Vector3 tangent;
				if (std::abs(normal.y) < 0.9f)
				{
					tangent = Ogre::Vector3::UNIT_Y.crossProduct(normal);
				}
				else
				{
					tangent = normal.crossProduct(Ogre::Vector3::UNIT_X);
				}
				tangent.normalise();

				edgeVertexData[dstOffset + 6] = tangent.x;
				edgeVertexData[dstOffset + 7] = tangent.y;
				edgeVertexData[dstOffset + 8] = tangent.z;
				edgeVertexData[dstOffset + 9] = 1.0f;

				// UV
				edgeVertexData[dstOffset + 10] = edgeVerts[srcOffset + 6];
				edgeVertexData[dstOffset + 11] = edgeVerts[srcOffset + 7];
			}

			// Create vertex buffer
			Ogre::VertexBufferPacked* edgeVertexBuffer = nullptr;
			try
			{
				edgeVertexBuffer = vaoManager->createVertexBuffer(vertexElements, numEdgeVerts, Ogre::BT_IMMUTABLE, edgeVertexData, true);
			}
			catch (Ogre::Exception& e)
			{
				OGRE_FREE_SIMD(edgeVertexData, Ogre::MEMCATEGORY_GEOMETRY);
				throw e;
			}

			// Allocate index data
			const size_t edgeIndexDataSize = edgeInds.size() * sizeof(Ogre::uint32);
			Ogre::uint32* edgeIndexData = reinterpret_cast<Ogre::uint32*>(OGRE_MALLOC_SIMD(edgeIndexDataSize, Ogre::MEMCATEGORY_GEOMETRY));
			memcpy(edgeIndexData, edgeInds.data(), edgeIndexDataSize);

			// Create index buffer
			Ogre::IndexBufferPacked* edgeIndexBuffer = nullptr;
			try
			{
				edgeIndexBuffer = vaoManager->createIndexBuffer(Ogre::IndexBufferPacked::IT_32BIT, edgeInds.size(), Ogre::BT_IMMUTABLE, edgeIndexData, true);
			}
			catch (Ogre::Exception& e)
			{
				OGRE_FREE_SIMD(edgeIndexData, Ogre::MEMCATEGORY_GEOMETRY);
				throw e;
			}

			// Create VAO
			Ogre::VertexBufferPackedVec edgeVertexBuffers;
			edgeVertexBuffers.push_back(edgeVertexBuffer);

			Ogre::VertexArrayObject* edgeVao = vaoManager->createVertexArrayObject(edgeVertexBuffers, edgeIndexBuffer, Ogre::OT_TRIANGLE_LIST);

			edgeSubMesh->mVao[Ogre::VpNormal].push_back(edgeVao);
			edgeSubMesh->mVao[Ogre::VpShadow].push_back(edgeVao);
		}
		else
		{
			// Create empty dummy VAO for edge submesh
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralRoadComponent] Creating empty edge submesh");

			// Create minimal dummy vertex buffer (1 vertex)
			const size_t dummyVertexDataSize = 1 * dstFloatsPerVertex * sizeof(float);
			float* dummyVertexData = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(dummyVertexDataSize, Ogre::MEMCATEGORY_GEOMETRY));

			// Position (0,0,0)
			dummyVertexData[0] = 0.0f;
			dummyVertexData[1] = 0.0f;
			dummyVertexData[2] = 0.0f;

			// Normal (0,1,0) - pointing up
			dummyVertexData[3] = 0.0f;
			dummyVertexData[4] = 1.0f;
			dummyVertexData[5] = 0.0f;

			// Tangent (1,0,0,1) - pointing along X axis with handedness 1
			dummyVertexData[6] = 1.0f;
			dummyVertexData[7] = 0.0f;
			dummyVertexData[8] = 0.0f;
			dummyVertexData[9] = 1.0f;

			// UV (0,0)
			dummyVertexData[10] = 0.0f;
			dummyVertexData[11] = 0.0f;

			Ogre::VertexBufferPacked* dummyVertexBuffer = vaoManager->createVertexBuffer(vertexElements, 1, Ogre::BT_IMMUTABLE, dummyVertexData, true);

			// Create dummy index buffer (empty - 0 indices)
			Ogre::uint32* dummyIndexData = reinterpret_cast<Ogre::uint32*>(OGRE_MALLOC_SIMD(sizeof(Ogre::uint32), Ogre::MEMCATEGORY_GEOMETRY));
			dummyIndexData[0] = 0;

			Ogre::IndexBufferPacked* dummyIndexBuffer = vaoManager->createIndexBuffer(Ogre::IndexBufferPacked::IT_32BIT, 0, Ogre::BT_IMMUTABLE, dummyIndexData, true);

			Ogre::VertexBufferPackedVec dummyVertexBuffers;
			dummyVertexBuffers.push_back(dummyVertexBuffer);

			Ogre::VertexArrayObject* dummyVao = vaoManager->createVertexArrayObject(dummyVertexBuffers, dummyIndexBuffer, Ogre::OT_TRIANGLE_LIST);

			edgeSubMesh->mVao[Ogre::VpNormal].push_back(dummyVao);
			edgeSubMesh->mVao[Ogre::VpShadow].push_back(dummyVao);
		}

		// Set bounds (handle case where no vertices exist)
		if (minBounds.x == std::numeric_limits<float>::max())
		{
			// No vertices at all - set default bounds
			minBounds = Ogre::Vector3(-1, -1, -1);
			maxBounds = Ogre::Vector3(1, 1, 1);
		}

		Ogre::Aabb bounds;
		bounds.setExtents(minBounds, maxBounds);
		this->roadMesh->_setBounds(bounds, false);
		this->roadMesh->_setBoundingSphereRadius(bounds.getRadius());

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralRoadComponent] Mesh bounds: min(" + Ogre::StringConverter::toString(minBounds) + "), max(" + Ogre::StringConverter::toString(maxBounds) + ")");

		// Create item
		this->roadItem = this->gameObjectPtr->getSceneManager()->createItem(this->roadMesh, this->gameObjectPtr->isDynamic() ? Ogre::SCENE_DYNAMIC : Ogre::SCENE_STATIC);

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralRoadComponent] Created road item with " + Ogre::StringConverter::toString(this->roadMesh->getNumSubMeshes()) + " submeshes");

		// Apply datablocks - submesh 0 is ALWAYS center, submesh 1 is ALWAYS edges

		// Submesh 0: Center datablock
		Ogre::String centerDbName = this->centerDatablock->getString();
		if (false == centerDbName.empty())
		{
			Ogre::HlmsDatablock* centerDb = Ogre::Root::getSingleton().getHlmsManager()->getDatablockNoDefault(centerDbName);
			if (nullptr != centerDb)
			{
				this->roadItem->getSubItem(0)->setDatablock(centerDb);
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralRoadComponent] Applied center datablock: " + centerDbName);
			}
		}

		// Submesh 1: Edge datablock
		Ogre::String edgeDbName = this->edgeDatablock->getString();
		if (false == edgeDbName.empty())
		{
			Ogre::HlmsDatablock* edgeDb = Ogre::Root::getSingleton().getHlmsManager()->getDatablockNoDefault(edgeDbName);
			if (nullptr != edgeDb)
			{
				this->roadItem->getSubItem(1)->setDatablock(edgeDb);
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralRoadComponent] Applied edge datablock: " + edgeDbName);
			}
		}
		else
		{
			// Fallback: use center datablock if no edge datablock specified
			if (false == centerDbName.empty())
			{
				Ogre::HlmsDatablock* centerDb = Ogre::Root::getSingleton().getHlmsManager()->getDatablockNoDefault(centerDbName);
				if (nullptr != centerDb)
				{
					this->roadItem->getSubItem(1)->setDatablock(centerDb);
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralRoadComponent] Applied center datablock to edges (fallback)");
				}
			}
		}

		this->gameObjectPtr->getSceneNode()->setPosition(origin);
		this->gameObjectPtr->getSceneNode()->attachObject(this->roadItem);
		this->gameObjectPtr->setDoNotDestroyMovableObject(true);
		this->gameObjectPtr->init(this->roadItem);

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralRoadComponent] Road mesh created with " + Ogre::StringConverter::toString(numCenterVerts) + " center vertices and " + Ogre::StringConverter::toString(numEdgeVerts) + " edge vertices, attached to scene");
	}

	void ProceduralRoadComponent::destroyRoadMesh(void)
	{
		if (nullptr == this->roadItem && nullptr == this->roadMesh)
		{
			return;
		}

		GraphicsModule::RenderCommand renderCommand = [this]()
		{
			if (nullptr != this->roadItem)
			{
				if (this->roadItem->getParentSceneNode())
				{
					this->roadItem->getParentSceneNode()->detachObject(this->roadItem);
				}
				this->gameObjectPtr->getSceneManager()->destroyItem(this->roadItem);
				this->roadItem = nullptr;
				this->gameObjectPtr->nullMovableObject();
			}

			if (this->roadMesh)
			{
				Ogre::MeshManager::getSingleton().remove(this->roadMesh->getHandle());
				this->roadMesh.reset();
			}
		};
		NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralRoadComponent::destroyRoadMesh");
	}

	void ProceduralRoadComponent::destroyPreviewMesh(void)
	{
		if (nullptr == this->previewItem && nullptr == this->previewMesh)
		{
			return;
		}

		GraphicsModule::RenderCommand renderCommand = [this]()
		{
			if (nullptr != this->previewItem)
			{
				if (this->previewItem->getParentSceneNode())
				{
					this->previewItem->getParentSceneNode()->detachObject(this->previewItem);
				}
				this->gameObjectPtr->getSceneManager()->destroyItem(this->previewItem);
				this->previewItem = nullptr;
			}

			if (nullptr != this->previewMesh)
			{
				Ogre::MeshManager::getSingleton().remove(this->previewMesh->getHandle());
				this->previewMesh.reset();
			}
		};
		NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralRoadComponent::destroyPreviewMesh");
	}

	void ProceduralRoadComponent::updatePreviewMesh(void)
	{
		if (this->currentSegment.controlPoints.size() < 2)
		{
			return;
		}

		// Create preview node if it doesn't exist
		if (nullptr == this->previewNode)
		{
			this->previewNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode();
		}

		// Generate preview geometry for current segment
		this->centerVertices.clear();
		this->centerIndices.clear();
		this->currentCenterVertexIndex = 0;

		this->edgeVertices.clear();
		this->edgeIndices.clear();
		this->currentEdgeVertexIndex = 0;

		// Generate the current segment in local space (relative to start point)
		RoadSegment localSegment;
		localSegment.isCurved = this->currentSegment.isCurved;
		localSegment.curvature = this->currentSegment.curvature;

		// Transform control points to local space
		Ogre::Vector3 startPos = this->currentSegment.controlPoints.front().position;
		Ogre::Real startHeight = this->currentSegment.controlPoints.front().smoothedHeight;

		for (const auto& cp : this->currentSegment.controlPoints)
		{
			RoadControlPoint localPoint;
			localPoint.position = cp.position - startPos;
			localPoint.position.y = 0.0f; // Keep flat XZ
			localPoint.groundHeight = 0.0f;
			localPoint.smoothedHeight = cp.smoothedHeight - startHeight;
			localSegment.controlPoints.push_back(localPoint);
		}

		// Generate the road geometry
		this->generateRoadSegment(localSegment);

		if (this->centerVertices.empty() && this->edgeVertices.empty())
		{
			return;
		}

		// Combine center and edge vertices for preview (single mesh for preview)
		std::vector<float> combinedVertices = this->centerVertices;
		std::vector<Ogre::uint32> combinedIndices = this->centerIndices;

		// Append edge data with offset indices
		size_t vertexOffset = this->currentCenterVertexIndex;
		combinedVertices.insert(combinedVertices.end(), this->edgeVertices.begin(), this->edgeVertices.end());
		for (auto idx : this->edgeIndices)
		{
			combinedIndices.push_back(idx + vertexOffset);
		}

		size_t totalVertices = this->currentCenterVertexIndex + this->currentEdgeVertexIndex;

		// Capture the world position for the preview node
		Ogre::Vector3 previewPosition = startPos;
		previewPosition.y = startHeight;

		// Execute on render thread
		GraphicsModule::RenderCommand renderCommand = [this, combinedVertices, combinedIndices, totalVertices, previewPosition]()
		{
			// Destroy existing preview
			if (nullptr != this->previewItem)
			{
				if (this->previewItem->getParentSceneNode())
				{
					this->previewItem->getParentSceneNode()->detachObject(this->previewItem);
				}
				this->gameObjectPtr->getSceneManager()->destroyItem(this->previewItem);
				this->previewItem = nullptr;
			}

			if (nullptr != this->previewMesh)
			{
				Ogre::MeshManager::getSingleton().remove(this->previewMesh->getHandle());
				this->previewMesh.reset();
			}

			// Create preview mesh
			Ogre::Root* root = Ogre::Root::getSingletonPtr();
			Ogre::VaoManager* vaoManager = root->getRenderSystem()->getVaoManager();

			Ogre::String meshName = "RoadPreview_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
			const Ogre::String groupName = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;

			this->previewMesh = Ogre::MeshManager::getSingleton().createManual(meshName, groupName);
			Ogre::SubMesh* subMesh = this->previewMesh->createSubMesh();

			// ADD TANGENTS to vertex format (same as main mesh)
			Ogre::VertexElement2Vec vertexElements;
			vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_POSITION));
			vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_NORMAL));
			vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT4, Ogre::VES_TANGENT));  // ADDED
			vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES));

			// Convert from 8 floats per vertex (pos+normal+uv) to 12 floats (pos+normal+tangent+uv)
			const size_t srcFloatsPerVertex = 8;
			const size_t dstFloatsPerVertex = 12;
			const size_t vertexDataSize = totalVertices * dstFloatsPerVertex * sizeof(float);
			float* vertexData = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(vertexDataSize, Ogre::MEMCATEGORY_GEOMETRY));

			for (size_t i = 0; i < totalVertices; ++i)
			{
				size_t srcOffset = i * srcFloatsPerVertex;
				size_t dstOffset = i * dstFloatsPerVertex;

				// Position
				vertexData[dstOffset + 0] = combinedVertices[srcOffset + 0];
				vertexData[dstOffset + 1] = combinedVertices[srcOffset + 1];
				vertexData[dstOffset + 2] = combinedVertices[srcOffset + 2];

				// Normal
				Ogre::Vector3 normal(combinedVertices[srcOffset + 3], combinedVertices[srcOffset + 4], combinedVertices[srcOffset + 5]);
				vertexData[dstOffset + 3] = normal.x;
				vertexData[dstOffset + 4] = normal.y;
				vertexData[dstOffset + 5] = normal.z;

				// Calculate tangent
				Ogre::Vector3 tangent;
				if (std::abs(normal.y) < 0.9f)
				{
					tangent = Ogre::Vector3::UNIT_Y.crossProduct(normal);
				}
				else
				{
					tangent = normal.crossProduct(Ogre::Vector3::UNIT_X);
				}
				tangent.normalise();

				vertexData[dstOffset + 6] = tangent.x;
				vertexData[dstOffset + 7] = tangent.y;
				vertexData[dstOffset + 8] = tangent.z;
				vertexData[dstOffset + 9] = 1.0f;

				// UV
				vertexData[dstOffset + 10] = combinedVertices[srcOffset + 6];
				vertexData[dstOffset + 11] = combinedVertices[srcOffset + 7];
			}

			Ogre::VertexBufferPacked* vertexBuffer = vaoManager->createVertexBuffer(vertexElements, totalVertices, Ogre::BT_IMMUTABLE, vertexData, true);

			const size_t indexDataSize = combinedIndices.size() * sizeof(Ogre::uint32);
			Ogre::uint32* indexData = reinterpret_cast<Ogre::uint32*>(OGRE_MALLOC_SIMD(indexDataSize, Ogre::MEMCATEGORY_GEOMETRY));
			memcpy(indexData, combinedIndices.data(), indexDataSize);

			Ogre::IndexBufferPacked* indexBuffer = vaoManager->createIndexBuffer(Ogre::IndexBufferPacked::IT_32BIT, combinedIndices.size(), Ogre::BT_IMMUTABLE, indexData, true);

			Ogre::VertexBufferPackedVec vertexBuffers;
			vertexBuffers.push_back(vertexBuffer);

			Ogre::VertexArrayObject* vao = vaoManager->createVertexArrayObject(vertexBuffers, indexBuffer, Ogre::OT_TRIANGLE_LIST);

			subMesh->mVao[Ogre::VpNormal].push_back(vao);
			subMesh->mVao[Ogre::VpShadow].push_back(vao);

			// Calculate proper bounds
			Ogre::Vector3 minBounds(std::numeric_limits<float>::max());
			Ogre::Vector3 maxBounds(std::numeric_limits<float>::lowest());

			for (size_t i = 0; i < totalVertices; ++i)
			{
				size_t offset = i * srcFloatsPerVertex;
				Ogre::Vector3 pos(combinedVertices[offset + 0], combinedVertices[offset + 1], combinedVertices[offset + 2]);
				minBounds.makeFloor(pos);
				maxBounds.makeCeil(pos);
			}

			Ogre::Aabb bounds;
			bounds.setExtents(minBounds, maxBounds);
			this->previewMesh->_setBounds(bounds, false);
			this->previewMesh->_setBoundingSphereRadius(bounds.getRadius());

			this->previewItem = this->gameObjectPtr->getSceneManager()->createItem(this->previewMesh, Ogre::SCENE_DYNAMIC);

			// Position the preview node at the start point
			this->previewNode->setPosition(previewPosition);
			this->previewNode->attachObject(this->previewItem);

			// Apply a semi-transparent material for preview if available
			Ogre::String dbName = this->centerDatablock->getString();
			if (false == dbName.empty())
			{
				Ogre::HlmsDatablock* db = Ogre::Root::getSingletonPtr()->getHlmsManager()->getDatablockNoDefault(dbName);
				if (nullptr != db)
				{
					this->previewItem->getSubItem(0)->setDatablock(db);
				}
			}
		};
		NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralRoadComponent::updatePreviewMesh");
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Save/Load
	///////////////////////////////////////////////////////////////////////////////////////////////

	Ogre::String ProceduralRoadComponent::getRoadDataFilePath(void) const
	{
		return "RoadData_" + this->gameObjectPtr->getName() + ".dat";
	}

	bool ProceduralRoadComponent::saveRoadDataToFile(void)
	{
		if (this->currentCenterVertexIndex == 0 && this->currentEdgeVertexIndex == 0)
		{
			return true;
		}

		try
		{
			std::ofstream file(this->getRoadDataFilePath(), std::ios::binary);
			if (!file.is_open())
			{
				return false;
			}

			// Write header
			file.write(reinterpret_cast<const char*>(&ROADDATA_MAGIC), sizeof(uint32_t));
			file.write(reinterpret_cast<const char*>(&ROADDATA_VERSION), sizeof(uint32_t));

			// Write road origin
			file.write(reinterpret_cast<const char*>(&this->roadOrigin), sizeof(Ogre::Vector3));

			// Write center mesh data
			uint32_t numCenterVerts = static_cast<uint32_t>(this->currentCenterVertexIndex);
			uint32_t numCenterInds = static_cast<uint32_t>(this->centerIndices.size());
			file.write(reinterpret_cast<const char*>(&numCenterVerts), sizeof(uint32_t));
			file.write(reinterpret_cast<const char*>(&numCenterInds), sizeof(uint32_t));
			file.write(reinterpret_cast<const char*>(this->centerVertices.data()),
				this->centerVertices.size() * sizeof(float));
			file.write(reinterpret_cast<const char*>(this->centerIndices.data()),
				this->centerIndices.size() * sizeof(uint32_t));

			// Write edge mesh data
			uint32_t numEdgeVerts = static_cast<uint32_t>(this->currentEdgeVertexIndex);
			uint32_t numEdgeInds = static_cast<uint32_t>(this->edgeIndices.size());
			file.write(reinterpret_cast<const char*>(&numEdgeVerts), sizeof(uint32_t));
			file.write(reinterpret_cast<const char*>(&numEdgeInds), sizeof(uint32_t));
			file.write(reinterpret_cast<const char*>(this->edgeVertices.data()),
				this->edgeVertices.size() * sizeof(float));
			file.write(reinterpret_cast<const char*>(this->edgeIndices.data()),
				this->edgeIndices.size() * sizeof(uint32_t));

			file.close();
			return true;
		}
		catch (...)
		{
			return false;
		}
	}

	bool ProceduralRoadComponent::loadRoadDataFromFile(void)
	{
		try
		{
			std::ifstream file(this->getRoadDataFilePath(), std::ios::binary);
			if (!file.is_open())
			{
				return false;
			}

			// Read and verify header
			uint32_t magic, version;
			file.read(reinterpret_cast<char*>(&magic), sizeof(uint32_t));
			file.read(reinterpret_cast<char*>(&version), sizeof(uint32_t));

			if (magic != ROADDATA_MAGIC || version != ROADDATA_VERSION)
			{
				file.close();
				return false;
			}

			// Read road origin
			file.read(reinterpret_cast<char*>(&this->cachedRoadOrigin), sizeof(Ogre::Vector3));
			this->roadOrigin = this->cachedRoadOrigin;
			this->hasRoadOrigin = true;

			// Read center mesh data
			uint32_t numCenterVerts, numCenterInds;
			file.read(reinterpret_cast<char*>(&numCenterVerts), sizeof(uint32_t));
			file.read(reinterpret_cast<char*>(&numCenterInds), sizeof(uint32_t));

			this->cachedNumCenterVertices = numCenterVerts;
			this->cachedCenterVertices.resize(numCenterVerts * 8);  // 8 floats per vertex
			this->cachedCenterIndices.resize(numCenterInds);

			file.read(reinterpret_cast<char*>(this->cachedCenterVertices.data()),
				this->cachedCenterVertices.size() * sizeof(float));
			file.read(reinterpret_cast<char*>(this->cachedCenterIndices.data()),
				this->cachedCenterIndices.size() * sizeof(uint32_t));

			// Read edge mesh data
			uint32_t numEdgeVerts, numEdgeInds;
			file.read(reinterpret_cast<char*>(&numEdgeVerts), sizeof(uint32_t));
			file.read(reinterpret_cast<char*>(&numEdgeInds), sizeof(uint32_t));

			this->cachedNumEdgeVertices = numEdgeVerts;
			this->cachedEdgeVertices.resize(numEdgeVerts * 8);
			this->cachedEdgeIndices.resize(numEdgeInds);

			file.read(reinterpret_cast<char*>(this->cachedEdgeVertices.data()),
				this->cachedEdgeVertices.size() * sizeof(float));
			file.read(reinterpret_cast<char*>(this->cachedEdgeIndices.data()),
				this->cachedEdgeIndices.size() * sizeof(uint32_t));

			file.close();
			return true;
		}
		catch (...)
		{
			return false;
		}
	}

	void ProceduralRoadComponent::deleteRoadDataFile(void)
	{
		std::remove(this->getRoadDataFilePath().c_str());
	}

	void ProceduralRoadComponent::handleMeshModifyMode(NOWA::EventDataPtr eventData)
	{
		boost::shared_ptr<NOWA::EventDataEditorMode> castEventData = boost::static_pointer_cast<NOWA::EventDataEditorMode>(eventData);

		bool wasModifiable = this->canModify;

		if (NOWA::EditorManager::EDITOR_MESH_MODIFY_MODE == castEventData->getManipulationMode())
		{
			this->canModify = true;
		}
		else
		{
			this->canModify = false;

			// If we lost modify permission, clean up
			if (wasModifiable)
			{
				this->removeInputListener();
				if (this->buildState == BuildState::DRAGGING)
				{
					this->cancelRoad();
				}
			}
		}
	}

	void ProceduralRoadComponent::handleGameObjectSelected(NOWA::EventDataPtr eventData)
	{
		boost::shared_ptr<NOWA::EventDataGameObjectSelected> castEventData = boost::static_pointer_cast<NOWA::EventDataGameObjectSelected>(eventData);

		if (nullptr != this->gameObjectPtr)
		{
			if (castEventData->getGameObjectId() == this->gameObjectPtr->getId())
			{
				// This road component's GameObject was selected
				if (true == this->canModify && true == castEventData->getIsSelected())
				{
					// Only add listener if:
					// 1. We can modify
					// 2. Object is being selected (not deselected)
					// 3. We're in IDLE state (not already building)
					if (this->buildState == BuildState::IDLE)
					{
						this->addInputListener();
					}
				}
				else
				{
					// Object deselected or can't modify - clean up
					this->removeInputListener();
					if (this->buildState == BuildState::DRAGGING)
					{
						this->cancelRoad();
					}
				}
			}
			else
			{
				// Different object selected - clean up this component
				this->removeInputListener();
				if (this->buildState == BuildState::DRAGGING)
				{
					this->cancelRoad();
				}
			}
		}
	}

	void ProceduralRoadComponent::addInputListener(void)
	{
		const Ogre::String listenerName = ProceduralRoadComponent::getStaticClassName() + "_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
		NOWA::GraphicsModule::RenderCommand renderCommand = [this, listenerName]()
			{
				if (auto* core = InputDeviceCore::getSingletonPtr())
				{
					if (auto* core = InputDeviceCore::getSingletonPtr())
					{
						core->addKeyListener(this, listenerName);
						core->addMouseListener(this, listenerName);
					}
				}
			};
		NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "SelectionManager::handleMousePress select");
	}

	void ProceduralRoadComponent::removeInputListener(void)
	{
		const Ogre::String listenerName = ProceduralRoadComponent::getStaticClassName() + "_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
		NOWA::GraphicsModule::RenderCommand renderCommand = [this, listenerName]()
			{
				if (auto* core = InputDeviceCore::getSingletonPtr())
				{
					core->removeKeyListener(listenerName);
					core->removeMouseListener(listenerName);
				}
			};
		NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "SelectionManager::handleMousePress select");
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Utility Functions
	///////////////////////////////////////////////////////////////////////////////////////////////

	Ogre::Vector3 ProceduralRoadComponent::snapToGridFunc(const Ogre::Vector3& position)
	{
		if (!this->snapToGrid->getBool())
			return position;

		Ogre::Real gridSz = this->gridSize->getReal();

		return Ogre::Vector3(
			Ogre::Math::Floor(position.x / gridSz + 0.5f) * gridSz,
			position.y,  // Don't snap Y
			Ogre::Math::Floor(position.z / gridSz + 0.5f) * gridSz
		);
	}

	ProceduralRoadComponent::RoadStyle ProceduralRoadComponent::getRoadStyleEnum(void) const
	{
		Ogre::String styleStr = this->roadStyle->getListSelectedValue();

		if (styleStr == "Paved")
			return RoadStyle::PAVED;
		else if (styleStr == "Highway")
			return RoadStyle::HIGHWAY;
		else if (styleStr == "Trail")
			return RoadStyle::TRAIL;
		else if (styleStr == "Dirt")
			return RoadStyle::DIRT;
		else if (styleStr == "Cobblestone")
			return RoadStyle::COBBLESTONE;

		return RoadStyle::PAVED;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Attribute Setters/Getters
	///////////////////////////////////////////////////////////////////////////////////////////////

	void ProceduralRoadComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
	}

	bool ProceduralRoadComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void ProceduralRoadComponent::setRoadWidth(Ogre::Real width)
	{
		this->roadWidth->setValue(width);
		this->rebuildMesh();
	}

	Ogre::Real ProceduralRoadComponent::getRoadWidth(void) const
	{
		return this->roadWidth->getReal();
	}

	void ProceduralRoadComponent::setEdgeWidth(Ogre::Real width)
	{
		this->edgeWidth->setValue(width);
		this->rebuildMesh();
	}

	Ogre::Real ProceduralRoadComponent::getEdgeWidth(void) const
	{
		return this->edgeWidth->getReal();
	}

	void ProceduralRoadComponent::setRoadStyle(const Ogre::String& style)
	{
		this->roadStyle->setListSelectedValue(style);
		this->rebuildMesh();
	}

	Ogre::String ProceduralRoadComponent::getRoadStyle(void) const
	{
		return this->roadStyle->getListSelectedValue();
	}

	void ProceduralRoadComponent::setSnapToGrid(bool snap)
	{
		this->snapToGrid->setValue(snap);
	}

	bool ProceduralRoadComponent::getSnapToGrid(void) const
	{
		return this->snapToGrid->getBool();
	}

	void ProceduralRoadComponent::setGridSize(Ogre::Real size)
	{
		this->gridSize->setValue(size);
	}

	Ogre::Real ProceduralRoadComponent::getGridSize(void) const
	{
		return this->gridSize->getReal();
	}

	void ProceduralRoadComponent::setAdaptToGround(bool adapt)
	{
		this->adaptToGround->setValue(adapt);
		this->rebuildMesh();
	}

	bool ProceduralRoadComponent::getAdaptToGround(void) const
	{
		return this->adaptToGround->getBool();
	}

	void ProceduralRoadComponent::setHeightOffset(Ogre::Real offset)
	{
		this->heightOffset->setValue(offset);
		this->rebuildMesh();
	}

	Ogre::Real ProceduralRoadComponent::getHeightOffset(void) const
	{
		return this->heightOffset->getReal();
	}

	void ProceduralRoadComponent::setMaxGradient(Ogre::Real gradient)
	{
		this->maxGradient->setValue(gradient);
		this->rebuildMesh();
	}

	Ogre::Real ProceduralRoadComponent::getMaxGradient(void) const
	{
		return this->maxGradient->getReal();
	}

	void ProceduralRoadComponent::setSmoothingFactor(Ogre::Real factor)
	{
		this->smoothingFactor->setValue(Ogre::Math::Clamp(factor, 0.0f, 1.0f));
		this->rebuildMesh();
	}

	Ogre::Real ProceduralRoadComponent::getSmoothingFactor(void) const
	{
		return this->smoothingFactor->getReal();
	}

	void ProceduralRoadComponent::setEnableBanking(bool enable)
	{
		this->enableBanking->setValue(enable);
		this->rebuildMesh();
	}

	bool ProceduralRoadComponent::getEnableBanking(void) const
	{
		return this->enableBanking->getBool();
	}

	void ProceduralRoadComponent::setBankingAngle(Ogre::Real angle)
	{
		this->bankingAngle->setValue(angle);
		this->rebuildMesh();
	}

	Ogre::Real ProceduralRoadComponent::getBankingAngle(void) const
	{
		return this->bankingAngle->getReal();
	}

	void ProceduralRoadComponent::setCurveSubdivisions(int subdivisions)
	{
		this->curveSubdivisions->setValue(Ogre::Math::Clamp(subdivisions, 3, 50));
		this->rebuildMesh();
	}

	int ProceduralRoadComponent::getCurveSubdivisions(void) const
	{
		return this->curveSubdivisions->getInt();
	}

	void ProceduralRoadComponent::setCenterDatablock(const Ogre::String& datablock)
	{
		this->centerDatablock->setValue(datablock);

		// Apply datablock immediately if road exists
		if (this->roadItem && !datablock.empty())
		{
			GraphicsModule::RenderCommand renderCommand = [this, datablock]()
			{
				// Double-check roadItem still exists
				if (nullptr == this->roadItem)
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralRoadComponent] roadItem is null, datablock will be applied on next mesh creation");
					return;
				}

				// Verify we have at least 1 submesh
				if (this->roadItem->getNumSubItems() < 1)
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
						"[ProceduralRoadComponent] roadItem has no submeshes! Cannot apply center datablock.");
					return;
				}

				Ogre::HlmsDatablock* centerDb = Ogre::Root::getSingletonPtr()->getHlmsManager()->getDatablockNoDefault(datablock);
				if (nullptr != centerDb)
				{
					// Submesh 0 is ALWAYS center
					this->roadItem->getSubItem(0)->setDatablock(centerDb);
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralRoadComponent] Applied center datablock: " + datablock);
				}
				else
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralRoadComponent] Center datablock not found: " + datablock);
				}
			};
			NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralRoadComponent::setCenterDatablock");
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
				"[ProceduralRoadComponent] Center datablock will be applied when mesh is created (roadItem: " +
				Ogre::StringConverter::toString(this->roadItem != nullptr) + ", datablock empty: " +
				Ogre::StringConverter::toString(datablock.empty()) + ")");
		}
	}

	Ogre::String ProceduralRoadComponent::getCenterDatablock(void) const
	{
		return this->centerDatablock->getString();
	}

	void ProceduralRoadComponent::setEdgeDatablock(const Ogre::String& datablock)
	{
		this->edgeDatablock->setValue(datablock);

		// Apply datablock immediately if road exists
		if (this->roadItem && !datablock.empty())
		{
			GraphicsModule::RenderCommand renderCommand = [this, datablock]()
			{
				// Double-check roadItem still exists (could be destroyed between check and execution)
				if (nullptr == this->roadItem)
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralRoadComponent] roadItem is null, datablock will be applied on next mesh creation");
					return;
				}

				// Verify we have at least 2 submeshes
				if (this->roadItem->getNumSubItems() < 2)
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralRoadComponent] roadItem has fewer than 2 submeshes! Cannot apply edge datablock.");
					return;
				}

				Ogre::HlmsDatablock* edgeDb = Ogre::Root::getSingletonPtr()->getHlmsManager()->getDatablockNoDefault(datablock);
				if (nullptr != edgeDb)
				{
					// Submesh 1 is ALWAYS edges
					this->roadItem->getSubItem(1)->setDatablock(edgeDb);
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralRoadComponent] Applied edge datablock: " + datablock);
				}
				else
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralRoadComponent] Edge datablock not found: " + datablock);
				}
			};
			NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralRoadComponent::setEdgeDatablock");
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralRoadComponent] Edge datablock will be applied when mesh is created (roadItem: " +
				Ogre::StringConverter::toString(this->roadItem != nullptr) + ", datablock empty: " + Ogre::StringConverter::toString(datablock.empty()) + ")");
		}
	}

	Ogre::String ProceduralRoadComponent::getEdgeDatablock(void) const
	{
		return this->edgeDatablock->getString();
	}

	void ProceduralRoadComponent::setCenterUVTiling(const Ogre::Vector2& tiling)
	{
		this->centerUVTiling->setValue(tiling);
		this->rebuildMesh();
	}

	Ogre::Vector2 ProceduralRoadComponent::getCenterUVTiling(void) const
	{
		return this->centerUVTiling->getVector2();
	}

	void ProceduralRoadComponent::setEdgeUVTiling(const Ogre::Vector2& tiling)
	{
		this->edgeUVTiling->setValue(tiling);
		this->rebuildMesh();
	}

	Ogre::Vector2 ProceduralRoadComponent::getEdgeUVTiling(void) const
	{
		return this->edgeUVTiling->getVector2();
	}

	void ProceduralRoadComponent::setCurbHeight(Ogre::Real height)
	{
		this->curbHeight->setValue(Ogre::Math::Clamp(height, 0.0f, 2.0f));
		if (false == this->roadSegments.empty())
		{
			this->rebuildMesh();
		}
	}

	Ogre::Real ProceduralRoadComponent::getCurbHeight(void) const
	{
		return this->curbHeight->getReal();
	}

	void ProceduralRoadComponent::setTerrainSampleInterval(Ogre::Real interval)
	{
		this->terrainSampleInterval->setValue(Ogre::Math::Clamp(interval, 0.5f, 20.0f));
		if (false == this->roadSegments.empty())
		{
			this->rebuildMesh();
		}
	}

	Ogre::Real ProceduralRoadComponent::getTerrainSampleInterval(void) const
	{
		return this->terrainSampleInterval->getReal();
	}

} // namespace NOWA
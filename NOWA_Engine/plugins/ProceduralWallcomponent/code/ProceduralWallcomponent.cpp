#include "NOWAPrecompiled.h"
#include "ProceduralWallComponent.h"
#include "gameObject/GameObjectController.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "main/AppStateManager.h"
#include "main/Core.h"
#include "main/InputDeviceCore.h"

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

namespace NOWA
{
	using namespace rapidxml;

	ProceduralWallComponent::ProceduralWallComponent()
		: GameObjectComponent(),
		name("ProceduralWallComponent"),
		buildState(BuildState::IDLE),
		currentVertexIndex(0),
		wallMesh(nullptr),
		wallItem(nullptr),
		previewMesh(nullptr),
		previewItem(nullptr),
		previewNode(nullptr),
		isShiftPressed(false),
		isCtrlPressed(false),
		lastValidPosition(Ogre::Vector3::ZERO),
		continuousMode(false),
		wallOrigin(Ogre::Vector3::ZERO),
		hasWallOrigin(false),
		groundQuery(nullptr)
	{
		this->activated = new Variant(ProceduralWallComponent::AttrActivated(), true, this->attributes);
		this->wallHeight = new Variant(ProceduralWallComponent::AttrWallHeight(), 3.0f, this->attributes);
		this->wallThickness = new Variant(ProceduralWallComponent::AttrWallThickness(), 0.3f, this->attributes);

		std::vector<Ogre::String> wallStyles;
		wallStyles.push_back("Solid");
		wallStyles.push_back("Fence");
		wallStyles.push_back("Battlement");
		wallStyles.push_back("Arch");
		this->wallStyle = new Variant(ProceduralWallComponent::AttrWallStyle(), wallStyles, this->attributes);

		this->snapToGrid = new Variant(ProceduralWallComponent::AttrSnapToGrid(), true, this->attributes);
		this->gridSize = new Variant(ProceduralWallComponent::AttrGridSize(), 1.0f, this->attributes);
		this->adaptToGround = new Variant(ProceduralWallComponent::AttrAdaptToGround(), true, this->attributes);
		this->createPillars = new Variant(ProceduralWallComponent::AttrCreatePillars(), true, this->attributes);
		this->pillarSize = new Variant(ProceduralWallComponent::AttrPillarSize(), 0.5f, this->attributes);
		this->wallDatablock = new Variant(ProceduralWallComponent::AttrWallDatablock(), Ogre::String(""), this->attributes);
		this->pillarDatablock = new Variant(ProceduralWallComponent::AttrPillarDatablock(), Ogre::String(""), this->attributes);
		this->uvTiling = new Variant(ProceduralWallComponent::AttrUVTiling(), Ogre::Vector2(1.0f, 1.0f), this->attributes);
		this->fencePostSpacing = new Variant(ProceduralWallComponent::AttrFencePostSpacing(), 2.0f, this->attributes);
		this->battlementWidth = new Variant(ProceduralWallComponent::AttrBattlementWidth(), 0.5f, this->attributes);
		this->battlementHeight = new Variant(ProceduralWallComponent::AttrBattlementHeight(), 0.5f, this->attributes);

		// Set descriptions
		this->activated->setDescription("Activate/deactivate wall building mode");
		this->wallHeight->setDescription("Height of the wall in world units");
		this->wallThickness->setDescription("Thickness of the wall");
		this->wallStyle->setDescription("Style of wall: Solid, Fence, Battlement, or Arch");
		this->snapToGrid->setDescription("Snap wall endpoints to grid");
		this->gridSize->setDescription("Grid cell size for snapping");
		this->adaptToGround->setDescription("Adapt wall bottom to terrain/ground height");
		this->createPillars->setDescription("Create corner pillars at wall endpoints");
		this->pillarSize->setDescription("Size of corner pillars (width/depth)");
		this->wallDatablock->setDescription("PBS datablock name for wall material");
		this->pillarDatablock->setDescription("PBS datablock name for pillar material (uses wall if empty)");
		this->uvTiling->setDescription("UV tiling multiplier (X, Y)");
		this->fencePostSpacing->setDescription("Spacing between fence posts (Fence style only)");
		this->battlementWidth->setDescription("Width of each battlement (Battlement style only)");
		this->battlementHeight->setDescription("Height of battlements above wall (Battlement style only)");

		// Constraints
		this->wallHeight->addUserData(GameObject::AttrActionNeedRefresh());
		this->wallThickness->addUserData(GameObject::AttrActionNeedRefresh());
		this->wallStyle->addUserData(GameObject::AttrActionNeedRefresh());
		this->createPillars->addUserData(GameObject::AttrActionNeedRefresh());
		this->pillarSize->addUserData(GameObject::AttrActionNeedRefresh());
	}

	ProceduralWallComponent::~ProceduralWallComponent()
	{
		
	}

	void ProceduralWallComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<ProceduralWallComponent>(ProceduralWallComponent::getStaticClassId(), ProceduralWallComponent::getStaticClassName());
	}

	void ProceduralWallComponent::initialise()
	{
	}

	void ProceduralWallComponent::shutdown()
	{
	}

	void ProceduralWallComponent::uninstall()
	{
	}

	const Ogre::String& ProceduralWallComponent::getName() const
	{
		return this->name;
	}

	void ProceduralWallComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool ProceduralWallComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralWallComponent::AttrActivated())
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralWallComponent::AttrWallHeight())
		{
			this->wallHeight->setValue(XMLConverter::getAttribReal(propertyElement, "data", 3.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralWallComponent::AttrWallThickness())
		{
			this->wallThickness->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.3f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralWallComponent::AttrWallStyle())
		{
			this->wallStyle->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralWallComponent::AttrSnapToGrid())
		{
			this->snapToGrid->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralWallComponent::AttrGridSize())
		{
			this->gridSize->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralWallComponent::AttrAdaptToGround())
		{
			this->adaptToGround->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralWallComponent::AttrCreatePillars())
		{
			this->createPillars->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralWallComponent::AttrPillarSize())
		{
			this->pillarSize->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.5f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralWallComponent::AttrWallDatablock())
		{
			this->wallDatablock->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralWallComponent::AttrPillarDatablock())
		{
			this->pillarDatablock->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralWallComponent::AttrUVTiling())
		{
			this->uvTiling->setValue(XMLConverter::getAttribVector2(propertyElement, "data", Ogre::Vector2(1.0f, 1.0f)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralWallComponent::AttrFencePostSpacing())
		{
			this->fencePostSpacing->setValue(XMLConverter::getAttribReal(propertyElement, "data", 2.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralWallComponent::AttrBattlementWidth())
		{
			this->battlementWidth->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.5f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralWallComponent::AttrBattlementHeight())
		{
			this->battlementHeight->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.5f));
			propertyElement = propertyElement->next_sibling("property");
		}

		return true;
	}

	bool ProceduralWallComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] Init component for game object: " + this->gameObjectPtr->getName());

		// Create raycast query for ground detection
		this->groundQuery = this->gameObjectPtr->getSceneManager()->createRayQuery(Ogre::Ray());
		this->groundQuery->setSortByDistance(true);

		// Setup fallback ground plane at Y=0
		this->groundPlane = Ogre::Plane(Ogre::Vector3::UNIT_Y, 0.0f);

		// If a listener has been added via key/mouse/joystick pressed, a new listener would be inserted during this iteration, which would cause a crash in mouse/key/button release iterator, hence add in next frame
		NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(0.25f));
		auto ptrFunction = [this]()
		{
			InputDeviceCore::getSingletonPtr()->addKeyListener(this, ProceduralWallComponent::getStaticClassName() + "_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()));
			InputDeviceCore::getSingletonPtr()->addMouseListener(this, ProceduralWallComponent::getStaticClassName() + "_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()));
		};
		NOWA::ProcessPtr closureProcess(new NOWA::ClosureProcess(ptrFunction));
		delayProcess->attachChild(closureProcess);
		NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);

		// Create preview scene node
		this->previewNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode();

		return true;
	}

	bool ProceduralWallComponent::connect(void)
	{

		return true;
	}

	bool ProceduralWallComponent::disconnect(void)
	{
		this->destroyPreviewMesh();
		this->buildState = BuildState::IDLE;

		return true;
	}

	bool ProceduralWallComponent::onCloned(void)
	{
		return true;
	}

	void ProceduralWallComponent::onRemoveComponent(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] Destructor called");

		if (nullptr != this->groundQuery)
		{
			this->gameObjectPtr->getSceneManager()->destroyQuery(this->groundQuery);
			this->groundQuery = nullptr;
		}

		InputDeviceCore::getSingletonPtr()->removeKeyListener(ProceduralWallComponent::getStaticClassName() + "_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()));
		InputDeviceCore::getSingletonPtr()->removeMouseListener(ProceduralWallComponent::getStaticClassName() + "_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()));

		this->destroyWallMesh();
		this->destroyPreviewMesh();

		if (this->previewNode)
		{
			this->gameObjectPtr->getSceneManager()->destroySceneNode(this->previewNode);
			this->previewNode = nullptr;
		}
	}

	void ProceduralWallComponent::onOtherComponentRemoved(unsigned int index)
	{
	}
	void ProceduralWallComponent::onOtherComponentAdded(unsigned int index)
	{
	}

	GameObjectCompPtr ProceduralWallComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		ProceduralWallComponentPtr clonedCompPtr(boost::make_shared<ProceduralWallComponent>());

		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setWallHeight(this->wallHeight->getReal());
		clonedCompPtr->setWallThickness(this->wallThickness->getReal());
		clonedCompPtr->setWallStyle(this->wallStyle->getListSelectedValue());
		clonedCompPtr->setSnapToGrid(this->snapToGrid->getBool());
		clonedCompPtr->setGridSize(this->gridSize->getReal());
		clonedCompPtr->setAdaptToGround(this->adaptToGround->getBool());
		clonedCompPtr->setCreatePillars(this->createPillars->getBool());
		clonedCompPtr->setPillarSize(this->pillarSize->getReal());
		clonedCompPtr->setWallDatablock(this->wallDatablock->getString());
		clonedCompPtr->setPillarDatablock(this->pillarDatablock->getString());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	void ProceduralWallComponent::update(Ogre::Real dt, bool notSimulating)
	{
		// Update preview visualization during dragging
		if (this->buildState == BuildState::DRAGGING && this->previewItem)
		{
			// Preview is updated in mouseMoved
		}
	}

	void ProceduralWallComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (ProceduralWallComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (ProceduralWallComponent::AttrWallHeight() == attribute->getName())
		{
			this->setWallHeight(attribute->getReal());
		}
		else if (ProceduralWallComponent::AttrWallThickness() == attribute->getName())
		{
			this->setWallThickness(attribute->getReal());
		}
		else if (ProceduralWallComponent::AttrWallStyle() == attribute->getName())
		{
			this->setWallStyle(attribute->getListSelectedValue());
		}
		else if (ProceduralWallComponent::AttrSnapToGrid() == attribute->getName())
		{
			this->setSnapToGrid(attribute->getBool());
		}
		else if (ProceduralWallComponent::AttrGridSize() == attribute->getName())
		{
			this->setGridSize(attribute->getReal());
		}
		else if (ProceduralWallComponent::AttrAdaptToGround() == attribute->getName())
		{
			this->setAdaptToGround(attribute->getBool());
		}
		else if (ProceduralWallComponent::AttrCreatePillars() == attribute->getName())
		{
			this->setCreatePillars(attribute->getBool());
		}
		else if (ProceduralWallComponent::AttrPillarSize() == attribute->getName())
		{
			this->setPillarSize(attribute->getReal());
		}
		else if (ProceduralWallComponent::AttrWallDatablock() == attribute->getName())
		{
			this->setWallDatablock(attribute->getString());
		}
		else if (ProceduralWallComponent::AttrPillarDatablock() == attribute->getName())
		{
			this->setPillarDatablock(attribute->getString());
		}
		else if (ProceduralWallComponent::AttrUVTiling() == attribute->getName())
		{
			this->setUVTiling(attribute->getVector2());
		}
		else if (ProceduralWallComponent::AttrFencePostSpacing() == attribute->getName())
		{
			this->setFencePostSpacing(attribute->getReal());
		}
		else if (ProceduralWallComponent::AttrBattlementWidth() == attribute->getName())
		{
			this->setBattlementWidth(attribute->getReal());
		}
		else if (ProceduralWallComponent::AttrBattlementHeight() == attribute->getName())
		{
			this->setBattlementHeight(attribute->getReal());
		}
	}

	void ProceduralWallComponent::writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc)
	{
		GameObjectComponent::writeXML(propertiesXML, doc);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", ProceduralWallComponent::AttrActivated().c_str()));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", ProceduralWallComponent::AttrWallHeight().c_str()));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->wallHeight->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", ProceduralWallComponent::AttrWallThickness().c_str()));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->wallThickness->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", ProceduralWallComponent::AttrWallStyle().c_str()));
		propertyXML->append_attribute(doc.allocate_attribute("data", this->wallStyle->getListSelectedValue().c_str()));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", ProceduralWallComponent::AttrSnapToGrid().c_str()));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->snapToGrid->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", ProceduralWallComponent::AttrGridSize().c_str()));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->gridSize->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", ProceduralWallComponent::AttrAdaptToGround().c_str()));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->adaptToGround->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", ProceduralWallComponent::AttrCreatePillars().c_str()));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->createPillars->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", ProceduralWallComponent::AttrPillarSize().c_str()));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->pillarSize->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", ProceduralWallComponent::AttrWallDatablock().c_str()));
		propertyXML->append_attribute(doc.allocate_attribute("data", this->wallDatablock->getString().c_str()));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", ProceduralWallComponent::AttrPillarDatablock().c_str()));
		propertyXML->append_attribute(doc.allocate_attribute("data", this->pillarDatablock->getString().c_str()));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", ProceduralWallComponent::AttrUVTiling().c_str()));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->uvTiling->getVector2())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", ProceduralWallComponent::AttrFencePostSpacing().c_str()));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->fencePostSpacing->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", ProceduralWallComponent::AttrBattlementWidth().c_str()));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->battlementWidth->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", ProceduralWallComponent::AttrBattlementHeight().c_str()));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->battlementHeight->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	bool ProceduralWallComponent::canStaticAddComponent(GameObject* gameObject)
	{
		// Special component, cannot only be added via editor   
		return false;
	}

	// ==================== MOUSE/KEY HANDLERS ====================

	bool ProceduralWallComponent::mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
	{
		if (false == this->activated->getBool())
		{
			return false;
		}

		if (id != OIS::MB_Left)
		{
			return false;
		}

		// Check if clicking on GUI
		if (MyGUI::InputManager::getInstance().getMouseFocusWidget() != nullptr)
		{
			return false;
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
				// Start new wall
				this->startWallPlacement(hitPosition);
			}
			else if (this->buildState == BuildState::DRAGGING)
			{
				// Confirm current wall and potentially start new one
				this->confirmWall();

				if (this->isShiftPressed)
				{
					// Continuous mode: start next segment from end of last
					this->startWallPlacement(hitPosition);
				}
			}
		}

		return true;
	}

	bool ProceduralWallComponent::mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
	{
		if (false == this->activated->getBool())
		{
			return false;
		}

		if (id == OIS::MB_Right)
		{
			// Cancel current wall
			this->cancelWall();
			return true;
		}

		return false;
	}

	bool ProceduralWallComponent::mouseMoved(const OIS::MouseEvent& evt)
	{
		if (false == this->activated->getBool())
		{
			return false;
		}

		if (this->buildState != BuildState::DRAGGING)
		{
			return false;
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

			// Constrain to axis if shift is held
			if (this->isCtrlPressed)
			{
				Ogre::Vector3 delta = hitPosition - this->currentSegment.startPoint;
				if (std::abs(delta.x) > std::abs(delta.z))
				{
					hitPosition.z = this->currentSegment.startPoint.z;
				}
				else
				{
					hitPosition.x = this->currentSegment.startPoint.x;
				}
			}

			this->updateWallPreview(hitPosition);
		}

		return false;
	}

	bool ProceduralWallComponent::keyPressed(const OIS::KeyEvent& evt)
	{
		if (evt.key == OIS::KC_LSHIFT || evt.key == OIS::KC_RSHIFT)
		{
			this->isShiftPressed = true;
		}
		else if (evt.key == OIS::KC_LCONTROL || evt.key == OIS::KC_RCONTROL)
		{
			this->isCtrlPressed = true;
		}
		else if (evt.key == OIS::KC_ESCAPE)
		{
			this->cancelWall();
		}
		else if (evt.key == OIS::KC_Z && this->isCtrlPressed)
		{
			// Undo last segment
			this->removeLastSegment();
		}

		return false;
	}

	bool ProceduralWallComponent::keyReleased(const OIS::KeyEvent& evt)
	{
		if (evt.key == OIS::KC_LSHIFT || evt.key == OIS::KC_RSHIFT)
		{
			this->isShiftPressed = false;
		}
		else if (evt.key == OIS::KC_LCONTROL || evt.key == OIS::KC_RCONTROL)
		{
			this->isCtrlPressed = false;
		}

		return false;
	}

	// ==================== WALL BUILDING API ====================

	void ProceduralWallComponent::startWallPlacement(const Ogre::Vector3& worldPosition)
	{
		// Set wall origin on first placement
		if (false == this->hasWallOrigin)
		{
			this->wallOrigin = worldPosition;
			this->wallOrigin.y = this->adaptToGround->getBool() ? this->getGroundHeight(worldPosition) : 0.0f;
			this->hasWallOrigin = true;
		}

		this->currentSegment.startPoint = worldPosition;
		this->currentSegment.endPoint = worldPosition;
		this->currentSegment.groundHeightStart = this->adaptToGround->getBool() ? this->getGroundHeight(worldPosition) : 0.0f;
		this->currentSegment.hasStartPillar = this->createPillars->getBool();
		this->currentSegment.hasEndPillar = this->createPillars->getBool();

		this->buildState = BuildState::DRAGGING;
		this->lastValidPosition = worldPosition;

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] Started wall placement at: " + Ogre::StringConverter::toString(worldPosition));
	}

	void ProceduralWallComponent::updateWallPreview(const Ogre::Vector3& worldPosition)
	{
		this->currentSegment.endPoint = worldPosition;
		this->currentSegment.groundHeightEnd = this->adaptToGround->getBool() ? this->getGroundHeight(worldPosition) : 0.0f;

		this->lastValidPosition = worldPosition;
		this->updatePreviewMesh();
	}

	void ProceduralWallComponent::confirmWall(void)
	{
		if (this->buildState != BuildState::DRAGGING)
		{
			return;
		}

		// Don't add zero-length walls
		Ogre::Real length = this->currentSegment.startPoint.distance(this->currentSegment.endPoint);
		if (length < 0.1f)
		{
			this->cancelWall();
			return;
		}

		// Check if this segment connects to existing wall (skip start pillar)
		if (!this->wallSegments.empty())
		{
			const WallSegment& lastSeg = this->wallSegments.back();
			if (lastSeg.endPoint.squaredDistance(this->currentSegment.startPoint) < 0.01f)
			{
				this->currentSegment.hasStartPillar = false;
			}
		}

		this->wallSegments.push_back(this->currentSegment);

		// Clean up preview BEFORE rebuilding final mesh
		this->destroyPreviewMesh();

		// Reset preview node position
		if (this->previewNode)
		{
			this->previewNode->setPosition(Ogre::Vector3::ZERO);
		}

		this->rebuildMesh();

		// If shift is held, start next segment from END of current segment
		if (this->isShiftPressed)
		{
			this->startWallPlacement(this->currentSegment.endPoint);
		}
		else
		{
			this->buildState = BuildState::IDLE;
		}

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] Confirmed wall segment, total segments: " + Ogre::StringConverter::toString(this->wallSegments.size()));
	}

	void ProceduralWallComponent::cancelWall(void)
	{
		this->destroyPreviewMesh();
		this->buildState = BuildState::IDLE;

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] Cancelled wall placement");
	}

	void ProceduralWallComponent::addWallSegment(const Ogre::Vector3& start, const Ogre::Vector3& end)
	{
		WallSegment segment;
		segment.startPoint = start;
		segment.endPoint = end;
		segment.groundHeightStart = this->adaptToGround->getBool() ? this->getGroundHeight(start) : 0.0f;
		segment.groundHeightEnd = this->adaptToGround->getBool() ? this->getGroundHeight(end) : 0.0f;
		segment.hasStartPillar = this->createPillars->getBool();
		segment.hasEndPillar = this->createPillars->getBool();

		this->wallSegments.push_back(segment);
		this->rebuildMesh();
	}

	void ProceduralWallComponent::removeLastSegment(void)
	{
		if (true == this->wallSegments.empty())
		{
			return;
		}

		this->wallSegments.pop_back();
		this->rebuildMesh();

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] Removed last segment, remaining: " + Ogre::StringConverter::toString(this->wallSegments.size()));
	}

	void ProceduralWallComponent::clearAllSegments(void)
	{
		this->wallSegments.clear();
		this->destroyWallMesh();
		this->hasWallOrigin = false;
		this->wallOrigin = Ogre::Vector3::ZERO;

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] Cleared all wall segments");
	}

	// ==================== MESH OPERATIONS ====================

	void ProceduralWallComponent::rebuildMesh(void)
	{
		this->destroyWallMesh();

		if (true == this->wallSegments.empty())
		{
			return;
		}

		this->createWallMesh();
	}

	bool ProceduralWallComponent::exportMesh(const Ogre::String& filename)
	{
		if (nullptr == this->wallMesh)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralWallComponent] No mesh to export!");
			return false;
		}

		try
		{
			Ogre::MeshSerializer serializer(Ogre::Root::getSingletonPtr()->getRenderSystem()->getVaoManager());
			serializer.exportMesh(this->wallMesh.get(), filename);

			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] Exported mesh to: " + filename);
			return true;
		}
		catch (Ogre::Exception& e)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralWallComponent] Failed to export mesh: " + e.getDescription());
			return false;
		}
	}

	// ==================== GROUND ADAPTATION ====================

	Ogre::Real ProceduralWallComponent::getGroundHeight(const Ogre::Vector3& position)
	{
		// Create a ray from high above pointing down
		Ogre::Vector3 rayOrigin = Ogre::Vector3(position.x, position.y + 1000.0f, position.z);
		Ogre::Ray downRay(rayOrigin, Ogre::Vector3::NEGATIVE_UNIT_Y);

		Ogre::Vector3 internalHitPoint = Ogre::Vector3::ZERO;
		Ogre::MovableObject* hitMovableObject = nullptr;
		Ogre::Real closestDistance = 0.0f;
		Ogre::Vector3 normal = Ogre::Vector3::ZERO;

		// Build exclusion list - exclude our own wall and preview items
		std::vector<Ogre::MovableObject*> excludeMovableObjects;
		if (this->wallItem)
		{
			excludeMovableObjects.emplace_back(this->wallItem);
		}
		if (this->previewItem)
		{
			excludeMovableObjects.emplace_back(this->previewItem);
		}

		// Set the ray for the query
		this->groundQuery->setRay(downRay);
		this->groundQuery->setSortByDistance(true);

		Ogre::RaySceneQueryResult& results = this->groundQuery->execute();

		Ogre::Real groundHeight = 0.0f;
		bool foundGround = false;

		for (auto& result : results)
		{
			if (result.movable)
			{
				// Skip our own wall items
				bool isExcluded = false;
				for (auto* excluded : excludeMovableObjects)
				{
					if (result.movable == excluded)
					{
						isExcluded = true;
						break;
					}
				}

				if (true == isExcluded)
				{
					continue;
				}

				// Found ground - calculate Y position
				groundHeight = rayOrigin.y - result.distance;
				foundGround = true;

				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] Ground height at " + Ogre::StringConverter::toString(position) + " = " + Ogre::StringConverter::toString(groundHeight));

				break;
			}
		}

		// If no ground found, use position Y or fallback to 0
		if (false == foundGround)
		{
			// Try plane intersection as fallback
			std::pair<bool, Ogre::Real> planeResult = downRay.intersects(this->groundPlane);
			if (planeResult.first && planeResult.second > 0.0f)
			{
				groundHeight = rayOrigin.y - planeResult.second;
			}
			else
			{
				groundHeight = position.y; // Use current position Y
			}
		}

		return groundHeight;
	}

	bool ProceduralWallComponent::raycastGround(Ogre::Real screenX, Ogre::Real screenY, Ogre::Vector3& hitPosition)
	{
		Ogre::Camera* camera = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
		if (!camera)
			return false;

		Ogre::Ray ray = camera->getCameraToViewportRay(screenX, screenY);

		Ogre::Vector3 internalHitPoint = Ogre::Vector3::ZERO;
		Ogre::MovableObject* hitMovableObject = nullptr;
		Ogre::Real closestDistance = 0.0f;
		Ogre::Vector3 normal = Ogre::Vector3::ZERO;

		// Build exclusion list - exclude our own wall and preview items
		std::vector<Ogre::MovableObject*> excludeMovableObjects;
		if (this->wallItem)
		{
			excludeMovableObjects.emplace_back(this->wallItem);
		}
		if (this->previewItem)
		{
			excludeMovableObjects.emplace_back(this->previewItem);
		}

		// Perform raycast using MathHelper
		MathHelper::getInstance()->getRaycastFromPoint(this->groundQuery, camera, internalHitPoint, (size_t&)hitMovableObject, closestDistance, normal, &excludeMovableObjects);

		// If we hit something, use that position
		if (hitMovableObject != nullptr)
		{
			hitPosition = internalHitPoint;

			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] Raycast hit: " + Ogre::StringConverter::toString(hitPosition) + " on object: " + hitMovableObject->getName());

			return true;
		}

		// Fallback: intersect with ground plane at Y=0
		std::pair<bool, Ogre::Real> result = ray.intersects(this->groundPlane);
		if (result.first && result.second > 0.0f)
		{
			hitPosition = ray.getPoint(result.second);

			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] Raycast fallback to ground plane: " + Ogre::StringConverter::toString(hitPosition));

			return true;
		}

		return false;
	}

	// ==================== GEOMETRY GENERATION ====================

	void ProceduralWallComponent::createWallMesh(void)
	{
		if (this->wallSegments.empty())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[ProceduralWallComponent] createWallMesh: No segments to create");
			return;
		}

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[ProceduralWallComponent] createWallMesh: Creating mesh for " + Ogre::StringConverter::toString(this->wallSegments.size()) + " segments");

		// Clear mesh data
		this->vertices.clear();
		this->indices.clear();
		this->currentVertexIndex = 0;

		// Use stored wall origin (set when first segment was created)
		Ogre::Vector3 wallOriginToUse = this->wallOrigin;

		// Generate geometry for each segment (in LOCAL space relative to wallOrigin)
		WallStyle style = this->getWallStyleEnum();

		for (size_t segIdx = 0; segIdx < this->wallSegments.size(); ++segIdx)
		{
			const WallSegment& segment = this->wallSegments[segIdx];

			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL,
				"[ProceduralWallComponent] Generating segment " + Ogre::StringConverter::toString(segIdx) +
				": start(" + Ogre::StringConverter::toString(segment.startPoint) +
				") -> end(" + Ogre::StringConverter::toString(segment.endPoint) + ")");

			// Create a LOCAL segment relative to wallOrigin
			WallSegment localSegment;
			localSegment.startPoint = segment.startPoint - wallOriginToUse;
			localSegment.startPoint.y = 0.0f;  // CRITICAL: Reset Y to 0 in local space

			localSegment.endPoint = segment.endPoint - wallOriginToUse;
			localSegment.endPoint.y = 0.0f;    // CRITICAL: Reset Y to 0 in local space

			// Ground heights are now relative to wallOrigin.y
			localSegment.groundHeightStart = segment.groundHeightStart - wallOriginToUse.y;
			localSegment.groundHeightEnd = segment.groundHeightEnd - wallOriginToUse.y;
			localSegment.hasStartPillar = segment.hasStartPillar;
			localSegment.hasEndPillar = segment.hasEndPillar;

			switch (style)
			{
			case WallStyle::SOLID:
				this->generateSolidWall(localSegment);
				break;
			case WallStyle::FENCE:
				this->generateFenceWall(localSegment);
				break;
			case WallStyle::BATTLEMENT:
				this->generateBattlementWall(localSegment);
				break;
			case WallStyle::ARCH:
				this->generateArchWall(localSegment);
				break;
			}

			// Generate pillars
			if (segment.hasStartPillar)
			{
				Ogre::Vector3 localPos = segment.startPoint - wallOriginToUse;
				localPos.y = 0.0f;  // Reset Y
				this->generatePillar(localPos, segment.groundHeightStart - wallOriginToUse.y);
			}
			if (segment.hasEndPillar)
			{
				Ogre::Vector3 localPos = segment.endPoint - wallOriginToUse;
				localPos.y = 0.0f;  // Reset Y
				this->generatePillar(localPos, segment.groundHeightEnd - wallOriginToUse.y);
			}
		}

		if (this->vertices.empty())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
				"[ProceduralWallComponent] No vertices generated!");
			return;
		}

		// Create Ogre mesh
		std::vector<float> verticesCopy = this->vertices;
		std::vector<Ogre::uint32> indicesCopy = this->indices;
		size_t numVertices = this->currentVertexIndex;

		// Execute mesh creation on render thread
		GraphicsModule::RenderCommand renderCommand = [this, verticesCopy, indicesCopy, numVertices, wallOriginToUse]()
			{
				this->createWallMeshInternal(verticesCopy, indicesCopy, numVertices, wallOriginToUse);
			};
		NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralWallComponent::createWallMesh");

		// Clear temporary data
		this->vertices.clear();
		this->indices.clear();
	}

	void ProceduralWallComponent::createWallMeshInternal(const std::vector<float>& vertices, const std::vector<Ogre::uint32>& indices, size_t numVertices, const Ogre::Vector3& wallOrigin)
	{
		Ogre::Root* root = Ogre::Root::getSingletonPtr();
		Ogre::RenderSystem* renderSystem = root->getRenderSystem();
		Ogre::VaoManager* vaoManager = renderSystem->getVaoManager();

		Ogre::String meshName = this->gameObjectPtr->getName() + "_Wall_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());

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

		this->wallMesh = Ogre::MeshManager::getSingleton().createManual(meshName, groupName);
		Ogre::SubMesh* subMesh = this->wallMesh->createSubMesh();

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

		// Allocate and convert vertex data
		const size_t vertexDataSize = numVertices * dstFloatsPerVertex * sizeof(float);
		float* vertexData = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(vertexDataSize, Ogre::MEMCATEGORY_GEOMETRY));

		for (size_t i = 0; i < numVertices; ++i)
		{
			size_t srcOffset = i * srcFloatsPerVertex;
			size_t dstOffset = i * dstFloatsPerVertex;

			// Position
			vertexData[dstOffset + 0] = vertices[srcOffset + 0];
			vertexData[dstOffset + 1] = vertices[srcOffset + 1];
			vertexData[dstOffset + 2] = vertices[srcOffset + 2];

			// Normal
			Ogre::Vector3 normal(vertices[srcOffset + 3], vertices[srcOffset + 4], vertices[srcOffset + 5]);
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
			vertexData[dstOffset + 10] = vertices[srcOffset + 6];
			vertexData[dstOffset + 11] = vertices[srcOffset + 7];
		}

		// Create vertex buffer
		Ogre::VertexBufferPacked* vertexBuffer = nullptr;
		try
		{
			vertexBuffer = vaoManager->createVertexBuffer( vertexElements, numVertices, Ogre::BT_IMMUTABLE, vertexData, true);
		}
		catch (Ogre::Exception& e)
		{
			OGRE_FREE_SIMD(vertexData, Ogre::MEMCATEGORY_GEOMETRY);
			throw e;
		}

		// Allocate index data
		const size_t indexDataSize = indices.size() * sizeof(Ogre::uint32);
		Ogre::uint32* indexData = reinterpret_cast<Ogre::uint32*>(OGRE_MALLOC_SIMD(indexDataSize, Ogre::MEMCATEGORY_GEOMETRY));
		memcpy(indexData, indices.data(), indexDataSize);

		// Create index buffer
		Ogre::IndexBufferPacked* indexBuffer = nullptr;
		try
		{
			indexBuffer = vaoManager->createIndexBuffer(Ogre::IndexBufferPacked::IT_32BIT, indices.size(), Ogre::BT_IMMUTABLE, indexData, true);
		}
		catch (Ogre::Exception& e)
		{
			OGRE_FREE_SIMD(indexData, Ogre::MEMCATEGORY_GEOMETRY);
			throw e;
		}

		// Create VAO
		Ogre::VertexBufferPackedVec vertexBuffers;
		vertexBuffers.push_back(vertexBuffer);

		Ogre::VertexArrayObject* vao = vaoManager->createVertexArrayObject(vertexBuffers, indexBuffer, Ogre::OT_TRIANGLE_LIST);

		subMesh->mVao[Ogre::VpNormal].push_back(vao);
		subMesh->mVao[Ogre::VpShadow].push_back(vao);

		// Set bounds
		Ogre::Vector3 minBounds(std::numeric_limits<float>::max());
		Ogre::Vector3 maxBounds(std::numeric_limits<float>::lowest());

		// Iterate through vertices (8 floats per vertex: pos(3) + normal(3) + uv(2))
		const size_t floatsPerVertex = 8;
		for (size_t i = 0; i < numVertices; ++i)
		{
			size_t offset = i * floatsPerVertex;
			Ogre::Vector3 pos(vertices[offset + 0], vertices[offset + 1], vertices[offset + 2]);
			minBounds.makeFloor(pos);
			maxBounds.makeCeil(pos);
		}

		Ogre::Aabb bounds;
		bounds.setExtents(minBounds, maxBounds);
		this->wallMesh->_setBounds(bounds, false);
		this->wallMesh->_setBoundingSphereRadius(bounds.getRadius());

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] Mesh bounds: min(" + Ogre::StringConverter::toString(minBounds) + "), max(" + Ogre::StringConverter::toString(maxBounds) + ")");

		// Create item
		this->wallItem = this->gameObjectPtr->getSceneManager()->createItem(this->wallMesh, this->gameObjectPtr->isDynamic() ? Ogre::SCENE_DYNAMIC : Ogre::SCENE_STATIC);

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] Created wall item");

		// Apply datablock
		Ogre::String dbName = this->wallDatablock->getString();
		if (false == dbName.empty())
		{
			Ogre::HlmsDatablock* datablock = Ogre::Root::getSingleton().getHlmsManager()->getDatablockNoDefault(dbName);
			if (nullptr != datablock)
			{
				this->wallItem->setDatablock(datablock);
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] Applied datablock: " + dbName);
			}
		}

		this->gameObjectPtr->getSceneNode()->setPosition(wallOrigin);
		this->gameObjectPtr->getSceneNode()->attachObject(this->wallItem);
		this->gameObjectPtr->setDoNotDestroyMovableObject(true);
		this->gameObjectPtr->init(this->wallItem);

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] Wall mesh created with " +  Ogre::StringConverter::toString(numVertices) + " vertices, attached to scene");
	}

	void ProceduralWallComponent::destroyWallMesh(void)
	{
		if (nullptr == this->wallItem && nullptr == this->wallMesh)
		{
			return;
		}

		GraphicsModule::RenderCommand renderCommand = [this]()
		{
			if (nullptr != this->wallItem)
			{
				if (this->wallItem->getParentSceneNode())
				{
					this->wallItem->getParentSceneNode()->detachObject(this->wallItem);
				}
				this->gameObjectPtr->getSceneManager()->destroyItem(this->wallItem);
				this->wallItem = nullptr;
				this->gameObjectPtr->nullMovableObject();
			}

			if (this->wallMesh)
			{
				Ogre::MeshManager::getSingleton().remove(this->wallMesh->getHandle());
				this->wallMesh.reset();
			}
		};
		NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralWallComponent::destroyWallMesh");
	}

	void ProceduralWallComponent::destroyPreviewMesh(void)
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
		NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralWallComponent::destroyPreviewMesh");
	}

	void ProceduralWallComponent::updatePreviewMesh(void)
	{
		// Generate preview geometry for current segment
		this->vertices.clear();
		this->indices.clear();
		this->currentVertexIndex = 0;

		// IMPORTANT: Generate preview in LOCAL space relative to startPoint
		WallSegment localSegment;
		localSegment.startPoint = Ogre::Vector3::ZERO; // Start at origin
		localSegment.endPoint = this->currentSegment.endPoint - this->currentSegment.startPoint; // Relative to start
		localSegment.groundHeightStart = 0.0f; // Relative ground height
		localSegment.groundHeightEnd = this->currentSegment.groundHeightEnd - this->currentSegment.groundHeightStart;
		localSegment.hasStartPillar = this->currentSegment.hasStartPillar;
		localSegment.hasEndPillar = this->currentSegment.hasEndPillar;

		WallStyle style = this->getWallStyleEnum();
		switch (style)
		{
		case WallStyle::SOLID:
			this->generateSolidWall(localSegment);
			break;
		case WallStyle::FENCE:
			this->generateFenceWall(localSegment);
			break;
		case WallStyle::BATTLEMENT:
			this->generateBattlementWall(localSegment);
			break;
		case WallStyle::ARCH:
			this->generateArchWall(localSegment);
			break;
		}

		if (true == localSegment.hasStartPillar)
		{
			this->generatePillar(Ogre::Vector3::ZERO, 0.0f);
		}
		if (true == localSegment.hasEndPillar)
		{
			this->generatePillar(localSegment.endPoint, localSegment.groundHeightEnd);
		}

		if (true == this->vertices.empty())
		{
			return;
		}

		// Create preview mesh (simplified, no tangents needed for preview)
		std::vector<float> verticesCopy = this->vertices;
		std::vector<Ogre::uint32> indicesCopy = this->indices;
		size_t numVertices = this->currentVertexIndex;

		// Capture the world position for the preview node
		Ogre::Vector3 previewPosition = this->currentSegment.startPoint;
		previewPosition.y = this->currentSegment.groundHeightStart;

		// Execute on render thread
		GraphicsModule::RenderCommand renderCommand = [this, verticesCopy, indicesCopy, numVertices, previewPosition]()
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

			Ogre::String meshName = "WallPreview_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
			const Ogre::String groupName = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;

			this->previewMesh = Ogre::MeshManager::getSingleton().createManual(meshName, groupName);
			Ogre::SubMesh* subMesh = this->previewMesh->createSubMesh();

			Ogre::VertexElement2Vec vertexElements;
			vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_POSITION));
			vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_NORMAL));
			vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES));

			const size_t vertexDataSize = verticesCopy.size() * sizeof(float);
			float* vertexData = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(vertexDataSize, Ogre::MEMCATEGORY_GEOMETRY));
			memcpy(vertexData, verticesCopy.data(), vertexDataSize);

			Ogre::VertexBufferPacked* vertexBuffer = vaoManager->createVertexBuffer(vertexElements, numVertices, Ogre::BT_IMMUTABLE, vertexData, true);

			const size_t indexDataSize = indicesCopy.size() * sizeof(Ogre::uint32);
			Ogre::uint32* indexData = reinterpret_cast<Ogre::uint32*>(OGRE_MALLOC_SIMD(indexDataSize, Ogre::MEMCATEGORY_GEOMETRY));
			memcpy(indexData, indicesCopy.data(), indexDataSize);

			Ogre::IndexBufferPacked* indexBuffer = vaoManager->createIndexBuffer(Ogre::IndexBufferPacked::IT_32BIT, indicesCopy.size(), Ogre::BT_IMMUTABLE, indexData, true);

			Ogre::VertexBufferPackedVec vertexBuffers;
			vertexBuffers.push_back(vertexBuffer);

			Ogre::VertexArrayObject* vao = vaoManager->createVertexArrayObject(vertexBuffers, indexBuffer, Ogre::OT_TRIANGLE_LIST);

			subMesh->mVao[Ogre::VpNormal].push_back(vao);
			subMesh->mVao[Ogre::VpShadow].push_back(vao);

			// Calculate proper bounds
			Ogre::Vector3 minBounds(std::numeric_limits<float>::max());
			Ogre::Vector3 maxBounds(std::numeric_limits<float>::lowest());

			const size_t floatsPerVertex = 8;
			for (size_t i = 0; i < numVertices; ++i)
			{
				size_t offset = i * floatsPerVertex;
				Ogre::Vector3 pos(verticesCopy[offset + 0], verticesCopy[offset + 1], verticesCopy[offset + 2]);
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
			Ogre::String dbName = this->wallDatablock->getString();
			if (false == dbName.empty())
			{
				Ogre::HlmsDatablock* datablock = Ogre::Root::getSingleton().getHlmsManager()->getDatablockNoDefault(dbName);
				if (datablock)
				{
					this->previewItem->setDatablock(datablock);
				}
			}
		};
		NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "ProceduralWallComponent::updatePreviewMesh");

		this->vertices.clear();
		this->indices.clear();
	}

	// ==================== GEOMETRY HELPERS ====================

	void ProceduralWallComponent::generateSolidWall(const WallSegment& segment)
	{
		Ogre::Vector3 dir = segment.endPoint - segment.startPoint;
		Ogre::Real length = dir.length();
		if (length < 0.001f)
			return;

		dir.normalise();

		// Perpendicular direction for wall thickness
		Ogre::Vector3 right = dir.crossProduct(Ogre::Vector3::UNIT_Y);
		right.normalise();

		Ogre::Real halfThick = this->wallThickness->getReal() * 0.5f;
		Ogre::Real height = this->wallHeight->getReal();
		Ogre::Real pillarSize = this->pillarSize->getReal();
		Ogre::Real halfPillar = pillarSize * 0.5f;

		// Adjust start/end points to avoid pillar overlap
		Ogre::Vector3 adjustedStart = segment.startPoint;
		Ogre::Vector3 adjustedEnd = segment.endPoint;

		if (segment.hasStartPillar)
		{
			adjustedStart = adjustedStart + dir * halfPillar;
		}
		if (segment.hasEndPillar)
		{
			adjustedEnd = adjustedEnd - dir * halfPillar;
		}

		// Recalculate length with adjusted points
		Ogre::Vector3 adjustedDir = adjustedEnd - adjustedStart;
		Ogre::Real adjustedLength = adjustedDir.length();

		// If wall is too short after adjustment, skip it
		if (adjustedLength < 0.001f)
			return;

		// Bottom corners at start
		Ogre::Vector3 bl_start = adjustedStart - right * halfThick;
		Ogre::Vector3 br_start = adjustedStart + right * halfThick;

		// Bottom corners at end
		Ogre::Vector3 bl_end = adjustedEnd - right * halfThick;
		Ogre::Vector3 br_end = adjustedEnd + right * halfThick;

		// Adjust Y for ground height - this creates the slope
		bl_start.y += segment.groundHeightStart;
		br_start.y += segment.groundHeightStart;
		bl_end.y += segment.groundHeightEnd;
		br_end.y += segment.groundHeightEnd;

		// Top corners - maintain height above ground at each point
		Ogre::Vector3 tl_start = bl_start + Ogre::Vector3(0, height, 0);
		Ogre::Vector3 tr_start = br_start + Ogre::Vector3(0, height, 0);
		Ogre::Vector3 tl_end = bl_end + Ogre::Vector3(0, height, 0);
		Ogre::Vector3 tr_end = br_end + Ogre::Vector3(0, height, 0);

		Ogre::Vector2 uvTile = this->uvTiling->getVector2();
		Ogre::Real uLen = adjustedLength * uvTile.x;
		Ogre::Real vHeight = height * uvTile.y;
		Ogre::Real uThick = this->wallThickness->getReal() * uvTile.x;

		// Left face (normal points -right): BL, TL, TR, BR
		this->addQuad(bl_start, tl_start, tl_end, bl_end, -right, uLen, vHeight);

		// Right face (normal points +right): BL, TL, TR, BR
		this->addQuad(br_end, tr_end, tr_start, br_start, right, uLen, vHeight);

		// Top face (normal points up): BL, TL, TR, BR
		this->addQuad(tl_start, tr_start, tr_end, tl_end, Ogre::Vector3::UNIT_Y, uLen, uThick);

		// Start cap (normal points -dir): BL, TL, TR, BR
		this->addQuad(br_start, tr_start, tl_start, bl_start, -dir, uThick, vHeight);

		// End cap (normal points +dir): BL, TL, TR, BR
		this->addQuad(bl_end, tl_end, tr_end, br_end, dir, uThick, vHeight);
	}

	void ProceduralWallComponent::generateFenceWall(const WallSegment& segment)
	{
		// Generate fence posts along the wall
		Ogre::Vector3 dir = segment.endPoint - segment.startPoint;
		Ogre::Real length = dir.length();
		if (length < 0.001f)
			return;

		dir.normalise();
		Ogre::Real spacing = this->fencePostSpacing->getReal();
		int numPosts = static_cast<int>(length / spacing) + 1;
		if (numPosts < 2) numPosts = 2;

		Ogre::Real actualSpacing = length / (numPosts - 1);
		Ogre::Real postSize = this->wallThickness->getReal() * 0.5f;
		Ogre::Real height = this->wallHeight->getReal();
		Ogre::Real railHeight = height * 0.5f;
		Ogre::Real railThickness = postSize * 0.5f;

		// Generate posts
		for (int i = 0; i < numPosts; ++i)
		{
			Ogre::Real t = static_cast<Ogre::Real>(i) / (numPosts - 1);
			Ogre::Vector3 pos = segment.startPoint + dir * (actualSpacing * i);

			// Interpolate ground height along the segment
			Ogre::Real groundH = segment.groundHeightStart * (1.0f - t) + segment.groundHeightEnd * t;
			pos.y = 0.0f;  // Ensure Y is 0 before adding ground height

			this->addBox(
				pos.x - postSize, groundH, pos.z - postSize,
				pos.x + postSize, groundH + height, pos.z + postSize,
				1.0f, height);
		}

		// Generate horizontal rails
		Ogre::Real origThickness = this->wallThickness->getReal();
		Ogre::Real origHeight = this->wallHeight->getReal();

		// Temporarily modify for rail generation
		this->wallThickness->setValue(railThickness);
		this->wallHeight->setValue(railThickness);

		// Top rail
		WallSegment topRail;
		topRail.startPoint = segment.startPoint;
		topRail.endPoint = segment.endPoint;
		topRail.groundHeightStart = segment.groundHeightStart + height - railThickness;
		topRail.groundHeightEnd = segment.groundHeightEnd + height - railThickness;
		topRail.hasStartPillar = false;
		topRail.hasEndPillar = false;
		this->generateSolidWall(topRail);

		// Middle rail
		WallSegment midRail;
		midRail.startPoint = segment.startPoint;
		midRail.endPoint = segment.endPoint;
		midRail.groundHeightStart = segment.groundHeightStart + railHeight;
		midRail.groundHeightEnd = segment.groundHeightEnd + railHeight;
		midRail.hasStartPillar = false;
		midRail.hasEndPillar = false;
		this->generateSolidWall(midRail);

		// Restore original values
		this->wallThickness->setValue(origThickness);
		this->wallHeight->setValue(origHeight);
	}

	void ProceduralWallComponent::generateBattlementWall(const WallSegment& segment)
	{
		// Generate main wall
		WallSegment mainWall = segment;
		Ogre::Real origHeight = this->wallHeight->getReal();
		Ogre::Real battleHeight = this->battlementHeight->getReal();

		this->wallHeight->setValue(origHeight - battleHeight);
		this->generateSolidWall(mainWall);
		this->wallHeight->setValue(origHeight);

		// Generate battlements (merlons)
		Ogre::Vector3 dir = segment.endPoint - segment.startPoint;
		Ogre::Real length = dir.length();
		if (length < 0.001f)
			return;

		dir.normalise();
		Ogre::Vector3 perp = dir.crossProduct(Ogre::Vector3::UNIT_Y);
		perp.normalise();

		Ogre::Real merlonWidth = this->battlementWidth->getReal();
		Ogre::Real gapWidth = merlonWidth; // Same size gaps
		Ogre::Real halfThick = this->wallThickness->getReal() * 0.5f;

		// Calculate how many merlons fit along the wall
		int numMerlons = static_cast<int>(length / (merlonWidth + gapWidth));
		if (numMerlons < 1) numMerlons = 1;

		Ogre::Real actualSpacing = length / numMerlons;

		for (int i = 0; i < numMerlons; ++i)
		{
			// Calculate interpolation factor for ground height
			Ogre::Real t = (i + 0.5f) / numMerlons;

			// Position merlon center along the wall
			Ogre::Vector3 center = segment.startPoint + dir * ((i + 0.5f) * actualSpacing);
			center.y = 0.0f;  // Ensure Y is 0

			// Interpolate ground height
			Ogre::Real groundH = segment.groundHeightStart * (1.0f - t) + segment.groundHeightEnd * t;

			// Calculate merlon corners aligned with wall direction
			Ogre::Real halfMerlon = merlonWidth * 0.5f;

			Ogre::Vector3 merlonStart = center - dir * halfMerlon;
			Ogre::Vector3 merlonEnd = center + dir * halfMerlon;

			// Expand perpendicular to wall thickness
			Ogre::Vector3 corner1 = merlonStart - perp * halfThick;
			Ogre::Vector3 corner2 = merlonStart + perp * halfThick;
			Ogre::Vector3 corner3 = merlonEnd - perp * halfThick;
			Ogre::Vector3 corner4 = merlonEnd + perp * halfThick;

			// Find bounding box
			Ogre::Real minX = std::min({ corner1.x, corner2.x, corner3.x, corner4.x });
			Ogre::Real maxX = std::max({ corner1.x, corner2.x, corner3.x, corner4.x });
			Ogre::Real minZ = std::min({ corner1.z, corner2.z, corner3.z, corner4.z });
			Ogre::Real maxZ = std::max({ corner1.z, corner2.z, corner3.z, corner4.z });

			this->addBox(
				minX,
				groundH + origHeight - battleHeight,
				minZ,
				maxX,
				groundH + origHeight,
				maxZ,
				merlonWidth, battleHeight);
		}
	}

	void ProceduralWallComponent::generateArchWall(const WallSegment& segment)
	{
		Ogre::Vector3 dir = segment.endPoint - segment.startPoint;
		Ogre::Real length = dir.length();
		if (length < 0.001f)
			return;

		dir.normalise();
		Ogre::Vector3 right = dir.crossProduct(Ogre::Vector3::UNIT_Y);
		right.normalise();

		Ogre::Real halfThick = this->wallThickness->getReal() * 0.5f;
		Ogre::Real height = this->wallHeight->getReal();

		// Arch parameters
		Ogre::Real archWidth = length * 0.4f;
		Ogre::Real archHeight = height * 0.6f;
		Ogre::Real archCenterT = 0.5f;

		// Calculate arch boundaries
		Ogre::Real archStartT = archCenterT - (archWidth / length) * 0.5f;
		Ogre::Real archEndT = archCenterT + (archWidth / length) * 0.5f;

		// Calculate ground height at arch positions
		Ogre::Real groundHeightArchStart = segment.groundHeightStart * (1.0f - archStartT) + segment.groundHeightEnd * archStartT;
		Ogre::Real groundHeightArchEnd = segment.groundHeightStart * (1.0f - archEndT) + segment.groundHeightEnd * archEndT;
		Ogre::Real groundHeightArchCenter = segment.groundHeightStart * (1.0f - archCenterT) + segment.groundHeightEnd * archCenterT;

		// Left wall segment (before arch)
		if (archStartT > 0.01f)
		{
			WallSegment leftSeg;
			leftSeg.startPoint = segment.startPoint;
			leftSeg.endPoint = segment.startPoint + dir * (length * archStartT);
			leftSeg.endPoint.y = 0.0f;
			leftSeg.groundHeightStart = segment.groundHeightStart;
			leftSeg.groundHeightEnd = groundHeightArchStart;
			leftSeg.hasStartPillar = false;
			leftSeg.hasEndPillar = false;
			this->generateSolidWall(leftSeg);
		}

		// Right wall segment (after arch)
		if (archEndT < 0.99f)
		{
			WallSegment rightSeg;
			rightSeg.startPoint = segment.startPoint + dir * (length * archEndT);
			rightSeg.startPoint.y = 0.0f;
			rightSeg.endPoint = segment.endPoint;
			rightSeg.groundHeightStart = groundHeightArchEnd;
			rightSeg.groundHeightEnd = segment.groundHeightEnd;
			rightSeg.hasStartPillar = false;
			rightSeg.hasEndPillar = false;
			this->generateSolidWall(rightSeg);
		}

		// Top segment (above arch)
		Ogre::Vector3 archStart = segment.startPoint + dir * (length * archStartT);
		archStart.y = 0.0f;
		Ogre::Vector3 archEnd = segment.startPoint + dir * (length * archEndT);
		archEnd.y = 0.0f;

		WallSegment topSeg;
		topSeg.startPoint = archStart;
		topSeg.endPoint = archEnd;
		topSeg.groundHeightStart = groundHeightArchStart + archHeight;
		topSeg.groundHeightEnd = groundHeightArchEnd + archHeight;
		topSeg.hasStartPillar = false;
		topSeg.hasEndPillar = false;

		// Temporarily adjust height for top segment
		Ogre::Real origHeight = this->wallHeight->getReal();
		this->wallHeight->setValue(height - archHeight);
		this->generateSolidWall(topSeg);
		this->wallHeight->setValue(origHeight);

		// Generate arch columns on sides
		Ogre::Real columnWidth = this->wallThickness->getReal();

		// Left column
		this->addBox(
			archStart.x - right.x * halfThick,
			groundHeightArchStart,
			archStart.z - right.z * halfThick,
			archStart.x + right.x * halfThick,
			groundHeightArchStart + archHeight,
			archStart.z + right.z * halfThick,
			columnWidth, archHeight);

		// Right column
		this->addBox(
			archEnd.x - right.x * halfThick,
			groundHeightArchEnd,
			archEnd.z - right.z * halfThick,
			archEnd.x + right.x * halfThick,
			groundHeightArchEnd + archHeight,
			archEnd.z + right.z * halfThick,
			columnWidth, archHeight);
	}

	void ProceduralWallComponent::generatePillar(const Ogre::Vector3& localOffset, Ogre::Real groundHeightOffset)
	{
		Ogre::Real size = this->pillarSize->getReal();
		Ogre::Real halfSize = size * 0.5f;
		Ogre::Real height = this->wallHeight->getReal();

		Ogre::Real minX = localOffset.x - halfSize;
		Ogre::Real maxX = localOffset.x + halfSize;
		Ogre::Real minY = groundHeightOffset;
		Ogre::Real maxY = groundHeightOffset + height;
		Ogre::Real minZ = localOffset.z - halfSize;
		Ogre::Real maxZ = localOffset.z + halfSize;

		Ogre::Vector2 uvTile = this->uvTiling->getVector2();
		Ogre::Real uTile = size * uvTile.x;
		Ogre::Real vTile = height * uvTile.y;

		// Front face (-Z): BL, TL, TR, BR
		this->addQuad(
			Ogre::Vector3(minX, minY, minZ),  // BL
			Ogre::Vector3(minX, maxY, minZ),  // TL
			Ogre::Vector3(maxX, maxY, minZ),  // TR
			Ogre::Vector3(maxX, minY, minZ),  // BR
			Ogre::Vector3::NEGATIVE_UNIT_Z, uTile, vTile);

		// Back face (+Z): BL, TL, TR, BR (from back view)
		this->addQuad(
			Ogre::Vector3(maxX, minY, maxZ),  // BL (from back)
			Ogre::Vector3(maxX, maxY, maxZ),  // TL
			Ogre::Vector3(minX, maxY, maxZ),  // TR
			Ogre::Vector3(minX, minY, maxZ),  // BR
			Ogre::Vector3::UNIT_Z, uTile, vTile);

		// Left face (-X): BL, TL, TR, BR
		this->addQuad(
			Ogre::Vector3(minX, minY, maxZ),  // BL (back)
			Ogre::Vector3(minX, maxY, maxZ),  // TL
			Ogre::Vector3(minX, maxY, minZ),  // TR (front)
			Ogre::Vector3(minX, minY, minZ),  // BR
			Ogre::Vector3::NEGATIVE_UNIT_X, uTile, vTile);

		// Right face (+X): BL, TL, TR, BR
		this->addQuad(
			Ogre::Vector3(maxX, minY, minZ),  // BL (front)
			Ogre::Vector3(maxX, maxY, minZ),  // TL
			Ogre::Vector3(maxX, maxY, maxZ),  // TR (back)
			Ogre::Vector3(maxX, minY, maxZ),  // BR
			Ogre::Vector3::UNIT_X, uTile, vTile);

		// Top face (+Y): BL, TL, TR, BR
		this->addQuad(
			Ogre::Vector3(minX, maxY, minZ),  // BL (front-left)
			Ogre::Vector3(minX, maxY, maxZ),  // TL (back-left)
			Ogre::Vector3(maxX, maxY, maxZ),  // TR (back-right)
			Ogre::Vector3(maxX, maxY, minZ),  // BR (front-right)
			Ogre::Vector3::UNIT_Y, uTile, uTile);
	}

	void ProceduralWallComponent::generateSolidWallWithSubdivision(const WallSegment& segment, int subdivisions)
	{
		if (subdivisions <= 1)
		{
			this->generateSolidWall(segment);
			return;
		}

		Ogre::Vector3 dir = segment.endPoint - segment.startPoint;
		Ogre::Real length = dir.length();
		Ogre::Real segmentLength = length / subdivisions;

		for (int i = 0; i < subdivisions; ++i)
		{
			Ogre::Real t0 = static_cast<Ogre::Real>(i) / subdivisions;
			Ogre::Real t1 = static_cast<Ogre::Real>(i + 1) / subdivisions;

			WallSegment subSegment;
			subSegment.startPoint = segment.startPoint + dir * (t0 * length / dir.length());
			subSegment.endPoint = segment.startPoint + dir * (t1 * length / dir.length());

			// Sample ground height at each point for accurate terrain following
			if (this->adaptToGround->getBool())
			{
				subSegment.groundHeightStart = this->getGroundHeight(subSegment.startPoint);
				subSegment.groundHeightEnd = this->getGroundHeight(subSegment.endPoint);
			}
			else
			{
				subSegment.groundHeightStart = segment.groundHeightStart * (1.0f - t0) + segment.groundHeightEnd * t0;
				subSegment.groundHeightEnd = segment.groundHeightStart * (1.0f - t1) + segment.groundHeightEnd * t1;
			}

			subSegment.hasStartPillar = false;
			subSegment.hasEndPillar = false;

			this->generateSolidWall(subSegment);
		}
	}

	void ProceduralWallComponent::addBox(Ogre::Real minX, Ogre::Real minY, Ogre::Real minZ, Ogre::Real maxX, Ogre::Real maxY, Ogre::Real maxZ, Ogre::Real uTile, Ogre::Real vTile)
	{
		Ogre::Vector2 uvTile = this->uvTiling->getVector2();
		uTile *= uvTile.x;
		vTile *= uvTile.y;

		// Front face (-Z): BL, TL, TR, BR
		this->addQuad(
			Ogre::Vector3(minX, minY, minZ),
			Ogre::Vector3(minX, maxY, minZ),
			Ogre::Vector3(maxX, maxY, minZ),
			Ogre::Vector3(maxX, minY, minZ),
			Ogre::Vector3::NEGATIVE_UNIT_Z, uTile, vTile);

		// Back face (+Z): BL, TL, TR, BR
		this->addQuad(
			Ogre::Vector3(maxX, minY, maxZ),
			Ogre::Vector3(maxX, maxY, maxZ),
			Ogre::Vector3(minX, maxY, maxZ),
			Ogre::Vector3(minX, minY, maxZ),
			Ogre::Vector3::UNIT_Z, uTile, vTile);

		// Left face (-X): BL, TL, TR, BR
		this->addQuad(
			Ogre::Vector3(minX, minY, maxZ),
			Ogre::Vector3(minX, maxY, maxZ),
			Ogre::Vector3(minX, maxY, minZ),
			Ogre::Vector3(minX, minY, minZ),
			Ogre::Vector3::NEGATIVE_UNIT_X, uTile, vTile);

		// Right face (+X): BL, TL, TR, BR
		this->addQuad(
			Ogre::Vector3(maxX, minY, minZ),
			Ogre::Vector3(maxX, maxY, minZ),
			Ogre::Vector3(maxX, maxY, maxZ),
			Ogre::Vector3(maxX, minY, maxZ),
			Ogre::Vector3::UNIT_X, uTile, vTile);

		// Top face (+Y): BL, TL, TR, BR
		this->addQuad(
			Ogre::Vector3(minX, maxY, minZ),
			Ogre::Vector3(minX, maxY, maxZ),
			Ogre::Vector3(maxX, maxY, maxZ),
			Ogre::Vector3(maxX, maxY, minZ),
			Ogre::Vector3::UNIT_Y, uTile, uTile);
	}

	void ProceduralWallComponent::addQuad(const Ogre::Vector3& v0, const Ogre::Vector3& v1, const Ogre::Vector3& v2, const Ogre::Vector3& v3, const Ogre::Vector3& normal, Ogre::Real uTile, Ogre::Real vTile)
	{
		Ogre::uint32 baseIndex = this->currentVertexIndex;

		// Vertex format: pos(3) + normal(3) + uv(2) = 8 floats
		// Expected vertex order: v0=BL, v1=TL, v2=TR, v3=BR

		// Vertex 0 - bottom-left (UV 0,0)
		this->vertices.push_back(v0.x);
		this->vertices.push_back(v0.y);
		this->vertices.push_back(v0.z);
		this->vertices.push_back(normal.x);
		this->vertices.push_back(normal.y);
		this->vertices.push_back(normal.z);
		this->vertices.push_back(0.0f);
		this->vertices.push_back(0.0f);

		// Vertex 1 - top-left (UV 0,vTile)
		this->vertices.push_back(v1.x);
		this->vertices.push_back(v1.y);
		this->vertices.push_back(v1.z);
		this->vertices.push_back(normal.x);
		this->vertices.push_back(normal.y);
		this->vertices.push_back(normal.z);
		this->vertices.push_back(0.0f);
		this->vertices.push_back(vTile);

		// Vertex 2 - top-right (UV uTile,vTile)
		this->vertices.push_back(v2.x);
		this->vertices.push_back(v2.y);
		this->vertices.push_back(v2.z);
		this->vertices.push_back(normal.x);
		this->vertices.push_back(normal.y);
		this->vertices.push_back(normal.z);
		this->vertices.push_back(uTile);
		this->vertices.push_back(vTile);

		// Vertex 3 - bottom-right (UV uTile,0)
		this->vertices.push_back(v3.x);
		this->vertices.push_back(v3.y);
		this->vertices.push_back(v3.z);
		this->vertices.push_back(normal.x);
		this->vertices.push_back(normal.y);
		this->vertices.push_back(normal.z);
		this->vertices.push_back(uTile);
		this->vertices.push_back(0.0f);

		this->currentVertexIndex += 4;

		// Two triangles with counter-clockwise winding: 0-1-2 and 0-2-3
		// (BL,TL,TR) and (BL,TR,BR)
		this->indices.push_back(baseIndex + 0);
		this->indices.push_back(baseIndex + 1);
		this->indices.push_back(baseIndex + 2);

		this->indices.push_back(baseIndex + 0);
		this->indices.push_back(baseIndex + 2);
		this->indices.push_back(baseIndex + 3);
	}

	Ogre::Vector3 ProceduralWallComponent::snapToGridFunc(const Ogre::Vector3& position)
	{
		Ogre::Real gridSz = this->gridSize->getReal();
		return Ogre::Vector3(std::round(position.x / gridSz) * gridSz, position.y, std::round(position.z / gridSz) * gridSz);
	}

	ProceduralWallComponent::WallStyle ProceduralWallComponent::getWallStyleEnum(void) const
	{
		Ogre::String style = this->wallStyle->getListSelectedValue();
		if (style == "Fence")
		{
			return WallStyle::FENCE;
		}
		if (style == "Battlement")
		{
			return WallStyle::BATTLEMENT;
		}
		if (style == "Arch")
		{
			return WallStyle::ARCH;
		}
		return WallStyle::SOLID;
	}

	// ==================== ATTRIBUTE SETTERS/GETTERS ====================

	void ProceduralWallComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
	}

	bool ProceduralWallComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void ProceduralWallComponent::setWallHeight(Ogre::Real height)
	{
		this->wallHeight->setValue(height);

		// If wall exists, rebuild it
		if (false == this->wallSegments.empty())
		{
			this->rebuildMesh();
		}
	}

	Ogre::Real ProceduralWallComponent::getWallHeight(void) const
	{
		return this->wallHeight->getReal();
	}

	void ProceduralWallComponent::setWallThickness(Ogre::Real thickness)
	{
		this->wallThickness->setValue(thickness);

		if (false == this->wallSegments.empty())
		{
			this->rebuildMesh();
		}
	}

	Ogre::Real ProceduralWallComponent::getWallThickness(void) const
	{
		return this->wallThickness->getReal();
	}

	void ProceduralWallComponent::setWallStyle(const Ogre::String& style)
	{
		this->wallStyle->setListSelectedValue(style);

		if (false == this->wallSegments.empty())
		{
			this->rebuildMesh();
		}
	}

	Ogre::String ProceduralWallComponent::getWallStyle(void) const
	{
		return this->wallStyle->getListSelectedValue();
	}

	void ProceduralWallComponent::setSnapToGrid(bool snap)
	{
		this->snapToGrid->setValue(snap);
	}

	bool ProceduralWallComponent::getSnapToGrid(void) const
	{
		return this->snapToGrid->getBool();
	}

	void ProceduralWallComponent::setGridSize(Ogre::Real size)
	{
		this->gridSize->setValue(size);
	}

	Ogre::Real ProceduralWallComponent::getGridSize(void) const
	{
		return this->gridSize->getReal();
	}

	void ProceduralWallComponent::setAdaptToGround(bool adapt)
	{
		this->adaptToGround->setValue(adapt);
	}

	bool ProceduralWallComponent::getAdaptToGround(void) const
	{
		return this->adaptToGround->getBool();
	}

	void ProceduralWallComponent::setCreatePillars(bool create)
	{
		this->createPillars->setValue(create);

		if (false == this->wallSegments.empty())
		{
			// Update pillar flags for all segments
			for (auto& seg : this->wallSegments)
			{
				seg.hasStartPillar = create;
				seg.hasEndPillar = create;
			}
			this->rebuildMesh();
		}
	}

	bool ProceduralWallComponent::getCreatePillars(void) const
	{
		return this->createPillars->getBool();
	}

	void ProceduralWallComponent::setPillarSize(Ogre::Real size)
	{
		this->pillarSize->setValue(size);

		if (false == this->wallSegments.empty())
		{
			this->rebuildMesh();
		}
	}

	Ogre::Real ProceduralWallComponent::getPillarSize(void) const
	{
		return this->pillarSize->getReal();
	}

	void ProceduralWallComponent::setWallDatablock(const Ogre::String& datablock)
	{
		this->wallDatablock->setValue(datablock);

		// Apply datablock immediately if wall exists
		if (this->wallItem && !datablock.empty())
		{
			GraphicsModule::RenderCommand renderCommand = [this, datablock]()
			{
				Ogre::HlmsDatablock* wallDB = Ogre::Root::getSingleton().getHlmsManager()->getDatablockNoDefault(datablock);
				if (nullptr != wallDB)
				{
					this->wallItem->setDatablock(wallDB);
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] Applied wall datablock: " + datablock);
				}
			};
			NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "ProceduralWallComponent::setWallDatablock");
		}
	}

	Ogre::String ProceduralWallComponent::getWallDatablock(void) const
	{
		return this->wallDatablock->getString();
	}

	void ProceduralWallComponent::setPillarDatablock(const Ogre::String& datablock)
	{
		this->pillarDatablock->setValue(datablock);

		// Pillars are part of the main mesh, so changing pillar datablock requires rebuild
		// OR we could create separate items for pillars - for now, just note this
		// If you want separate pillar materials, you'd need to split into multiple submeshes
	}

	Ogre::String ProceduralWallComponent::getPillarDatablock(void) const
	{
		return this->pillarDatablock->getString();
	}

	void ProceduralWallComponent::setUVTiling(const Ogre::Vector2& tiling)
	{
		this->uvTiling->setValue(tiling);

		if (false == this->wallSegments.empty())
		{
			this->rebuildMesh();
		}
	}

	Ogre::Vector2 ProceduralWallComponent::getUVTiling(void) const
	{
		return this->uvTiling->getVector2();
	}

	void ProceduralWallComponent::setFencePostSpacing(Ogre::Real spacing)
	{
		this->fencePostSpacing->setValue(spacing);

		if (false == this->wallSegments.empty() && this->getWallStyle() == "Fence")
		{
			this->rebuildMesh();
		}
	}

	void ProceduralWallComponent::setBattlementWidth(Ogre::Real width)
	{
		this->battlementWidth->setValue(width);

		if (false == this->wallSegments.empty() && this->getWallStyle() == "Battlement")
		{
			this->rebuildMesh();
		}
	}

	void ProceduralWallComponent::setBattlementHeight(Ogre::Real height)
	{
		this->battlementHeight->setValue(height);

		if (false == this->wallSegments.empty() && this->getWallStyle() == "Battlement")
		{
			this->rebuildMesh();
		}
	}

} // namespace NOWA
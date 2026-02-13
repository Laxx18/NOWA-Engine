#include "NOWAPrecompiled.h"
#include "ProceduralWallComponent.h"
#include "editor/EditorManager.h"
#include "gameObject/GameObjectController.h"
#include "main/AppStateManager.h"
#include "main/Core.h"
#include "main/InputDeviceCore.h"
#include "utilities/MathHelper.h"
#include "utilities/XMLConverter.h"

#include "OgreHlmsManager.h"
#include "OgreHlmsPbs.h"
#include "OgreHlmsPbsDatablock.h"
#include "OgreItem.h"
#include "OgreMesh2.h"
#include "OgreMesh2Serializer.h"
#include "OgreMeshManager2.h"
#include "OgreSubMesh2.h"
#include "Vao/OgreVaoManager.h"
#include "Vao/OgreVertexArrayObject.h"

#include <filesystem>
#include <system_error>

#include "OgreAbiUtils.h"

namespace NOWA
{
using namespace rapidxml;

ProceduralWallComponent::ProceduralWallComponent() :
    WallComponentBase(),
    name("ProceduralWallComponent"),
    buildState(BuildState::IDLE),
    isEditorMeshModifyMode(false),
    isSelected(false),
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
    groundQuery(nullptr),
    cachedNumWallVertices(0),
    cachedNumPillarVertices(0),
    cachedWallOrigin(Ogre::Vector3::ZERO),
    originPositionSet(false)
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
    this->wallDatablock = new Variant(ProceduralWallComponent::AttrWallDatablock(), Ogre::String("proceduralWall1"), this->attributes);
    this->pillarDatablock = new Variant(ProceduralWallComponent::AttrPillarDatablock(), Ogre::String("proceduralWall1"), this->attributes);
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

    AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &ProceduralWallComponent::handleMeshModifyMode), NOWA::EventDataEditorMode::getStaticEventType());
    AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &ProceduralWallComponent::handleGameObjectSelected), NOWA::EventDataGameObjectSelected::getStaticEventType());
    AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &ProceduralWallComponent::handleComponentManuallyDeleted), EventDataDeleteComponent::getStaticEventType());

    // Create raycast query for ground detection
    this->groundQuery = this->gameObjectPtr->getSceneManager()->createRayQuery(Ogre::Ray(), GameObjectController::ALL_CATEGORIES_ID);
    this->groundQuery->setSortByDistance(true);

    // Setup fallback ground plane at Y=0
    this->groundPlane = Ogre::Plane(Ogre::Vector3::UNIT_Y, 0.0f);

    // Create preview scene node
    this->previewNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode();

    // Load wall data from file
    if (true == this->loadWallDataFromFile())
    {
        // If we successfully loaded segments, rebuild the mesh
        if (!this->wallSegments.empty())
        {
            this->rebuildMesh();

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] Successfully loaded and rebuilt wall with " + Ogre::StringConverter::toString(this->wallSegments.size()) + " segments");
        }
    }

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

void ProceduralWallComponent::onAddComponent(void)
{
    boost::shared_ptr<EventDataEditorMode> eventDataEditorMode(new EventDataEditorMode(EditorManager::EDITOR_MESH_MODIFY_MODE));
    NOWA::AppStateManager::getSingletonPtr()->getEventManager()->threadSafeQueueEvent(eventDataEditorMode);

    this->isSelected = true;
    this->addInputListener();
}

void ProceduralWallComponent::onRemoveComponent(void)
{
    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] Destructor called");

    AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &ProceduralWallComponent::handleMeshModifyMode), NOWA::EventDataEditorMode::getStaticEventType());
    AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &ProceduralWallComponent::handleGameObjectSelected), NOWA::EventDataGameObjectSelected::getStaticEventType());
    AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &ProceduralWallComponent::handleComponentManuallyDeleted), EventDataDeleteComponent::getStaticEventType());

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
    propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
    propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralWallComponent::AttrActivated().c_str())));
    propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
    propertiesXML->append_node(propertyXML);

    propertyXML = doc.allocate_node(node_element, "property");
    propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
    propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralWallComponent::AttrWallHeight().c_str())));
    propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->wallHeight->getReal())));
    propertiesXML->append_node(propertyXML);

    propertyXML = doc.allocate_node(node_element, "property");
    propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
    propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralWallComponent::AttrWallThickness().c_str())));
    propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->wallThickness->getReal())));
    propertiesXML->append_node(propertyXML);

    propertyXML = doc.allocate_node(node_element, "property");
    propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
    propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralWallComponent::AttrWallStyle().c_str())));
    propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->wallStyle->getListSelectedValue())));
    propertiesXML->append_node(propertyXML);

    propertyXML = doc.allocate_node(node_element, "property");
    propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
    propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralWallComponent::AttrSnapToGrid().c_str())));
    propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->snapToGrid->getBool())));
    propertiesXML->append_node(propertyXML);

    propertyXML = doc.allocate_node(node_element, "property");
    propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
    propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralWallComponent::AttrGridSize().c_str())));
    propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->gridSize->getReal())));
    propertiesXML->append_node(propertyXML);

    propertyXML = doc.allocate_node(node_element, "property");
    propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
    propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralWallComponent::AttrAdaptToGround().c_str())));
    propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->adaptToGround->getBool())));
    propertiesXML->append_node(propertyXML);

    propertyXML = doc.allocate_node(node_element, "property");
    propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
    propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralWallComponent::AttrCreatePillars().c_str())));
    propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->createPillars->getBool())));
    propertiesXML->append_node(propertyXML);

    propertyXML = doc.allocate_node(node_element, "property");
    propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
    propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralWallComponent::AttrPillarSize().c_str())));
    propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->pillarSize->getReal())));
    propertiesXML->append_node(propertyXML);

    propertyXML = doc.allocate_node(node_element, "property");
    propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
    propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralWallComponent::AttrWallDatablock().c_str())));
    propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->wallDatablock->getString())));
    propertiesXML->append_node(propertyXML);

    propertyXML = doc.allocate_node(node_element, "property");
    propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
    propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralWallComponent::AttrPillarDatablock().c_str())));
    propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->pillarDatablock->getString())));
    propertiesXML->append_node(propertyXML);

    propertyXML = doc.allocate_node(node_element, "property");
    propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
    propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralWallComponent::AttrUVTiling().c_str())));
    propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->uvTiling->getVector2())));
    propertiesXML->append_node(propertyXML);

    propertyXML = doc.allocate_node(node_element, "property");
    propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
    propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralWallComponent::AttrFencePostSpacing().c_str())));
    propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->fencePostSpacing->getReal())));
    propertiesXML->append_node(propertyXML);

    propertyXML = doc.allocate_node(node_element, "property");
    propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
    propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralWallComponent::AttrBattlementWidth().c_str())));
    propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->battlementWidth->getReal())));
    propertiesXML->append_node(propertyXML);

    propertyXML = doc.allocate_node(node_element, "property");
    propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
    propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralWallComponent::AttrBattlementHeight().c_str())));
    propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->battlementHeight->getReal())));
    propertiesXML->append_node(propertyXML);

    this->saveWallDataToFile();
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
        return true;
    }

    if (id != OIS::MB_Left)
    {
        return true;
    }

    // Check if clicking on GUI
    if (MyGUI::InputManager::getInstance().getMouseFocusWidget() != nullptr)
    {
        return true;
    }

    Ogre::Vector3 internalHitPoint = Ogre::Vector3::ZERO;
    Ogre::MovableObject* hitMovableObject = nullptr;
    Ogre::Real closestDistance = 0.0f;
    Ogre::Vector3 normal = Ogre::Vector3::ZERO;

    // Build exclusion list - exclude our own wall and preview items
    std::vector<Ogre::MovableObject*> excludeMovableObjects;

    Ogre::Camera* camera = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
    if (nullptr == camera)
    {
        return true;
    }

    // Check if mouse hit already created wall, -> then skip
    const OIS::MouseState& ms = NOWA::InputDeviceCore::getSingletonPtr()->getMouse()->getMouseState();
    bool hitFound = MathHelper::getInstance()->getRaycastFromPoint(ms.X.abs, ms.Y.abs, camera, Core::getSingletonPtr()->getOgreRenderWindow(), this->groundQuery, internalHitPoint, (size_t&)hitMovableObject, closestDistance, normal,
                                                                   &excludeMovableObjects, false);

    if (true == hitFound && this->buildState == BuildState::IDLE)
    {
        if (this->wallItem == hitMovableObject)
        {
            return true;
        }
        if (this->previewItem == hitMovableObject)
        {
            return true;
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
            // **NEW: Check if shift is held and we have a loaded endpoint**
            if (this->isShiftPressed && this->hasLoadedWallEndpoint)
            {
                // Continue from loaded wall endpoint
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] Continuing from loaded endpoint");

                // Start wall at the exact loaded endpoint
                this->currentSegment.startPoint = this->loadedWallEndpoint;
                this->currentSegment.startPoint.y = 0.0f;
                this->currentSegment.endPoint = this->loadedWallEndpoint;
                this->currentSegment.endPoint.y = 0.0f;
                this->currentSegment.groundHeightStart = this->loadedWallEndpointHeight;
                this->currentSegment.hasStartPillar = false; // Connected to existing wall
                this->currentSegment.hasEndPillar = this->createPillars->getBool();

                this->buildState = BuildState::DRAGGING;
                this->lastValidPosition = this->loadedWallEndpoint;

                // Clear the flag
                this->hasLoadedWallEndpoint = false;

                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] Started continuation from: " + Ogre::StringConverter::toString(this->loadedWallEndpoint));
            }
            else
            {
                // Normal: start new wall at mouse position
                this->startWallPlacement(hitPosition);

                // Clear continuation flag
                this->hasLoadedWallEndpoint = false;
            }
        }
        else if (this->buildState == BuildState::DRAGGING)
        {
            this->confirmWall();
        }

        return false;
    }

    return true;
}

bool ProceduralWallComponent::mouseMoved(const OIS::MouseEvent& evt)
{
    if (false == this->activated->getBool())
    {
        return true;
    }

    // Only update preview if we're actively dragging
    if (this->buildState != BuildState::DRAGGING)
    {
        return true;
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

        // Constrain to axis if ctrl is held
        if (true == this->isCtrlPressed)
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

    return true; // not handled -> bubble
}

bool ProceduralWallComponent::mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
{
    if (false == this->activated->getBool())
    {
        return true;
    }

    if (id == OIS::MB_Right)
    {
        // Cancel current wall segment in progress
        this->cancelWall();

        // Remove listeners - user wants to stop building
        this->removeInputListener();

        return false;
    }

    return true; // not handled -> bubble
}

bool ProceduralWallComponent::keyPressed(const OIS::KeyEvent& evt)
{
    if (false == this->activated->getBool())
    {
        return true; // bubble when not active
    }

    if (evt.key == OIS::KC_LSHIFT || evt.key == OIS::KC_RSHIFT)
    {
        this->isShiftPressed = true;
        return false; // consume (optional)
    }
    else if (evt.key == OIS::KC_LCONTROL || evt.key == OIS::KC_RCONTROL)
    {
        this->isCtrlPressed = true;
        return false; // consume (optional)
    }
    else if (evt.key == OIS::KC_ESCAPE)
    {
        this->cancelWall();
        this->removeInputListener();
        return false; // consume -> STOP DesignState ESC
    }
    else if (evt.key == OIS::KC_Z && this->isCtrlPressed)
    {
        this->removeLastSegment();
        return false; // consume
    }

    return true; // not handled -> bubble
}

bool ProceduralWallComponent::keyReleased(const OIS::KeyEvent& evt)
{
    if (false == this->activated->getBool())
    {
        return true;
    }

    if (evt.key == OIS::KC_LSHIFT || evt.key == OIS::KC_RSHIFT)
    {
        // cancelWall does it
        // this->isShiftPressed = false;
    }
    else if (evt.key == OIS::KC_LCONTROL || evt.key == OIS::KC_RCONTROL)
    {
        this->isCtrlPressed = false;
    }

    return false; // handled -> do not bubble
}

// ==================== WALL BUILDING API ====================

void ProceduralWallComponent::startWallPlacement(const Ogre::Vector3& worldPosition)
{
    // Set wall origin on first placement
    if (false == this->hasWallOrigin)
    {
        this->wallOrigin = worldPosition;
        // Get actual terrain height for origin
        if (this->adaptToGround->getBool())
        {
            this->wallOrigin.y = this->getGroundHeight(worldPosition);
        }
        else
        {
            this->wallOrigin.y = worldPosition.y;
        }
        this->hasWallOrigin = true;

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] Set wall origin: " + Ogre::StringConverter::toString(this->wallOrigin));
    }

    this->currentSegment.startPoint = worldPosition;
    this->currentSegment.startPoint.y = 0.0f;
    this->currentSegment.endPoint = worldPosition;
    this->currentSegment.endPoint.y = 0.0f;

    // Get ground heights at start position
    if (this->adaptToGround->getBool())
    {
        this->currentSegment.groundHeightStart = this->getGroundHeight(worldPosition);
    }
    else
    {
        this->currentSegment.groundHeightStart = worldPosition.y;
    }

    this->currentSegment.hasStartPillar = this->createPillars->getBool();
    this->currentSegment.hasEndPillar = this->createPillars->getBool();

    this->buildState = BuildState::DRAGGING;
    this->lastValidPosition = worldPosition;

    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] Started wall placement at: " + Ogre::StringConverter::toString(worldPosition) +
                                                                           ", ground height: " + Ogre::StringConverter::toString(this->currentSegment.groundHeightStart));
}

void ProceduralWallComponent::updateWallPreview(const Ogre::Vector3& worldPosition)
{
    this->currentSegment.endPoint = worldPosition;
    this->currentSegment.endPoint.y = 0.0f;

    // Get ground height at end position
    if (this->adaptToGround->getBool())
    {
        this->currentSegment.groundHeightEnd = this->getGroundHeight(worldPosition);
    }
    else
    {
        this->currentSegment.groundHeightEnd = worldPosition.y;
    }

    this->lastValidPosition = worldPosition;
    this->updatePreviewMesh();

    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
                                                    "[ProceduralWallComponent] Updated preview to: " + Ogre::StringConverter::toString(worldPosition) + ", ground height: " + Ogre::StringConverter::toString(this->currentSegment.groundHeightEnd));
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

    // Remember if we're chaining BEFORE we modify anything
    bool shouldChain = this->isShiftPressed;
    Ogre::Vector3 chainEndPos = this->currentSegment.endPoint;

    // ---- UNDO: Begin transaction ----
    boost::shared_ptr<NOWA::EventDataCommandTransactionBegin> eventDataUndoBegin(new NOWA::EventDataCommandTransactionBegin("Add Wall Segment"));
    NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataUndoBegin);

    // ---- UNDO: Capture state BEFORE modification ----
    std::vector<unsigned char> oldData = this->getWallData();

    // Add segment
    this->wallSegments.push_back(this->currentSegment);

    // Clean up preview BEFORE rebuilding final mesh
    this->destroyPreviewMesh();

    // Reset preview node position
    if (this->previewNode)
    {
        this->previewNode->setPosition(Ogre::Vector3::ZERO);
    }

    this->rebuildMesh();

    // ---- UNDO: Capture state AFTER modification, fire event ----
    std::vector<unsigned char> newData = this->getWallData();

    boost::shared_ptr<EventDataWallModifyEnd> eventDataWallModifyEnd(new EventDataWallModifyEnd(oldData, newData, this->gameObjectPtr->getId()));
    NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataWallModifyEnd);

    boost::shared_ptr<NOWA::EventDataCommandTransactionEnd> eventDataUndoEnd(new NOWA::EventDataCommandTransactionEnd());
    NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataUndoEnd);

    // NOW handle shift-chaining using the saved state
    if (shouldChain)
    {
        // Start next segment from END of current segment
        this->startWallPlacement(chainEndPos);

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] Starting chained segment from: " + Ogre::StringConverter::toString(chainEndPos));
    }
    else
    {
        // Wall building complete - remove listeners
        this->buildState = BuildState::IDLE;
        this->removeInputListener();
    }

    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
                                                    "[ProceduralWallComponent] Confirmed wall segment, total segments: " + Ogre::StringConverter::toString(this->wallSegments.size()) + ", chaining: " + Ogre::StringConverter::toString(shouldChain));
}

void ProceduralWallComponent::cancelWall(void)
{
    this->destroyPreviewMesh();
    this->buildState = BuildState::IDLE;

    // Reset shift/ctrl flags when canceling
    this->isShiftPressed = false;
    this->isCtrlPressed = false;

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

    // ---- UNDO: Capture state BEFORE ----
    std::vector<unsigned char> oldData = this->getWallData();

    this->wallSegments.pop_back();

    if (true == this->wallSegments.empty())
    {
        this->destroyWallMesh();
        this->hasWallOrigin = false;
    }
    else
    {
        this->rebuildMesh();
    }

    // ---- UNDO: Capture state AFTER, fire event ----
    std::vector<unsigned char> newData = this->getWallData();

    boost::shared_ptr<EventDataWallModifyEnd> eventDataWallModifyEnd(new EventDataWallModifyEnd(oldData, newData, this->gameObjectPtr->getId()));
    NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataWallModifyEnd);

    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] Removed last segment, remaining: " + Ogre::StringConverter::toString(this->wallSegments.size()));
}

void ProceduralWallComponent::clearAllSegments(void)
{
    if (true == this->wallSegments.empty())
    {
        return;
    }

    // ---- UNDO: Capture state BEFORE ----
    std::vector<unsigned char> oldData = this->getWallData();

    this->wallSegments.clear();
    this->destroyWallMesh();
    this->hasWallOrigin = false;
    this->wallOrigin = Ogre::Vector3::ZERO;

    // Clear cache too
    this->cachedWallVertices.clear();
    this->cachedWallIndices.clear();
    this->cachedNumWallVertices = 0;
    this->cachedPillarVertices.clear();
    this->cachedPillarIndices.clear();
    this->cachedNumPillarVertices = 0;
    this->cachedWallOrigin = Ogre::Vector3::ZERO;
    this->hasLoadedWallEndpoint = false;

    // Deletes the file since cache is empty
    this->saveWallDataToFile();

    // ---- UNDO: Capture state AFTER (empty), fire event ----
    std::vector<unsigned char> newData; // Empty = cleared wall

    boost::shared_ptr<EventDataWallModifyEnd> eventDataWallModifyEnd(new EventDataWallModifyEnd(oldData, newData, this->gameObjectPtr->getId()));
    NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataWallModifyEnd);

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
    if (false == this->adaptToGround->getBool())
    {
        return position.y;
    }

    // Create a ray from high above pointing down
    Ogre::Vector3 rayOrigin = Ogre::Vector3(position.x, position.y + 1000.0f, position.z);
    Ogre::Ray downRay(rayOrigin, Ogre::Vector3::NEGATIVE_UNIT_Y);

    // Set the ray on the query BEFORE calling MathHelper
    this->groundQuery->setRay(downRay);
    this->groundQuery->setSortByDistance(true);

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
    bool hitFound =
        MathHelper::getInstance()->getRaycastFromPoint(this->groundQuery, AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera(), internalHitPoint, (size_t&)hitMovableObject, closestDistance, normal, &excludeMovableObjects, false);

    // If we hit terrain, use that height
    if (hitFound && hitMovableObject != nullptr)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] Ground height at (" + Ogre::StringConverter::toString(position.x) + ", " + Ogre::StringConverter::toString(position.z) +
                                                                               ") = " + Ogre::StringConverter::toString(internalHitPoint.y));

        return internalHitPoint.y;
    }

    // Fallback: try plane intersection
    std::pair<bool, Ogre::Real> planeResult = downRay.intersects(this->groundPlane);
    if (planeResult.first && planeResult.second > 0.0f)
    {
        Ogre::Real groundHeight = rayOrigin.y - planeResult.second;

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] Ground height (plane fallback) = " + Ogre::StringConverter::toString(groundHeight));

        return groundHeight;
    }

    // Last resort: return 0
    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] No ground found at position, using Y=0");

    return 0.0f;
}

bool ProceduralWallComponent::raycastGround(Ogre::Real screenX, Ogre::Real screenY, Ogre::Vector3& hitPosition)
{
    Ogre::Camera* camera = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
    if (nullptr == camera)
    {
        return false;
    }

    // Note: screenX and screenY are already normalized viewport coordinates (0-1)
    // We need to convert them back to pixel coordinates for MathHelper
    const OIS::MouseState& ms = NOWA::InputDeviceCore::getSingletonPtr()->getMouse()->getMouseState();

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

    // Use the MathHelper version that takes mouse coordinates
    bool hitFound = MathHelper::getInstance()->getRaycastFromPoint(ms.X.abs, ms.Y.abs, camera, Core::getSingletonPtr()->getOgreRenderWindow(), this->groundQuery, internalHitPoint, (size_t&)hitMovableObject, closestDistance, normal,
                                                                   &excludeMovableObjects, false);

    // If we hit something, use that position
    if (hitFound && hitMovableObject != nullptr)
    {
        hitPosition = internalHitPoint;
        return true;
    }

    // Fallback: intersect with ground plane at Y=0
    Ogre::Ray ray = camera->getCameraToViewportRay(screenX, screenY);
    std::pair<bool, Ogre::Real> result = ray.intersects(this->groundPlane);
    if (result.first && result.second > 0.0f)
    {
        hitPosition = ray.getPoint(result.second);
        return true;
    }

    return false;
}

// ==================== GEOMETRY GENERATION ====================

void ProceduralWallComponent::createWallMesh(void)
{
    if (true == this->wallSegments.empty())
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[ProceduralWallComponent] createWallMesh: No segments to create");
        return;
    }

    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[ProceduralWallComponent] createWallMesh: Creating mesh for " + Ogre::StringConverter::toString(this->wallSegments.size()) + " segments");

    // Clear mesh data for BOTH walls and pillars
    this->vertices.clear();
    this->indices.clear();
    this->currentVertexIndex = 0;

    this->pillarVertices.clear();
    this->pillarIndices.clear();
    this->currentPillarVertexIndex = 0;

    // Use stored wall origin (set when first segment was created)
    Ogre::Vector3 wallOriginToUse = this->wallOrigin;

    // Generate geometry for each segment (in LOCAL space relative to wallOrigin)
    WallStyle style = this->getWallStyleEnum();

    for (size_t segIdx = 0; segIdx < this->wallSegments.size(); ++segIdx)
    {
        const WallSegment& segment = this->wallSegments[segIdx];

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[ProceduralWallComponent] Generating segment " + Ogre::StringConverter::toString(segIdx) + ": start(" + Ogre::StringConverter::toString(segment.startPoint) + ") -> end(" +
                                                                              Ogre::StringConverter::toString(segment.endPoint) + ")");

        // Create a LOCAL segment relative to wallOrigin
        WallSegment localSegment;
        localSegment.startPoint = segment.startPoint - wallOriginToUse;
        localSegment.startPoint.y = 0.0f; // CRITICAL: Reset Y to 0 in local space

        localSegment.endPoint = segment.endPoint - wallOriginToUse;
        localSegment.endPoint.y = 0.0f; // CRITICAL: Reset Y to 0 in local space

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

        // Generate pillars into SEPARATE buffers
        if (segment.hasStartPillar)
        {
            Ogre::Vector3 localPos = segment.startPoint - wallOriginToUse;
            localPos.y = 0.0f; // Reset Y
            this->generatePillar(localPos, segment.groundHeightStart - wallOriginToUse.y);
        }
        if (segment.hasEndPillar)
        {
            Ogre::Vector3 localPos = segment.endPoint - wallOriginToUse;
            localPos.y = 0.0f; // Reset Y
            this->generatePillar(localPos, segment.groundHeightEnd - wallOriginToUse.y);
        }
    }

    if (this->vertices.empty() && this->pillarVertices.empty())
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralWallComponent] No vertices generated!");
        return;
    }

    // ---- CACHE CPU DATA HERE (before anything goes to GPU) ----
    this->cachedWallVertices = this->vertices;
    this->cachedWallIndices = this->indices;
    this->cachedNumWallVertices = this->currentVertexIndex;

    this->cachedPillarVertices = this->pillarVertices;
    this->cachedPillarIndices = this->pillarIndices;
    this->cachedNumPillarVertices = this->currentPillarVertexIndex;

    this->cachedWallOrigin = wallOriginToUse;

    // Create Ogre mesh - copy both wall and pillar data
    std::vector<float> wallVerticesCopy = this->vertices;
    std::vector<Ogre::uint32> wallIndicesCopy = this->indices;
    size_t numWallVertices = this->currentVertexIndex;

    std::vector<float> pillarVerticesCopy = this->pillarVertices;
    std::vector<Ogre::uint32> pillarIndicesCopy = this->pillarIndices;
    size_t numPillarVertices = this->currentPillarVertexIndex;

    // Execute mesh creation on render thread
    GraphicsModule::RenderCommand renderCommand = [this, wallVerticesCopy, wallIndicesCopy, numWallVertices, pillarVerticesCopy, pillarIndicesCopy, numPillarVertices, wallOriginToUse]()
    {
        this->createWallMeshInternal(wallVerticesCopy, wallIndicesCopy, numWallVertices, pillarVerticesCopy, pillarIndicesCopy, numPillarVertices, wallOriginToUse);
    };
    NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralWallComponent::createWallMesh");

    if (nullptr == this->wallMesh || nullptr == this->wallItem)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralWallComponent] No mesh/item to debug");
        return;
    }

    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] === MESH DEBUG INFO ===");
    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "Mesh name: " + this->wallMesh->getName());
    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "Num submeshes: " + Ogre::StringConverter::toString(this->wallMesh->getNumSubMeshes()));
    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "Num subitems: " + Ogre::StringConverter::toString(this->wallItem->getNumSubItems()));

    for (size_t i = 0; i < this->wallItem->getNumSubItems(); ++i)
    {
        Ogre::SubItem* subItem = this->wallItem->getSubItem(i);
        Ogre::HlmsDatablock* db = subItem->getDatablock();
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "SubItem " + Ogre::StringConverter::toString(i) + " datablock: " + (db ? db->getName().getFriendlyText() : "NULL"));
    }

    // Clear temporary data
    this->vertices.clear();
    this->indices.clear();
    this->pillarVertices.clear();
    this->pillarIndices.clear();
}

void ProceduralWallComponent::createWallMeshInternal(const std::vector<float>& wallVertices, const std::vector<Ogre::uint32>& wallIndices, size_t numWallVertices, const std::vector<float>& pillarVertices,
                                                     const std::vector<Ogre::uint32>& pillarIndices, size_t numPillarVertices, const Ogre::Vector3& wallOrigin)
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

    // ==================== SUBMESH 0: WALLS ====================
    // ALWAYS create wall submesh, even if empty
    Ogre::SubMesh* wallSubMesh = this->wallMesh->createSubMesh();

    if (numWallVertices > 0)
    {
        // Allocate and convert vertex data
        const size_t wallVertexDataSize = numWallVertices * dstFloatsPerVertex * sizeof(float);
        float* wallVertexData = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(wallVertexDataSize, Ogre::MEMCATEGORY_GEOMETRY));

        for (size_t i = 0; i < numWallVertices; ++i)
        {
            size_t srcOffset = i * srcFloatsPerVertex;
            size_t dstOffset = i * dstFloatsPerVertex;

            // Position
            wallVertexData[dstOffset + 0] = wallVertices[srcOffset + 0];
            wallVertexData[dstOffset + 1] = wallVertices[srcOffset + 1];
            wallVertexData[dstOffset + 2] = wallVertices[srcOffset + 2];

            // Update bounds
            Ogre::Vector3 pos(wallVertices[srcOffset + 0], wallVertices[srcOffset + 1], wallVertices[srcOffset + 2]);
            minBounds.makeFloor(pos);
            maxBounds.makeCeil(pos);

            // Normal
            Ogre::Vector3 normal(wallVertices[srcOffset + 3], wallVertices[srcOffset + 4], wallVertices[srcOffset + 5]);
            wallVertexData[dstOffset + 3] = normal.x;
            wallVertexData[dstOffset + 4] = normal.y;
            wallVertexData[dstOffset + 5] = normal.z;

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

            wallVertexData[dstOffset + 6] = tangent.x;
            wallVertexData[dstOffset + 7] = tangent.y;
            wallVertexData[dstOffset + 8] = tangent.z;
            wallVertexData[dstOffset + 9] = 1.0f;

            // UV
            wallVertexData[dstOffset + 10] = wallVertices[srcOffset + 6];
            wallVertexData[dstOffset + 11] = wallVertices[srcOffset + 7];
        }

        // Create vertex buffer
        Ogre::VertexBufferPacked* wallVertexBuffer = nullptr;
        try
        {
            wallVertexBuffer = vaoManager->createVertexBuffer(vertexElements, numWallVertices, Ogre::BT_IMMUTABLE, wallVertexData, true);
        }
        catch (Ogre::Exception& e)
        {
            OGRE_FREE_SIMD(wallVertexData, Ogre::MEMCATEGORY_GEOMETRY);
            throw e;
        }

        // Allocate index data
        const size_t wallIndexDataSize = wallIndices.size() * sizeof(Ogre::uint32);
        Ogre::uint32* wallIndexData = reinterpret_cast<Ogre::uint32*>(OGRE_MALLOC_SIMD(wallIndexDataSize, Ogre::MEMCATEGORY_GEOMETRY));
        memcpy(wallIndexData, wallIndices.data(), wallIndexDataSize);

        // Create index buffer
        Ogre::IndexBufferPacked* wallIndexBuffer = nullptr;
        try
        {
            wallIndexBuffer = vaoManager->createIndexBuffer(Ogre::IndexBufferPacked::IT_32BIT, wallIndices.size(), Ogre::BT_IMMUTABLE, wallIndexData, true);
        }
        catch (Ogre::Exception& e)
        {
            OGRE_FREE_SIMD(wallIndexData, Ogre::MEMCATEGORY_GEOMETRY);
            throw e;
        }

        // Create VAO
        Ogre::VertexBufferPackedVec wallVertexBuffers;
        wallVertexBuffers.push_back(wallVertexBuffer);

        Ogre::VertexArrayObject* wallVao = vaoManager->createVertexArrayObject(wallVertexBuffers, wallIndexBuffer, Ogre::OT_TRIANGLE_LIST);

        wallSubMesh->mVao[Ogre::VpNormal].push_back(wallVao);
        wallSubMesh->mVao[Ogre::VpShadow].push_back(wallVao);
    }
    else
    {
        // Create empty dummy VAO for wall submesh
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] Creating empty wall submesh");

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

        wallSubMesh->mVao[Ogre::VpNormal].push_back(dummyVao);
        wallSubMesh->mVao[Ogre::VpShadow].push_back(dummyVao);
    }

    // ==================== SUBMESH 1: PILLARS ====================
    // ALWAYS create pillar submesh, even if empty
    Ogre::SubMesh* pillarSubMesh = this->wallMesh->createSubMesh();

    if (numPillarVertices > 0)
    {
        // Allocate and convert vertex data
        const size_t pillarVertexDataSize = numPillarVertices * dstFloatsPerVertex * sizeof(float);
        float* pillarVertexData = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(pillarVertexDataSize, Ogre::MEMCATEGORY_GEOMETRY));

        for (size_t i = 0; i < numPillarVertices; ++i)
        {
            size_t srcOffset = i * srcFloatsPerVertex;
            size_t dstOffset = i * dstFloatsPerVertex;

            // Position
            pillarVertexData[dstOffset + 0] = pillarVertices[srcOffset + 0];
            pillarVertexData[dstOffset + 1] = pillarVertices[srcOffset + 1];
            pillarVertexData[dstOffset + 2] = pillarVertices[srcOffset + 2];

            // Update bounds
            Ogre::Vector3 pos(pillarVertices[srcOffset + 0], pillarVertices[srcOffset + 1], pillarVertices[srcOffset + 2]);
            minBounds.makeFloor(pos);
            maxBounds.makeCeil(pos);

            // Normal
            Ogre::Vector3 normal(pillarVertices[srcOffset + 3], pillarVertices[srcOffset + 4], pillarVertices[srcOffset + 5]);
            pillarVertexData[dstOffset + 3] = normal.x;
            pillarVertexData[dstOffset + 4] = normal.y;
            pillarVertexData[dstOffset + 5] = normal.z;

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

            pillarVertexData[dstOffset + 6] = tangent.x;
            pillarVertexData[dstOffset + 7] = tangent.y;
            pillarVertexData[dstOffset + 8] = tangent.z;
            pillarVertexData[dstOffset + 9] = 1.0f;

            // UV
            pillarVertexData[dstOffset + 10] = pillarVertices[srcOffset + 6];
            pillarVertexData[dstOffset + 11] = pillarVertices[srcOffset + 7];
        }

        // Create vertex buffer
        Ogre::VertexBufferPacked* pillarVertexBuffer = nullptr;
        try
        {
            pillarVertexBuffer = vaoManager->createVertexBuffer(vertexElements, numPillarVertices, Ogre::BT_IMMUTABLE, pillarVertexData, true);
        }
        catch (Ogre::Exception& e)
        {
            OGRE_FREE_SIMD(pillarVertexData, Ogre::MEMCATEGORY_GEOMETRY);
            throw e;
        }

        // Allocate index data
        const size_t pillarIndexDataSize = pillarIndices.size() * sizeof(Ogre::uint32);
        Ogre::uint32* pillarIndexData = reinterpret_cast<Ogre::uint32*>(OGRE_MALLOC_SIMD(pillarIndexDataSize, Ogre::MEMCATEGORY_GEOMETRY));
        memcpy(pillarIndexData, pillarIndices.data(), pillarIndexDataSize);

        // Create index buffer
        Ogre::IndexBufferPacked* pillarIndexBuffer = nullptr;
        try
        {
            pillarIndexBuffer = vaoManager->createIndexBuffer(Ogre::IndexBufferPacked::IT_32BIT, pillarIndices.size(), Ogre::BT_IMMUTABLE, pillarIndexData, true);
        }
        catch (Ogre::Exception& e)
        {
            OGRE_FREE_SIMD(pillarIndexData, Ogre::MEMCATEGORY_GEOMETRY);
            throw e;
        }

        // Create VAO
        Ogre::VertexBufferPackedVec pillarVertexBuffers;
        pillarVertexBuffers.push_back(pillarVertexBuffer);

        Ogre::VertexArrayObject* pillarVao = vaoManager->createVertexArrayObject(pillarVertexBuffers, pillarIndexBuffer, Ogre::OT_TRIANGLE_LIST);

        pillarSubMesh->mVao[Ogre::VpNormal].push_back(pillarVao);
        pillarSubMesh->mVao[Ogre::VpShadow].push_back(pillarVao);
    }
    else
    {
        // Create empty dummy VAO for pillar submesh
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] Creating empty pillar submesh");

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

        pillarSubMesh->mVao[Ogre::VpNormal].push_back(dummyVao);
        pillarSubMesh->mVao[Ogre::VpShadow].push_back(dummyVao);
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
    this->wallMesh->_setBounds(bounds, false);
    this->wallMesh->_setBoundingSphereRadius(bounds.getRadius());

    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] Mesh bounds: min(" + Ogre::StringConverter::toString(minBounds) + "), max(" + Ogre::StringConverter::toString(maxBounds) + ")");

    // Create item
    this->wallItem = this->gameObjectPtr->getSceneManager()->createItem(this->wallMesh, this->gameObjectPtr->isDynamic() ? Ogre::SCENE_DYNAMIC : Ogre::SCENE_STATIC);

    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] Created wall item with " + Ogre::StringConverter::toString(this->wallMesh->getNumSubMeshes()) + " submeshes");

    // Apply datablocks - submesh 0 is ALWAYS walls, submesh 1 is ALWAYS pillars

    // Submesh 0: Wall datablock
    Ogre::String wallDbName = this->wallDatablock->getString();
    if (false == wallDbName.empty())
    {
        Ogre::HlmsDatablock* wallDb = Ogre::Root::getSingleton().getHlmsManager()->getDatablockNoDefault(wallDbName);
        if (nullptr != wallDb)
        {
            this->wallItem->getSubItem(0)->setDatablock(wallDb);
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] Applied wall datablock: " + wallDbName);
        }
    }

    // Submesh 1: Pillar datablock
    Ogre::String pillarDbName = this->pillarDatablock->getString();
    if (false == pillarDbName.empty())
    {
        Ogre::HlmsDatablock* pillarDb = Ogre::Root::getSingleton().getHlmsManager()->getDatablockNoDefault(pillarDbName);
        if (nullptr != pillarDb)
        {
            this->wallItem->getSubItem(1)->setDatablock(pillarDb);
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] Applied pillar datablock: " + pillarDbName);
        }
    }
    else
    {
        // Fallback: use wall datablock if no pillar datablock specified
        if (false == wallDbName.empty())
        {
            Ogre::HlmsDatablock* wallDb = Ogre::Root::getSingleton().getHlmsManager()->getDatablockNoDefault(wallDbName);
            if (nullptr != wallDb)
            {
                this->wallItem->getSubItem(1)->setDatablock(wallDb);
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] Applied wall datablock to pillars (fallback)");
            }
        }
    }

    // At the end, ONLY set position if this is the first time creating the wall
    // After that, let the scene file manage the GameObject position
    if (false == this->originPositionSet)
    {
        this->originPositionSet = true;
        this->gameObjectPtr->getSceneNode()->setPosition(wallOrigin);
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] Set initial wall position: " + Ogre::StringConverter::toString(wallOrigin));
    }
    else
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] Preserving existing GameObject position: " + Ogre::StringConverter::toString(this->gameObjectPtr->getSceneNode()->getPosition()));
    }

    this->gameObjectPtr->getSceneNode()->attachObject(this->wallItem);
    this->gameObjectPtr->setDoNotDestroyMovableObject(true);
    this->gameObjectPtr->init(this->wallItem);

    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] Wall mesh created with " + Ogre::StringConverter::toString(numWallVertices) + " wall vertices and " +
                                                                           Ogre::StringConverter::toString(numPillarVertices) + " pillar vertices, attached to scene");
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

    // Clear pillar buffers too for preview
    this->pillarVertices.clear();
    this->pillarIndices.clear();
    this->currentPillarVertexIndex = 0;

    // IMPORTANT: Generate preview in LOCAL space relative to startPoint
    WallSegment localSegment;
    localSegment.startPoint = Ogre::Vector3::ZERO;                                           // Start at origin
    localSegment.endPoint = this->currentSegment.endPoint - this->currentSegment.startPoint; // Relative to start
    localSegment.endPoint.y = 0.0f;
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

    if (this->vertices.empty() && this->pillarVertices.empty())
    {
        return;
    }

    // Combine wall and pillar vertices for preview (we'll use single mesh for preview)
    std::vector<float> combinedVertices = this->vertices;
    std::vector<Ogre::uint32> combinedIndices = this->indices;

    // Append pillar data with offset indices
    size_t vertexOffset = this->currentVertexIndex;
    combinedVertices.insert(combinedVertices.end(), this->pillarVertices.begin(), this->pillarVertices.end());
    for (auto idx : this->pillarIndices)
    {
        combinedIndices.push_back(idx + vertexOffset);
    }

    size_t totalVertices = this->currentVertexIndex + this->currentPillarVertexIndex;

    // Capture the world position for the preview node
    Ogre::Vector3 previewPosition = this->currentSegment.startPoint;
    previewPosition.y = this->currentSegment.groundHeightStart;

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

        Ogre::String meshName = "WallPreview_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
        const Ogre::String groupName = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;

        this->previewMesh = Ogre::MeshManager::getSingleton().createManual(meshName, groupName);
        Ogre::SubMesh* subMesh = this->previewMesh->createSubMesh();

        // ADD TANGENTS to vertex format (same as main mesh)
        Ogre::VertexElement2Vec vertexElements;
        vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_POSITION));
        vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_NORMAL));
        vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT4, Ogre::VES_TANGENT)); // ADDED
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
    this->pillarVertices.clear();
    this->pillarIndices.clear();
}

// ==================== GEOMETRY HELPERS ====================

void ProceduralWallComponent::generateSolidWall(const WallSegment& segment)
{
    Ogre::Vector3 dir = segment.endPoint - segment.startPoint;
    Ogre::Real length = dir.length();
    if (length < 0.001f)
    {
        return;
    }

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
    {
        return;
    }

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
    {
        return;
    }

    dir.normalise();
    Ogre::Real spacing = this->fencePostSpacing->getReal();
    int numPosts = static_cast<int>(length / spacing) + 1;
    if (numPosts < 2)
    {
        numPosts = 2;
    }

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
        pos.y = 0.0f; // Ensure Y is 0 before adding ground height

        this->addBox(pos.x - postSize, groundH, pos.z - postSize, pos.x + postSize, groundH + height, pos.z + postSize, 1.0f, height);
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
    {
        return;
    }

    dir.normalise();
    Ogre::Vector3 perp = dir.crossProduct(Ogre::Vector3::UNIT_Y);
    perp.normalise();

    Ogre::Real merlonWidth = this->battlementWidth->getReal();
    Ogre::Real gapWidth = merlonWidth; // Same size gaps
    Ogre::Real halfThick = this->wallThickness->getReal() * 0.5f;

    // Calculate how many merlons fit along the wall
    int numMerlons = static_cast<int>(length / (merlonWidth + gapWidth));
    if (numMerlons < 1)
    {
        numMerlons = 1;
    }

    Ogre::Real actualSpacing = length / numMerlons;

    for (int i = 0; i < numMerlons; ++i)
    {
        // Calculate interpolation factor for ground height
        Ogre::Real t = (i + 0.5f) / numMerlons;

        // Position merlon center along the wall
        Ogre::Vector3 center = segment.startPoint + dir * ((i + 0.5f) * actualSpacing);
        center.y = 0.0f; // Ensure Y is 0

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
        Ogre::Real minX = std::min({corner1.x, corner2.x, corner3.x, corner4.x});
        Ogre::Real maxX = std::max({corner1.x, corner2.x, corner3.x, corner4.x});
        Ogre::Real minZ = std::min({corner1.z, corner2.z, corner3.z, corner4.z});
        Ogre::Real maxZ = std::max({corner1.z, corner2.z, corner3.z, corner4.z});

        this->addBox(minX, groundH + origHeight - battleHeight, minZ, maxX, groundH + origHeight, maxZ, merlonWidth, battleHeight);
    }
}

void ProceduralWallComponent::generateArchWall(const WallSegment& segment)
{
    Ogre::Vector3 dir = segment.endPoint - segment.startPoint;
    Ogre::Real length = dir.length();
    if (length < 0.001f)
    {
        return;
    }

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
    this->addBox(archStart.x - right.x * halfThick, groundHeightArchStart, archStart.z - right.z * halfThick, archStart.x + right.x * halfThick, groundHeightArchStart + archHeight, archStart.z + right.z * halfThick, columnWidth, archHeight);

    // Right column
    this->addBox(archEnd.x - right.x * halfThick, groundHeightArchEnd, archEnd.z - right.z * halfThick, archEnd.x + right.x * halfThick, groundHeightArchEnd + archHeight, archEnd.z + right.z * halfThick, columnWidth, archHeight);
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
    this->addPillarQuad(                 // CHANGED: was addQuad
        Ogre::Vector3(minX, minY, minZ), // BL
        Ogre::Vector3(minX, maxY, minZ), // TL
        Ogre::Vector3(maxX, maxY, minZ), // TR
        Ogre::Vector3(maxX, minY, minZ), // BR
        Ogre::Vector3::NEGATIVE_UNIT_Z, uTile, vTile);

    // Back face (+Z): BL, TL, TR, BR (from back view)
    this->addPillarQuad(                 // CHANGED
        Ogre::Vector3(maxX, minY, maxZ), // BL (from back)
        Ogre::Vector3(maxX, maxY, maxZ), // TL
        Ogre::Vector3(minX, maxY, maxZ), // TR
        Ogre::Vector3(minX, minY, maxZ), // BR
        Ogre::Vector3::UNIT_Z, uTile, vTile);

    // Left face (-X): BL, TL, TR, BR
    this->addPillarQuad(                 // CHANGED
        Ogre::Vector3(minX, minY, maxZ), // BL (back)
        Ogre::Vector3(minX, maxY, maxZ), // TL
        Ogre::Vector3(minX, maxY, minZ), // TR (front)
        Ogre::Vector3(minX, minY, minZ), // BR
        Ogre::Vector3::NEGATIVE_UNIT_X, uTile, vTile);

    // Right face (+X): BL, TL, TR, BR
    this->addPillarQuad(                 // CHANGED
        Ogre::Vector3(maxX, minY, minZ), // BL (front)
        Ogre::Vector3(maxX, maxY, minZ), // TL
        Ogre::Vector3(maxX, maxY, maxZ), // TR (back)
        Ogre::Vector3(maxX, minY, maxZ), // BR
        Ogre::Vector3::UNIT_X, uTile, vTile);

    // Top face (+Y): BL, TL, TR, BR
    this->addPillarQuad(                 // CHANGED
        Ogre::Vector3(minX, maxY, minZ), // BL (front-left)
        Ogre::Vector3(minX, maxY, maxZ), // TL (back-left)
        Ogre::Vector3(maxX, maxY, maxZ), // TR (back-right)
        Ogre::Vector3(maxX, maxY, minZ), // BR (front-right)
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
    this->addQuad(Ogre::Vector3(minX, minY, minZ), Ogre::Vector3(minX, maxY, minZ), Ogre::Vector3(maxX, maxY, minZ), Ogre::Vector3(maxX, minY, minZ), Ogre::Vector3::NEGATIVE_UNIT_Z, uTile, vTile);

    // Back face (+Z): BL, TL, TR, BR
    this->addQuad(Ogre::Vector3(maxX, minY, maxZ), Ogre::Vector3(maxX, maxY, maxZ), Ogre::Vector3(minX, maxY, maxZ), Ogre::Vector3(minX, minY, maxZ), Ogre::Vector3::UNIT_Z, uTile, vTile);

    // Left face (-X): BL, TL, TR, BR
    this->addQuad(Ogre::Vector3(minX, minY, maxZ), Ogre::Vector3(minX, maxY, maxZ), Ogre::Vector3(minX, maxY, minZ), Ogre::Vector3(minX, minY, minZ), Ogre::Vector3::NEGATIVE_UNIT_X, uTile, vTile);

    // Right face (+X): BL, TL, TR, BR
    this->addQuad(Ogre::Vector3(maxX, minY, minZ), Ogre::Vector3(maxX, maxY, minZ), Ogre::Vector3(maxX, maxY, maxZ), Ogre::Vector3(maxX, minY, maxZ), Ogre::Vector3::UNIT_X, uTile, vTile);

    // Top face (+Y): BL, TL, TR, BR
    this->addQuad(Ogre::Vector3(minX, maxY, minZ), Ogre::Vector3(minX, maxY, maxZ), Ogre::Vector3(maxX, maxY, maxZ), Ogre::Vector3(maxX, maxY, minZ), Ogre::Vector3::UNIT_Y, uTile, uTile);
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

void ProceduralWallComponent::addPillarQuad(const Ogre::Vector3& v0, const Ogre::Vector3& v1, const Ogre::Vector3& v2, const Ogre::Vector3& v3, const Ogre::Vector3& normal, Ogre::Real uTile, Ogre::Real vTile)
{
    Ogre::uint32 baseIndex = this->currentPillarVertexIndex;

    // Vertex format: pos(3) + normal(3) + uv(2) = 8 floats
    // Expected vertex order: v0=BL, v1=TL, v2=TR, v3=BR

    // Vertex 0 - bottom-left (UV 0,0)
    this->pillarVertices.push_back(v0.x);
    this->pillarVertices.push_back(v0.y);
    this->pillarVertices.push_back(v0.z);
    this->pillarVertices.push_back(normal.x);
    this->pillarVertices.push_back(normal.y);
    this->pillarVertices.push_back(normal.z);
    this->pillarVertices.push_back(0.0f);
    this->pillarVertices.push_back(0.0f);

    // Vertex 1 - top-left (UV 0,vTile)
    this->pillarVertices.push_back(v1.x);
    this->pillarVertices.push_back(v1.y);
    this->pillarVertices.push_back(v1.z);
    this->pillarVertices.push_back(normal.x);
    this->pillarVertices.push_back(normal.y);
    this->pillarVertices.push_back(normal.z);
    this->pillarVertices.push_back(0.0f);
    this->pillarVertices.push_back(vTile);

    // Vertex 2 - top-right (UV uTile,vTile)
    this->pillarVertices.push_back(v2.x);
    this->pillarVertices.push_back(v2.y);
    this->pillarVertices.push_back(v2.z);
    this->pillarVertices.push_back(normal.x);
    this->pillarVertices.push_back(normal.y);
    this->pillarVertices.push_back(normal.z);
    this->pillarVertices.push_back(uTile);
    this->pillarVertices.push_back(vTile);

    // Vertex 3 - bottom-right (UV uTile,0)
    this->pillarVertices.push_back(v3.x);
    this->pillarVertices.push_back(v3.y);
    this->pillarVertices.push_back(v3.z);
    this->pillarVertices.push_back(normal.x);
    this->pillarVertices.push_back(normal.y);
    this->pillarVertices.push_back(normal.z);
    this->pillarVertices.push_back(uTile);
    this->pillarVertices.push_back(0.0f);

    this->currentPillarVertexIndex += 4;

    // Two triangles with counter-clockwise winding: 0-1-2 and 0-2-3
    // (BL,TL,TR) and (BL,TR,BR)
    this->pillarIndices.push_back(baseIndex + 0);
    this->pillarIndices.push_back(baseIndex + 1);
    this->pillarIndices.push_back(baseIndex + 2);

    this->pillarIndices.push_back(baseIndex + 0);
    this->pillarIndices.push_back(baseIndex + 2);
    this->pillarIndices.push_back(baseIndex + 3);
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

// ==================== FILE PATH HELPERS ====================

Ogre::String ProceduralWallComponent::getWallDataFilePath(void) const
{
    Ogre::String projectFilePath;

    if (false == this->gameObjectPtr->getGlobal())
    {
        projectFilePath = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getSceneName();
    }
    else
    {
        projectFilePath = Core::getSingletonPtr()->getCurrentProjectPath();
    }

    // Create filename based on GameObject ID for uniqueness
    Ogre::String filename = "Wall_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()) + ".walldata";

    return projectFilePath + "/" + filename;
}

// ==================== SAVE WALL DATA ====================

bool ProceduralWallComponent::saveWallDataToFile(void)
{
    /*
    Offset  Size  Description
    ------  ----  -----------
    0       4     Magic: 0x57414C4C ("WALL")
    4       4     Version: 2
    8       4     Origin X (float)
    12      4     Origin Y (float)
    16      4     Origin Z (float)
    20      4     numSegments (uint32)
    24      4     numWallVertices (uint32)
    28      4     numWallIndices (uint32)
    32      4     numPillarVertices (uint32)
    36      4     numPillarIndices (uint32)
    -- header: 40 bytes --

    For each segment (34 bytes each):
      startPoint.x/y/z     12 bytes (3 floats)
      endPoint.x/y/z       12 bytes (3 floats)
      groundHeightStart     4 bytes (float)
      groundHeightEnd       4 bytes (float)
      hasStartPillar        1 byte  (uint8)
      hasEndPillar          1 byte  (uint8)

    Wall vertex data:   numWallVertices   * 8 * sizeof(float)
    Wall index data:    numWallIndices    * sizeof(uint32)
    Pillar vertex data: numPillarVertices * 8 * sizeof(float)
    Pillar index data:  numPillarIndices  * sizeof(uint32)
    */

    // Need either segments or cached vertex data
    if (this->wallSegments.empty() && this->cachedWallVertices.empty())
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] saveWallDataToFile: nothing to save, deleting file");
        this->deleteWallDataFile();
        return true;
    }

    Ogre::String filePath = this->getWallDataFilePath();

    try
    {
        uint32_t numSegments = static_cast<uint32_t>(this->wallSegments.size());
        uint32_t numWallVerts = static_cast<uint32_t>(this->cachedNumWallVertices);
        uint32_t numWallIdx = static_cast<uint32_t>(this->cachedWallIndices.size());
        uint32_t numPillarVerts = static_cast<uint32_t>(this->cachedNumPillarVertices);
        uint32_t numPillarIdx = static_cast<uint32_t>(this->cachedPillarIndices.size());

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] saveWallDataToFile: " + Ogre::StringConverter::toString(numSegments) + " segments, " + Ogre::StringConverter::toString(numWallVerts) +
                                                                               " wall verts, " + Ogre::StringConverter::toString(numWallIdx) + " wall indices, " + Ogre::StringConverter::toString(numPillarVerts) + " pillar verts, " +
                                                                               Ogre::StringConverter::toString(numPillarIdx) + " pillar indices");

        const size_t floatsPerVertex = 8;
        const size_t bytesPerSegment = 34;

        size_t headerSize = 41;
        size_t segmentDataSize = numSegments * bytesPerSegment;
        size_t wallVertBytes = numWallVerts * floatsPerVertex * sizeof(float);
        size_t wallIdxBytes = numWallIdx * sizeof(uint32_t);
        size_t pillarVertBytes = numPillarVerts * floatsPerVertex * sizeof(float);
        size_t pillarIdxBytes = numPillarIdx * sizeof(uint32_t);

        size_t totalSize = headerSize + segmentDataSize + wallVertBytes + wallIdxBytes + pillarVertBytes + pillarIdxBytes;

        std::vector<unsigned char> buffer(totalSize);
        size_t off = 0;

        // --- Header ---
        uint32_t magic = WALLDATA_MAGIC;
        uint32_t version = WALLDATA_VERSION;
        memcpy(&buffer[off], &magic, 4);
        off += 4;
        memcpy(&buffer[off], &version, 4);
        off += 4;
        memcpy(&buffer[off], &this->cachedWallOrigin.x, 4);
        off += 4;
        memcpy(&buffer[off], &this->cachedWallOrigin.y, 4);
        off += 4;
        memcpy(&buffer[off], &this->cachedWallOrigin.z, 4);
        off += 4;
        memcpy(&buffer[off], &numSegments, 4);
        off += 4;
        memcpy(&buffer[off], &numWallVerts, 4);
        off += 4;
        memcpy(&buffer[off], &numWallIdx, 4);
        off += 4;
        memcpy(&buffer[off], &numPillarVerts, 4);
        off += 4;
        memcpy(&buffer[off], &numPillarIdx, 4);
        off += 4;
        uint8_t posSet = this->originPositionSet ? 1 : 0;
        buffer[off++] = posSet;

        // --- Segment data ---
        for (const WallSegment& seg : this->wallSegments)
        {
            memcpy(&buffer[off], &seg.startPoint.x, 4);
            off += 4;
            memcpy(&buffer[off], &seg.startPoint.y, 4);
            off += 4;
            memcpy(&buffer[off], &seg.startPoint.z, 4);
            off += 4;
            memcpy(&buffer[off], &seg.endPoint.x, 4);
            off += 4;
            memcpy(&buffer[off], &seg.endPoint.y, 4);
            off += 4;
            memcpy(&buffer[off], &seg.endPoint.z, 4);
            off += 4;
            memcpy(&buffer[off], &seg.groundHeightStart, 4);
            off += 4;
            memcpy(&buffer[off], &seg.groundHeightEnd, 4);
            off += 4;
            uint8_t sp = seg.hasStartPillar ? 1 : 0;
            uint8_t ep = seg.hasEndPillar ? 1 : 0;
            buffer[off++] = sp;
            buffer[off++] = ep;
        }

        // --- Vertex / index data (from CPU cache) ---
        if (wallVertBytes > 0)
        {
            memcpy(&buffer[off], this->cachedWallVertices.data(), wallVertBytes);
        }
        off += wallVertBytes;

        if (wallIdxBytes > 0)
        {
            memcpy(&buffer[off], this->cachedWallIndices.data(), wallIdxBytes);
        }
        off += wallIdxBytes;

        if (pillarVertBytes > 0)
        {
            memcpy(&buffer[off], this->cachedPillarVertices.data(), pillarVertBytes);
        }
        off += pillarVertBytes;

        if (pillarIdxBytes > 0)
        {
            memcpy(&buffer[off], this->cachedPillarIndices.data(), pillarIdxBytes);
        }
        off += pillarIdxBytes;

        // --- Write file ---
        std::ofstream outFile(filePath.c_str(), std::ios::binary);
        if (false == outFile.is_open())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralWallComponent] Cannot open for writing: " + filePath);
            return false;
        }

        outFile.write(reinterpret_cast<const char*>(buffer.data()), totalSize);
        outFile.close();

        if (true == outFile.fail())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralWallComponent] Write failed: " + filePath);
            return false;
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] Saved to: " + filePath + " (" + Ogre::StringConverter::toString(totalSize) + " bytes)");
        return true;
    }
    catch (const std::exception& e)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralWallComponent] Exception saving: " + Ogre::String(e.what()));
        return false;
    }
}

// ==================== LOAD MESH DATA ====================

bool ProceduralWallComponent::loadWallDataFromFile(void)
{
    Ogre::String filePath = this->getWallDataFilePath();

    std::ifstream inFile(filePath.c_str(), std::ios::binary);
    if (false == inFile.is_open())
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] No wall data file (new wall): " + filePath);
        return true;
    }

    try
    {
        inFile.seekg(0, std::ios::end);
        size_t fileSize = static_cast<size_t>(inFile.tellg());
        inFile.seekg(0, std::ios::beg);

        if (fileSize < 41)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralWallComponent] File too small: " + filePath);
            inFile.close();
            return false;
        }

        std::vector<unsigned char> buffer(fileSize);
        inFile.read(reinterpret_cast<char*>(buffer.data()), fileSize);
        inFile.close();

        if (true == inFile.fail())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralWallComponent] Read failed: " + filePath);
            return false;
        }

        size_t off = 0;

        // --- Header ---
        uint32_t magic, version;
        memcpy(&magic, &buffer[off], 4);
        off += 4;
        memcpy(&version, &buffer[off], 4);
        off += 4;

        if (magic != WALLDATA_MAGIC)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralWallComponent] Bad magic in: " + filePath);
            return false;
        }
        if (version != WALLDATA_VERSION)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralWallComponent] Unsupported version " + Ogre::StringConverter::toString(version) + " in: " + filePath);
            return false;
        }

        Ogre::Vector3 origin;
        memcpy(&origin.x, &buffer[off], 4);
        off += 4;
        memcpy(&origin.y, &buffer[off], 4);
        off += 4;
        memcpy(&origin.z, &buffer[off], 4);
        off += 4;

        uint32_t numSegments, numWallVerts, numWallIdx, numPillarVerts, numPillarIdx;
        memcpy(&numSegments, &buffer[off], 4);
        off += 4;
        memcpy(&numWallVerts, &buffer[off], 4);
        off += 4;
        memcpy(&numWallIdx, &buffer[off], 4);
        off += 4;
        memcpy(&numPillarVerts, &buffer[off], 4);
        off += 4;
        memcpy(&numPillarIdx, &buffer[off], 4);
        off += 4;

        // Read originPositionSet flag (moved BEFORE validation)
        uint8_t posSet = buffer[off++];
        this->originPositionSet = (posSet != 0);

        // --- Validate file size ---
        const size_t floatsPerVertex = 8;
        const size_t bytesPerSegment = 34;
        size_t expectedSize = 41 + +(numSegments * bytesPerSegment) + (numWallVerts * floatsPerVertex * sizeof(float)) + (numWallIdx * sizeof(uint32_t)) + (numPillarVerts * floatsPerVertex * sizeof(float)) + (numPillarIdx * sizeof(uint32_t));

        if (fileSize != expectedSize)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralWallComponent] File size mismatch: expected " + Ogre::StringConverter::toString(expectedSize) + " got " + Ogre::StringConverter::toString(fileSize));
            return false;
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] Loading: " + Ogre::StringConverter::toString(numSegments) + " segments, " + Ogre::StringConverter::toString(numWallVerts) + " wall verts, " +
                                                                               Ogre::StringConverter::toString(numWallIdx) + " wall idx, " + Ogre::StringConverter::toString(numPillarVerts) + " pillar verts, " +
                                                                               Ogre::StringConverter::toString(numPillarIdx) + " pillar idx");

        // --- Restore segments ---
        this->wallSegments.clear();
        for (uint32_t i = 0; i < numSegments; ++i)
        {
            WallSegment seg;
            memcpy(&seg.startPoint.x, &buffer[off], 4);
            off += 4;
            memcpy(&seg.startPoint.y, &buffer[off], 4);
            off += 4;
            memcpy(&seg.startPoint.z, &buffer[off], 4);
            off += 4;
            memcpy(&seg.endPoint.x, &buffer[off], 4);
            off += 4;
            memcpy(&seg.endPoint.y, &buffer[off], 4);
            off += 4;
            memcpy(&seg.endPoint.z, &buffer[off], 4);
            off += 4;
            memcpy(&seg.groundHeightStart, &buffer[off], 4);
            off += 4;
            memcpy(&seg.groundHeightEnd, &buffer[off], 4);
            off += 4;

            seg.hasStartPillar = (buffer[off++] != 0);
            seg.hasEndPillar = (buffer[off++] != 0);

            this->wallSegments.push_back(seg);
        }

        // Restore wall origin
        this->cachedWallOrigin = origin;
        this->wallOrigin = origin;
        this->hasWallOrigin = true;

        // --- Restore vertex/index cache ---
        size_t wallVertBytes = numWallVerts * floatsPerVertex * sizeof(float);
        size_t wallIdxBytes = numWallIdx * sizeof(uint32_t);
        size_t pillarVertBytes = numPillarVerts * floatsPerVertex * sizeof(float);
        size_t pillarIdxBytes = numPillarIdx * sizeof(uint32_t);

        this->cachedWallVertices.resize(numWallVerts * floatsPerVertex);
        this->cachedWallIndices.resize(numWallIdx);
        this->cachedPillarVertices.resize(numPillarVerts * floatsPerVertex);
        this->cachedPillarIndices.resize(numPillarIdx);

        if (wallVertBytes > 0)
        {
            memcpy(this->cachedWallVertices.data(), &buffer[off], wallVertBytes);
        }
        off += wallVertBytes;

        if (wallIdxBytes > 0)
        {
            memcpy(this->cachedWallIndices.data(), &buffer[off], wallIdxBytes);
        }
        off += wallIdxBytes;

        if (pillarVertBytes > 0)
        {
            memcpy(this->cachedPillarVertices.data(), &buffer[off], pillarVertBytes);
        }
        off += pillarVertBytes;

        if (pillarIdxBytes > 0)
        {
            memcpy(this->cachedPillarIndices.data(), &buffer[off], pillarIdxBytes);
        }
        off += pillarIdxBytes;

        this->cachedNumWallVertices = numWallVerts;
        this->cachedNumPillarVertices = numPillarVerts;

        // --- Recreate mesh via existing pipeline (fast path: uses cache directly) ---
        std::vector<float> wv = this->cachedWallVertices;
        std::vector<Ogre::uint32> wi = this->cachedWallIndices;
        std::vector<float> pv = this->cachedPillarVertices;
        std::vector<Ogre::uint32> pi = this->cachedPillarIndices;
        size_t nwv = this->cachedNumWallVertices;
        size_t npv = this->cachedNumPillarVertices;

        GraphicsModule::RenderCommand renderCommand = [this, wv, wi, nwv, pv, pi, npv, origin]()
        {
            this->createWallMeshInternal(wv, wi, nwv, pv, pi, npv, origin);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralWallComponent::loadWallDataFromFile");

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] Load complete: " + filePath);

        if (!this->wallSegments.empty())
        {
            const WallSegment& lastSegment = this->wallSegments.back();

            // Segments are in WORLD space, use directly
            this->loadedWallEndpoint = lastSegment.endPoint;
            this->loadedWallEndpointHeight = lastSegment.groundHeightEnd;
            this->hasLoadedWallEndpoint = true;

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] Loaded wall endpoint for continuation: " + Ogre::StringConverter::toString(this->loadedWallEndpoint) +
                                                                                   ", height: " + Ogre::StringConverter::toString(this->loadedWallEndpointHeight));
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] Load complete: " + filePath);
        return true;
    }
    catch (const std::exception& e)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralWallComponent] Exception loading: " + Ogre::String(e.what()));
        return false;
    }
}

std::vector<unsigned char> ProceduralWallComponent::getWallData(void) const
{
    /*
    Same format as saveWallDataToFile():

    Offset  Size  Description
    ------  ----  -----------
    0       4     Magic: 0x57414C4C ("WALL")
    4       4     Version: 2
    8       4     Origin X (float)
    12      4     Origin Y (float)
    16      4     Origin Z (float)
    20      4     numSegments (uint32)
    24      4     numWallVertices (uint32)
    28      4     numWallIndices (uint32)
    32      4     numPillarVertices (uint32)
    36      4     numPillarIndices (uint32)
    -- header: 40 bytes --

    Then: segment data, vertex data, index data
    */

    // If empty, return empty vector
    if (this->wallSegments.empty() && this->cachedWallVertices.empty())
    {
        return std::vector<unsigned char>();
    }

    try
    {
        uint32_t numSegments = static_cast<uint32_t>(this->wallSegments.size());
        uint32_t numWallVerts = static_cast<uint32_t>(this->cachedNumWallVertices);
        uint32_t numWallIdx = static_cast<uint32_t>(this->cachedWallIndices.size());
        uint32_t numPillarVerts = static_cast<uint32_t>(this->cachedNumPillarVertices);
        uint32_t numPillarIdx = static_cast<uint32_t>(this->cachedPillarIndices.size());

        const size_t floatsPerVertex = 8;  // pos(3) + normal(3) + uv(2)
        const size_t bytesPerSegment = 34; // 3+3 floats + 2 floats + 2 bytes

        size_t headerSize = 41;
        size_t segmentDataSize = numSegments * bytesPerSegment;
        size_t wallVertBytes = numWallVerts * floatsPerVertex * sizeof(float);
        size_t wallIdxBytes = numWallIdx * sizeof(uint32_t);
        size_t pillarVertBytes = numPillarVerts * floatsPerVertex * sizeof(float);
        size_t pillarIdxBytes = numPillarIdx * sizeof(uint32_t);

        size_t totalSize = headerSize + segmentDataSize + wallVertBytes + wallIdxBytes + pillarVertBytes + pillarIdxBytes;

        std::vector<unsigned char> buffer(totalSize);
        size_t off = 0;

        // --- Header ---
        uint32_t magic = WALLDATA_MAGIC;
        uint32_t version = WALLDATA_VERSION;
        memcpy(&buffer[off], &magic, 4);
        off += 4;
        memcpy(&buffer[off], &version, 4);
        off += 4;
        memcpy(&buffer[off], &this->cachedWallOrigin.x, 4);
        off += 4;
        memcpy(&buffer[off], &this->cachedWallOrigin.y, 4);
        off += 4;
        memcpy(&buffer[off], &this->cachedWallOrigin.z, 4);
        off += 4;
        memcpy(&buffer[off], &numSegments, 4);
        off += 4;
        memcpy(&buffer[off], &numWallVerts, 4);
        off += 4;
        memcpy(&buffer[off], &numWallIdx, 4);
        off += 4;
        memcpy(&buffer[off], &numPillarVerts, 4);
        off += 4;
        memcpy(&buffer[off], &numPillarIdx, 4);
        off += 4;
        uint8_t posSet = this->originPositionSet ? 1 : 0;
        buffer[off++] = posSet;

        // --- Segment data ---
        for (const WallSegment& seg : this->wallSegments)
        {
            memcpy(&buffer[off], &seg.startPoint.x, 4);
            off += 4;
            memcpy(&buffer[off], &seg.startPoint.y, 4);
            off += 4;
            memcpy(&buffer[off], &seg.startPoint.z, 4);
            off += 4;
            memcpy(&buffer[off], &seg.endPoint.x, 4);
            off += 4;
            memcpy(&buffer[off], &seg.endPoint.y, 4);
            off += 4;
            memcpy(&buffer[off], &seg.endPoint.z, 4);
            off += 4;
            memcpy(&buffer[off], &seg.groundHeightStart, 4);
            off += 4;
            memcpy(&buffer[off], &seg.groundHeightEnd, 4);
            off += 4;
            uint8_t sp = seg.hasStartPillar ? 1 : 0;
            uint8_t ep = seg.hasEndPillar ? 1 : 0;
            buffer[off++] = sp;
            buffer[off++] = ep;
        }

        // --- Vertex / index data (from CPU cache) ---
        if (wallVertBytes > 0)
        {
            memcpy(&buffer[off], this->cachedWallVertices.data(), wallVertBytes);
        }
        off += wallVertBytes;

        if (wallIdxBytes > 0)
        {
            memcpy(&buffer[off], this->cachedWallIndices.data(), wallIdxBytes);
        }
        off += wallIdxBytes;

        if (pillarVertBytes > 0)
        {
            memcpy(&buffer[off], this->cachedPillarVertices.data(), pillarVertBytes);
        }
        off += pillarVertBytes;

        if (pillarIdxBytes > 0)
        {
            memcpy(&buffer[off], this->cachedPillarIndices.data(), pillarIdxBytes);
        }
        off += pillarIdxBytes;

        return buffer;
    }
    catch (const std::exception& e)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralWallComponent] Exception in getWallData: " + Ogre::String(e.what()));
        return std::vector<unsigned char>();
    }
}

void ProceduralWallComponent::setWallData(const std::vector<unsigned char>& data)
{
    this->destroyWallMesh();

    // Empty data = clear the wall
    if (data.empty())
    {
        this->wallSegments.clear();
        this->destroyWallMesh();
        this->hasWallOrigin = false;
        this->wallOrigin = Ogre::Vector3::ZERO;
        this->cachedWallVertices.clear();
        this->cachedWallIndices.clear();
        this->cachedNumWallVertices = 0;
        this->cachedPillarVertices.clear();
        this->cachedPillarIndices.clear();
        this->cachedNumPillarVertices = 0;
        this->cachedWallOrigin = Ogre::Vector3::ZERO;

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] setWallData: cleared (empty data)");
        return;
    }

    try
    {
        size_t fileSize = data.size();

        if (fileSize < 41)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralWallComponent] setWallData: data too small");
            return;
        }

        size_t off = 0;

        // --- Header ---
        uint32_t magic, version;
        memcpy(&magic, &data[off], 4);
        off += 4;
        memcpy(&version, &data[off], 4);
        off += 4;

        if (magic != WALLDATA_MAGIC)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralWallComponent] setWallData: bad magic");
            return;
        }
        if (version != WALLDATA_VERSION)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralWallComponent] setWallData: unsupported version " + Ogre::StringConverter::toString(version));
            return;
        }

        Ogre::Vector3 origin;
        memcpy(&origin.x, &data[off], 4);
        off += 4;
        memcpy(&origin.y, &data[off], 4);
        off += 4;
        memcpy(&origin.z, &data[off], 4);
        off += 4;

        uint32_t numSegments, numWallVerts, numWallIdx, numPillarVerts, numPillarIdx;
        memcpy(&numSegments, &data[off], 4);
        off += 4;
        memcpy(&numWallVerts, &data[off], 4);
        off += 4;
        memcpy(&numWallIdx, &data[off], 4);
        off += 4;
        memcpy(&numPillarVerts, &data[off], 4);
        off += 4;
        memcpy(&numPillarIdx, &data[off], 4);
        off += 4;
        uint8_t posSet = data[off++];
        this->originPositionSet = (posSet != 0);

        // --- Validate size ---
        const size_t floatsPerVertex = 8;
        const size_t bytesPerSegment = 34;
        size_t expectedSize = 41 + (numSegments * bytesPerSegment) + (numWallVerts * floatsPerVertex * sizeof(float)) + (numWallIdx * sizeof(uint32_t)) + (numPillarVerts * floatsPerVertex * sizeof(float)) + (numPillarIdx * sizeof(uint32_t));

        if (fileSize != expectedSize)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralWallComponent] setWallData: size mismatch: expected " + Ogre::StringConverter::toString(expectedSize) + " got " + Ogre::StringConverter::toString(fileSize));
            return;
        }

        // --- Restore segments ---
        this->wallSegments.clear();
        for (uint32_t i = 0; i < numSegments; ++i)
        {
            WallSegment seg;
            memcpy(&seg.startPoint.x, &data[off], 4);
            off += 4;
            memcpy(&seg.startPoint.y, &data[off], 4);
            off += 4;
            memcpy(&seg.startPoint.z, &data[off], 4);
            off += 4;
            memcpy(&seg.endPoint.x, &data[off], 4);
            off += 4;
            memcpy(&seg.endPoint.y, &data[off], 4);
            off += 4;
            memcpy(&seg.endPoint.z, &data[off], 4);
            off += 4;
            memcpy(&seg.groundHeightStart, &data[off], 4);
            off += 4;
            memcpy(&seg.groundHeightEnd, &data[off], 4);
            off += 4;

            seg.hasStartPillar = (data[off++] != 0);
            seg.hasEndPillar = (data[off++] != 0);

            this->wallSegments.push_back(seg);
        }

        // Restore wall origin
        this->cachedWallOrigin = origin;
        this->wallOrigin = origin;
        this->hasWallOrigin = true;

        // --- Restore vertex/index cache ---
        size_t wallVertBytes = numWallVerts * floatsPerVertex * sizeof(float);
        size_t wallIdxBytes = numWallIdx * sizeof(uint32_t);
        size_t pillarVertBytes = numPillarVerts * floatsPerVertex * sizeof(float);
        size_t pillarIdxBytes = numPillarIdx * sizeof(uint32_t);

        this->cachedWallVertices.resize(numWallVerts * floatsPerVertex);
        this->cachedWallIndices.resize(numWallIdx);
        this->cachedPillarVertices.resize(numPillarVerts * floatsPerVertex);
        this->cachedPillarIndices.resize(numPillarIdx);

        if (wallVertBytes > 0)
        {
            memcpy(this->cachedWallVertices.data(), &data[off], wallVertBytes);
        }
        off += wallVertBytes;

        if (wallIdxBytes > 0)
        {
            memcpy(this->cachedWallIndices.data(), &data[off], wallIdxBytes);
        }
        off += wallIdxBytes;

        if (pillarVertBytes > 0)
        {
            memcpy(this->cachedPillarVertices.data(), &data[off], pillarVertBytes);
        }
        off += pillarVertBytes;

        if (pillarIdxBytes > 0)
        {
            memcpy(this->cachedPillarIndices.data(), &data[off], pillarIdxBytes);
        }
        off += pillarIdxBytes;

        this->cachedNumWallVertices = numWallVerts;
        this->cachedNumPillarVertices = numPillarVerts;

        // --- Recreate mesh via existing pipeline ---
        std::vector<float> wv = this->cachedWallVertices;
        std::vector<Ogre::uint32> wi = this->cachedWallIndices;
        std::vector<float> pv = this->cachedPillarVertices;
        std::vector<Ogre::uint32> pi = this->cachedPillarIndices;
        size_t nwv = this->cachedNumWallVertices;
        size_t npv = this->cachedNumPillarVertices;

        GraphicsModule::RenderCommand renderCommand = [this, wv, wi, nwv, pv, pi, npv, origin]()
        {
            this->createWallMeshInternal(wv, wi, nwv, pv, pi, npv, origin);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralWallComponent::setWallData");

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] setWallData: restored " + Ogre::StringConverter::toString(numSegments) + " segments");
    }
    catch (const std::exception& e)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralWallComponent] Exception in setWallData: " + Ogre::String(e.what()));
    }
}

// ==================== DELETE WALL DATA ====================

void ProceduralWallComponent::deleteWallDataFile(void)
{
    std::filesystem::path relativePath(this->getWallDataFilePath());

    // Resolve against current working directory
    std::filesystem::path absolutePath = std::filesystem::absolute(relativePath);

    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralWallComponent] Working dir: " + std::filesystem::current_path().string());

    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralWallComponent] Absolute delete path: " + absolutePath.string());

    if (!std::filesystem::exists(absolutePath))
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralWallComponent] File does not exist at resolved location.");
        return;
    }

    std::error_code ec;
    bool removed = std::filesystem::remove(absolutePath, ec);

    if (ec)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralWallComponent] Delete failed: " + ec.message());
        return;
    }

    if (removed)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralRoadComponent] File deleted successfully.");
    }
    else
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralRoadComponent] Remove returned false.");
    }

    // Safety check
    if (std::filesystem::exists(absolutePath))
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralRoadComponent] WARNING: File still exists after deletion.");
    }
}

void ProceduralWallComponent::handleMeshModifyMode(NOWA::EventDataPtr eventData)
{
    auto castEventData = boost::static_pointer_cast<EventDataEditorMode>(eventData);

    this->isEditorMeshModifyMode = (castEventData->getManipulationMode() == EditorManager::EDITOR_MESH_MODIFY_MODE);

    this->updateModificationState();
}

void ProceduralWallComponent::handleGameObjectSelected(NOWA::EventDataPtr eventData)
{
    auto castEventData = boost::static_pointer_cast<EventDataGameObjectSelected>(eventData);

    if (castEventData->getGameObjectId() == this->gameObjectPtr->getId())
    {
        this->isSelected = castEventData->getIsSelected();
    }
    else if (castEventData->getIsSelected())
    {
        this->isSelected = false;
    }

    this->updateModificationState();
}

void ProceduralWallComponent::handleComponentManuallyDeleted(NOWA::EventDataPtr eventData)
{
    boost::shared_ptr<EventDataDeleteComponent> castEventData = boost::static_pointer_cast<EventDataDeleteComponent>(eventData);
    // Found the game object
    if (this->gameObjectPtr->getId() == castEventData->getGameObjectId())
    {
        if (this->getClassName() == castEventData->getComponentName())
        {
            this->deleteWallDataFile();
        }
    }
}

void ProceduralWallComponent::addInputListener(void)
{
    const Ogre::String listenerName = ProceduralWallComponent::getStaticClassName() + "_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
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
    NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralWallComponent::addInputListener");
}

void ProceduralWallComponent::removeInputListener(void)
{
    const Ogre::String listenerName = ProceduralWallComponent::getStaticClassName() + "_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
    NOWA::GraphicsModule::RenderCommand renderCommand = [this, listenerName]()
    {
        if (auto* core = InputDeviceCore::getSingletonPtr())
        {
            core->removeKeyListener(listenerName);
            core->removeMouseListener(listenerName);
        }
    };
    NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralWallComponent::removeInputListener");
}

void ProceduralWallComponent::updateModificationState(void)
{
    bool shouldBeActive = this->activated->getBool() && this->isEditorMeshModifyMode && this->isSelected;

    if (shouldBeActive)
    {
        this->addInputListener();
    }
    else
    {
        this->removeInputListener();

        if (this->buildState == BuildState::DRAGGING)
        {
            this->cancelWall();
        }
    }
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
            // Double-check wallItem still exists
            if (nullptr == this->wallItem)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] wallItem is null, datablock will be applied on next mesh creation");
                return;
            }

            // Verify we have at least 1 submesh
            if (this->wallItem->getNumSubItems() < 1)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralWallComponent] wallItem has no submeshes! Cannot apply wall datablock.");
                return;
            }

            Ogre::HlmsDatablock* wallDb = Ogre::Root::getSingletonPtr()->getHlmsManager()->getDatablockNoDefault(datablock);
            if (nullptr != wallDb)
            {
                // Submesh 0 is ALWAYS walls
                this->wallItem->getSubItem(0)->setDatablock(wallDb);
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] Applied wall datablock: " + datablock);
            }
            else
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralWallComponent] Wall datablock not found: " + datablock);
            }
        };
        // Use enqueueAndWait to ensure it executes immediately
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralWallComponent::setWallDatablock");
    }
    else
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] Wall datablock will be applied when mesh is created (wallItem: " + Ogre::StringConverter::toString(this->wallItem != nullptr) +
                                                                               ", datablock empty: " + Ogre::StringConverter::toString(datablock.empty()) + ")");
    }
}

Ogre::String ProceduralWallComponent::getWallDatablock(void) const
{
    return this->wallDatablock->getString();
}

void ProceduralWallComponent::setPillarDatablock(const Ogre::String& datablock)
{
    this->pillarDatablock->setValue(datablock);

    // Apply datablock immediately if wall exists
    if (this->wallItem && !datablock.empty())
    {
        GraphicsModule::RenderCommand renderCommand = [this, datablock]()
        {
            // Double-check wallItem still exists (could be destroyed between check and execution)
            if (nullptr == this->wallItem)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] wallItem is null, datablock will be applied on next mesh creation");
                return;
            }

            // Verify we have at least 2 submeshes
            if (this->wallItem->getNumSubItems() < 2)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralWallComponent] wallItem has fewer than 2 submeshes! Cannot apply pillar datablock.");
                return;
            }

            Ogre::HlmsDatablock* pillarDb = Ogre::Root::getSingletonPtr()->getHlmsManager()->getDatablockNoDefault(datablock);
            if (nullptr != pillarDb)
            {
                // Submesh 1 is ALWAYS pillars
                this->wallItem->getSubItem(1)->setDatablock(pillarDb);
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] Applied pillar datablock: " + datablock);
            }
            else
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralWallComponent] Pillar datablock not found: " + datablock);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralWallComponent::setPillarDatablock");
    }
    else
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralWallComponent] Pillar datablock will be applied when mesh is created (wallItem: " + Ogre::StringConverter::toString(this->wallItem != nullptr) +
                                                                               ", datablock empty: " + Ogre::StringConverter::toString(datablock.empty()) + ")");
    }
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
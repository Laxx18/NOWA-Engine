/*
Copyright (c) 2025 Lukas Kalinowski

GPL v3
*/

#include "NOWAPrecompiled.h"
#include "MeshModifyComponent.h"
#include "gameobject/GameObjectFactory.h"
#include "main/AppStateManager.h"
#include "main/EventManager.h"
#include "modules/LuaScriptApi.h"
#include "utilities/XMLConverter.h"

#include "OgreAbiUtils.h"
#include "OgreBitwise.h"
#include "OgreHlmsPbsDatablock.h"
#include "OgreItem.h"
#include "OgreMesh2.h"
#include "OgreMesh2Serializer.h"
#include "OgreRenderSystem.h"
#include "OgreRoot.h"
#include "OgreSubMesh2.h"
#include "Vao/OgreAsyncTicket.h"
#include "Vao/OgreVaoManager.h"
#include "Vao/OgreVertexArrayObject.h"

#include "editor/EditorManager.h"

namespace NOWA
{
using namespace rapidxml;
using namespace luabind;

MeshModifyComponent::MeshModifyComponent()
    : GameObjectComponent(), name("MeshModifyComponent"), editableItem(nullptr), dynamicVertexBuffer(nullptr), dynamicIndexBuffer(nullptr), isIndices32(false), vertexCount(0), indexCount(0), bytesPerVertex(0),
      brushImageWidth(0), brushImageHeight(0), raySceneQuery(nullptr), isPressing(false), isCtrlPressed(false), lastBrushPosition(Ogre::Vector3::ZERO), pendingUpload(false), canModify(false),
      activated(new Variant(MeshModifyComponent::AttrActivated(), true, this->attributes)), brushName(new Variant(MeshModifyComponent::AttrBrushName(), this->attributes)),
      brushSize(new Variant(MeshModifyComponent::AttrBrushSize(), 1.0f, this->attributes)), brushIntensity(new Variant(MeshModifyComponent::AttrBrushIntensity(), 0.1f, this->attributes)),
      brushFalloff(new Variant(MeshModifyComponent::AttrBrushFalloff(), 2.0f, this->attributes)), brushMode(new Variant(MeshModifyComponent::AttrBrushMode(), Ogre::String("Push"), this->attributes)),
      category(new Variant(MeshModifyComponent::AttrCategory(), Ogre::String(), this->attributes))
{
}

MeshModifyComponent::~MeshModifyComponent(void)
{
}

const Ogre::String& MeshModifyComponent::getName() const
{
    return this->name;
}

void MeshModifyComponent::install(const Ogre::NameValuePairList* options)
{
    GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<MeshModifyComponent>(MeshModifyComponent::getStaticClassId(), MeshModifyComponent::getStaticClassName());
}

void MeshModifyComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
{
    outAbiCookie = Ogre::generateAbiCookie();
}

bool MeshModifyComponent::init(rapidxml::xml_node<>*& propertyElement)
{
    GameObjectComponent::init(propertyElement);

    if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == MeshModifyComponent::AttrActivated())
    {
        this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
        propertyElement = propertyElement->next_sibling("property");
    }
    if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == MeshModifyComponent::AttrBrushName())
    {
        this->brushName->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
        propertyElement = propertyElement->next_sibling("property");
    }
    if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == MeshModifyComponent::AttrBrushSize())
    {
        this->brushSize->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.0f));
        propertyElement = propertyElement->next_sibling("property");
    }
    if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == MeshModifyComponent::AttrBrushIntensity())
    {
        this->brushIntensity->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.1f));
        propertyElement = propertyElement->next_sibling("property");
    }
    if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == MeshModifyComponent::AttrBrushFalloff())
    {
        this->brushFalloff->setValue(XMLConverter::getAttribReal(propertyElement, "data", 2.0f));
        propertyElement = propertyElement->next_sibling("property");
    }
    if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == MeshModifyComponent::AttrBrushMode())
    {
        this->brushMode->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", "Push"));
        propertyElement = propertyElement->next_sibling("property");
    }
    if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == MeshModifyComponent::AttrCategory())
    {
        this->category->setValue(XMLConverter::getAttrib(propertyElement, "data"));
        propertyElement = propertyElement->next_sibling("property");
    }

    return true;
}

GameObjectCompPtr MeshModifyComponent::clone(GameObjectPtr clonedGameObjectPtr)
{
    MeshModifyComponentPtr clonedCompPtr(boost::make_shared<MeshModifyComponent>());

    clonedCompPtr->setActivated(this->activated->getBool());
    clonedCompPtr->setBrushSize(this->brushSize->getReal());
    clonedCompPtr->setBrushIntensity(this->brushIntensity->getReal());
    clonedCompPtr->setBrushFalloff(this->brushFalloff->getReal());
    clonedCompPtr->setBrushMode(this->getBrushModeString());
    clonedCompPtr->setCategory(this->category->getString());

    clonedGameObjectPtr->addComponent(clonedCompPtr);
    clonedCompPtr->setOwner(clonedGameObjectPtr);

    GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));

    return clonedCompPtr;
}

bool MeshModifyComponent::postInit(void)
{
    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MeshModifyComponent] Init component for game object: " + this->gameObjectPtr->getName());

    NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &MeshModifyComponent::handleMeshModifyMode), NOWA::EventDataEditorMode::getStaticEventType());
    NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &MeshModifyComponent::handleGameObjectSelected),
                                                                             NOWA::EventDataGameObjectSelected::getStaticEventType());

    // Setup brush list
    Ogre::StringVectorPtr brushNames = Ogre::ResourceGroupManager::getSingleton().findResourceNames("Brushes", "*.png");
    std::vector<Ogre::String> compatibleBrushNames;

    if (brushNames.get())
    {
        for (const auto& brushFile : *brushNames)
        {
            compatibleBrushNames.push_back(brushFile);
        }
    }

    this->brushName->setValue(compatibleBrushNames);
    if (compatibleBrushNames.size() > 0)
    {
        this->brushName->setListSelectedValue(compatibleBrushNames[0]);
    }
    this->brushName->addUserData(GameObject::AttrActionImage());
    this->brushName->addUserData(GameObject::AttrActionNoUndo());

    // Setup constraints
    this->brushSize->setConstraints(0.01f, 100.0f);
    this->brushSize->addUserData(GameObject::AttrActionNoUndo());

    this->brushIntensity->setConstraints(0.001f, 1.0f);
    this->brushIntensity->addUserData(GameObject::AttrActionNoUndo());

    this->brushFalloff->setConstraints(0.1f, 10.0f);
    this->brushFalloff->addUserData(GameObject::AttrActionNoUndo());

    // Setup brush mode list
    std::vector<Ogre::String> brushModes;
    brushModes.push_back("Push");
    brushModes.push_back("Pull");
    brushModes.push_back("Smooth");
    brushModes.push_back("Flatten");
    brushModes.push_back("Pinch");
    brushModes.push_back("Inflate");
    this->brushMode->setValue(brushModes);
    this->brushMode->setListSelectedValue("Push");

    this->addInputListener();

    // Create the editable mesh
    if (false == this->createEditableMesh())
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MeshModifyComponent] Failed to create editable mesh for: " + this->gameObjectPtr->getName());
        return false;
    }

    return true;
}

bool MeshModifyComponent::connect(void)
{
    GameObjectComponent::connect();
    return true;
}

bool MeshModifyComponent::disconnect(void)
{
    GameObjectComponent::disconnect();
    return true;
}

bool MeshModifyComponent::onCloned(void)
{
    return true;
}

void MeshModifyComponent::onAddComponent(void)
{
    boost::shared_ptr<EventDataEditorMode> eventDataEditorMode(new EventDataEditorMode(EditorManager::EDITOR_MESH_MODIFY_MODE));
    NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataEditorMode);
    this->canModify = true;
}

void MeshModifyComponent::onRemoveComponent(void)
{
    GameObjectComponent::onRemoveComponent();

    NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &MeshModifyComponent::handleMeshModifyMode), NOWA::EventDataEditorMode::getStaticEventType());
    NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &MeshModifyComponent::handleGameObjectSelected),
                                                                                NOWA::EventDataGameObjectSelected::getStaticEventType());

    this->removeInputListener();

    // Destroy ray query
    if (nullptr != this->raySceneQuery)
    {
        this->gameObjectPtr->getSceneManager()->destroyQuery(this->raySceneQuery);
        this->raySceneQuery = nullptr;
    }

    // Cleanup editable mesh
    this->destroyEditableMesh();
}

void MeshModifyComponent::onOtherComponentRemoved(unsigned int index)
{
}

void MeshModifyComponent::onOtherComponentAdded(unsigned int index)
{
}

void MeshModifyComponent::update(Ogre::Real dt, bool notSimulating)
{
    // Check for Ctrl key state
    this->isCtrlPressed = InputDeviceCore::getSingletonPtr()->getKeyboard()->isKeyDown(OIS::KC_LCONTROL) || InputDeviceCore::getSingletonPtr()->getKeyboard()->isKeyDown(OIS::KC_RCONTROL);
}

void MeshModifyComponent::actualizeValue(Variant* attribute)
{
    GameObjectComponent::actualizeValue(attribute);

    if (MeshModifyComponent::AttrActivated() == attribute->getName())
    {
        this->setActivated(attribute->getBool());
    }
    else if (MeshModifyComponent::AttrBrushName() == attribute->getName())
    {
        this->setBrushName(attribute->getListSelectedValue());
    }
    else if (MeshModifyComponent::AttrBrushSize() == attribute->getName())
    {
        this->setBrushSize(attribute->getReal());
    }
    else if (MeshModifyComponent::AttrBrushIntensity() == attribute->getName())
    {
        this->setBrushIntensity(attribute->getReal());
    }
    else if (MeshModifyComponent::AttrBrushFalloff() == attribute->getName())
    {
        this->setBrushFalloff(attribute->getReal());
    }
    else if (MeshModifyComponent::AttrBrushMode() == attribute->getName())
    {
        this->setBrushMode(attribute->getListSelectedValue());
    }
    else if (MeshModifyComponent::AttrCategory() == attribute->getName())
    {
        this->setCategory(attribute->getString());
    }
}

void MeshModifyComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
{
    GameObjectComponent::writeXML(propertiesXML, doc);

    xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
    propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
    propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
    propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
    propertiesXML->append_node(propertyXML);

    propertyXML = doc.allocate_node(node_element, "property");
    propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
    propertyXML->append_attribute(doc.allocate_attribute("name", "Brush Name"));
    propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->brushName->getListSelectedValue())));
    propertiesXML->append_node(propertyXML);

    propertyXML = doc.allocate_node(node_element, "property");
    propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
    propertyXML->append_attribute(doc.allocate_attribute("name", "Brush Size"));
    propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->brushSize->getReal())));
    propertiesXML->append_node(propertyXML);

    propertyXML = doc.allocate_node(node_element, "property");
    propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
    propertyXML->append_attribute(doc.allocate_attribute("name", "Brush Intensity"));
    propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->brushIntensity->getReal())));
    propertiesXML->append_node(propertyXML);

    propertyXML = doc.allocate_node(node_element, "property");
    propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
    propertyXML->append_attribute(doc.allocate_attribute("name", "Brush Falloff"));
    propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->brushFalloff->getReal())));
    propertiesXML->append_node(propertyXML);

    propertyXML = doc.allocate_node(node_element, "property");
    propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
    propertyXML->append_attribute(doc.allocate_attribute("name", "Brush Mode"));
    propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->brushMode->getListSelectedValue())));
    propertiesXML->append_node(propertyXML);

    propertyXML = doc.allocate_node(node_element, "property");
    propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
    propertyXML->append_attribute(doc.allocate_attribute("name", "Category"));
    propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->category->getString())));
    propertiesXML->append_node(propertyXML);
}

Ogre::String MeshModifyComponent::getClassName(void) const
{
    return "MeshModifyComponent";
}

Ogre::String MeshModifyComponent::getParentClassName(void) const
{
    return "GameObjectComponent";
}

void MeshModifyComponent::setActivated(bool activated)
{
    this->activated->setValue(activated);
}

bool MeshModifyComponent::isActivated(void) const
{
    return this->activated->getBool();
}

void MeshModifyComponent::setBrushName(const Ogre::String& brushName)
{
    this->brushName->setListSelectedValue(brushName);

    // Clear existing brush data
    this->brushData.clear();
    this->brushImageWidth = 0;
    this->brushImageHeight = 0;

    if (brushName == "Default (Smooth)" || brushName.empty())
    {
        // Use procedural smooth brush (no image needed)
        return;
    }

    try
    {
        Ogre::Image2 brushImage;
        brushImage.load(brushName, "Brushes");

        this->brushImageWidth = brushImage.getWidth();
        this->brushImageHeight = brushImage.getHeight();

        size_t size = this->brushImageWidth * this->brushImageHeight;
        if (size == 0)
        {
            return;
        }

        this->brushData.resize(size, 0.0f);

        for (Ogre::uint32 y = 0; y < this->brushImageHeight; ++y)
        {
            for (Ogre::uint32 x = 0; x < this->brushImageWidth; ++x)
            {
                Ogre::ColourValue color = brushImage.getColourAt(x, y, 0);
                // Use red channel (or luminance) as brush intensity
                this->brushData[y * this->brushImageWidth + x] = color.r;
            }
        }
    }
    catch (Ogre::Exception& e)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MeshModifyComponent] Failed to load brush image: " + brushName + " - " + e.getDescription());
    }
}

Ogre::String MeshModifyComponent::getBrushName(void) const
{
    return this->brushName->getListSelectedValue();
}

void MeshModifyComponent::setBrushSize(Ogre::Real brushSize)
{
    this->brushSize->setValue(brushSize);
}

Ogre::Real MeshModifyComponent::getBrushSize(void) const
{
    return this->brushSize->getReal();
}

void MeshModifyComponent::setBrushIntensity(Ogre::Real brushIntensity)
{
    this->brushIntensity->setValue(brushIntensity);
}

Ogre::Real MeshModifyComponent::getBrushIntensity(void) const
{
    return this->brushIntensity->getReal();
}

void MeshModifyComponent::setBrushFalloff(Ogre::Real brushFalloff)
{
    this->brushFalloff->setValue(brushFalloff);
}

Ogre::Real MeshModifyComponent::getBrushFalloff(void) const
{
    return this->brushFalloff->getReal();
}

void MeshModifyComponent::setBrushMode(const Ogre::String& brushMode)
{
    this->brushMode->setListSelectedValue(brushMode);
}

Ogre::String MeshModifyComponent::getBrushModeString(void) const
{
    return this->brushMode->getListSelectedValue();
}

MeshModifyComponent::BrushMode MeshModifyComponent::getBrushMode(void) const
{
    Ogre::String mode = this->brushMode->getListSelectedValue();
    if (mode == "Push")
    {
        return BrushMode::PUSH;
    }
    else if (mode == "Pull")
    {
        return BrushMode::PULL;
    }
    else if (mode == "Smooth")
    {
        return BrushMode::SMOOTH;
    }
    else if (mode == "Flatten")
    {
        return BrushMode::FLATTEN;
    }
    else if (mode == "Pinch")
    {
        return BrushMode::PINCH;
    }
    else if (mode == "Inflate")
    {
        return BrushMode::INFLATE;
    }
    return BrushMode::PUSH; // Default
}

void MeshModifyComponent::setCategory(const Ogre::String& category)
{
    this->category->setValue(category);
}

Ogre::String MeshModifyComponent::getCategory(void) const
{
    return this->category->getString();
}

void MeshModifyComponent::resetMesh(void)
{
    if (this->originalVertices.empty())
    {
        return;
    }

    GraphicsModule::RenderCommand renderCommand = [this]()
    {
        // Restore original vertex positions and normals
        this->vertices = this->originalVertices;
        this->normals = this->originalNormals;
        this->tangents = this->originalTangents;

        // Upload to GPU (must be done on render thread)
        // Note: You'll need to wrap this with your render thread mechanism
        this->uploadVertexData();
    };
    NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MeshModifyComponent::resetMesh");
}

bool MeshModifyComponent::exportMesh(const Ogre::String& fileName)
{
    bool success = true;
    if (nullptr == this->editableMesh)
    {
        success = false;
        return success;
    }

    GraphicsModule::RenderCommand renderCommand = [this, fileName, &success]()
    {
        try
        {
            Ogre::MeshSerializer serializer(Ogre::Root::getSingletonPtr()->getRenderSystem()->getVaoManager());
            serializer.exportMesh(this->editableMesh.get(), fileName + ".mesh", Ogre::MESH_VERSION_LATEST, Ogre::Serializer::ENDIAN_NATIVE);
            success = true;
            return;
        }
        catch (Ogre::Exception& e)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MeshModifyComponent] Failed to export mesh: " + e.getDescription());
            success = false;
            return;
        }
    };
    NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MeshModifyComponent::exportMesh");

    return success;
}

size_t MeshModifyComponent::getVertexCount(void) const
{
    return this->vertexCount;
}

size_t MeshModifyComponent::getIndexCount(void) const
{
    return this->indexCount;
}

// ==================== PRIVATE METHODS ====================

bool MeshModifyComponent::createEditableMesh(void)
{
    bool success = true;
    Ogre::Item* originalItem = this->gameObjectPtr->getMovableObject<Ogre::Item>();
    if (nullptr == originalItem)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MeshModifyComponent] GameObject has no Item attached: " + this->gameObjectPtr->getName());
        return false;
    }

    Ogre::MeshPtr originalMesh = originalItem->getMesh();
    if (nullptr == originalMesh)
    {
        return false;
    }

    GraphicsModule::RenderCommand renderCommand = [this, originalItem, &success]()
    {
        // Extract mesh data first
        if (false == this->extractMeshData())
        {
            success = false;
            return;
        }

        // Build vertex adjacency for smoothing operations
        this->buildVertexAdjacency();

        // Create dynamic buffers and mesh
        if (!this->createDynamicBuffers())
        {
            success = false;
            return;
        }

        // Hide original item and show editable one
        originalItem->setVisible(false);
    };
    NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MeshModifyComponent::createEditableMesh");

    return success;
}

void MeshModifyComponent::destroyEditableMesh(void)
{
    GraphicsModule::RenderCommand renderCommand = [this]()
    {
        if (this->editableItem)
        {
            // Detach from scene node
            if (this->editableItem->getParentSceneNode())
            {
                this->editableItem->getParentSceneNode()->detachObject(this->editableItem);
            }

            // Destroy item
            this->gameObjectPtr->getSceneManager()->destroyItem(this->editableItem);
            this->editableItem = nullptr;
        }

        // Unmap any persistently mapped buffers
        if (this->dynamicVertexBuffer)
        {
            if (this->dynamicVertexBuffer->getMappingState() != Ogre::MS_UNMAPPED)
            {
                this->dynamicVertexBuffer->unmap(Ogre::UO_UNMAP_ALL);
            }
            this->dynamicVertexBuffer = nullptr;
        }

        if (this->dynamicIndexBuffer)
        {
            this->dynamicIndexBuffer = nullptr;
        }

        // Clear mesh reference (VaoManager will handle buffer destruction)
        if (this->editableMesh)
        {
            Ogre::MeshManager::getSingleton().remove(this->editableMesh);
            this->editableMesh.reset();
        }

        // Clear CPU-side data
        this->vertices.clear();
        this->normals.clear();
        this->tangents.clear();
        this->uvCoordinates.clear();
        this->indices.clear();
        this->originalVertices.clear();
        this->originalNormals.clear();
        this->originalTangents.clear();
        this->vertexNeighbors.clear();

        // Show original item again
        Ogre::Item* originalItem = this->gameObjectPtr->getMovableObject<Ogre::Item>();
        if (originalItem)
        {
            originalItem->setVisible(true);
        }
    };
    NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MeshModifyComponent::destroyEditableMesh");
}

bool MeshModifyComponent::detectVertexFormat(Ogre::VertexArrayObject* vao)
{
    // Reset format info
    this->vertexFormat = VertexElementInfo();

    const Ogre::VertexBufferPackedVec& vertexBuffers = vao->getVertexBuffers();
    if (vertexBuffers.empty())
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MeshModifyComponent] No vertex buffers found!");
        return false;
    }

    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MeshModifyComponent] Analyzing " + Ogre::StringConverter::toString(vertexBuffers.size()) + " vertex buffer(s)");

    // Iterate through all vertex elements to detect format
    for (size_t bufIdx = 0; bufIdx < vertexBuffers.size(); ++bufIdx)
    {
        const Ogre::VertexBufferPacked* vb = vertexBuffers[bufIdx];
        const Ogre::VertexElement2Vec& elements = vb->getVertexElements();

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MeshModifyComponent] Buffer " + Ogre::StringConverter::toString(bufIdx) + " has " + Ogre::StringConverter::toString(elements.size()) +
                                                                                " elements, " + Ogre::StringConverter::toString(vb->getBytesPerElement()) + " bytes per vertex");

        for (const Ogre::VertexElement2& elem : elements)
        {
            Ogre::String semanticName = "UNKNOWN";
            Ogre::String typeName = "UNKNOWN";

            switch (elem.mType)
            {
            case Ogre::VET_FLOAT1:
                typeName = "FLOAT1";
                break;
            case Ogre::VET_FLOAT2:
                typeName = "FLOAT2";
                break;
            case Ogre::VET_FLOAT3:
                typeName = "FLOAT3";
                break;
            case Ogre::VET_FLOAT4:
                typeName = "FLOAT4";
                break;
            case Ogre::VET_HALF2:
                typeName = "HALF2";
                break;
            case Ogre::VET_HALF4:
                typeName = "HALF4";
                break;
            case Ogre::VET_SHORT2:
                typeName = "SHORT2";
                break;
            case Ogre::VET_SHORT4:
                typeName = "SHORT4";
                break;
            case Ogre::VET_USHORT2:
                typeName = "USHORT2";
                break;
            case Ogre::VET_USHORT4:
                typeName = "USHORT4";
                break;
            case Ogre::VET_UBYTE4:
                typeName = "UBYTE4";
                break;
            case Ogre::VET_UBYTE4_NORM:
                typeName = "UBYTE4_NORM";
                break;
            case Ogre::VET_SHORT2_SNORM:
                typeName = "SHORT2_SNORM";
                break;
            case Ogre::VET_SHORT4_SNORM:
                typeName = "SHORT4_SNORM";
                break;
            case Ogre::VET_USHORT2_NORM:
                typeName = "USHORT2_NORM";
                break;
            case Ogre::VET_USHORT4_NORM:
                typeName = "USHORT4_NORM";
                break;
            default:
                typeName = "TYPE(" + Ogre::StringConverter::toString(elem.mType) + ")";
                break;
            }

            switch (elem.mSemantic)
            {
            case Ogre::VES_POSITION:
                semanticName = "POSITION";
                this->vertexFormat.hasPosition = true;
                this->vertexFormat.positionType = elem.mType;
                break;
            case Ogre::VES_NORMAL:
                semanticName = "NORMAL";
                this->vertexFormat.hasNormal = true;
                this->vertexFormat.normalType = elem.mType;
                break;
            case Ogre::VES_TANGENT:
                semanticName = "TANGENT";
                this->vertexFormat.hasTangent = true;
                this->vertexFormat.tangentType = elem.mType;
                // Check if it's QTangent format (SHORT4_SNORM is used for quaternion tangents)
                if (elem.mType == Ogre::VET_SHORT4_SNORM)
                {
                    this->vertexFormat.hasQTangent = true;
                    semanticName = "TANGENT (QTangent)";
                }
                break;
            case Ogre::VES_TEXTURE_COORDINATES:
                semanticName = "TEXCOORD";
                this->vertexFormat.hasUV = true;
                this->vertexFormat.uvType = elem.mType;
                break;
            case Ogre::VES_BINORMAL:
                semanticName = "BINORMAL";
                break;
            case Ogre::VES_DIFFUSE:
                semanticName = "DIFFUSE";
                break;
            case Ogre::VES_BLEND_WEIGHTS:
                semanticName = "BLEND_WEIGHTS";
                break;
            case Ogre::VES_BLEND_INDICES:
                semanticName = "BLEND_INDICES";
                break;
            default:
                semanticName = "OTHER(" + Ogre::StringConverter::toString(elem.mSemantic) + ")";
                break;
            }

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MeshModifyComponent]   Element: " + semanticName + " Type: " + typeName);
        }
    }

    Ogre::LogManager::getSingletonPtr()->logMessage(
        Ogre::LML_CRITICAL, "[MeshModifyComponent] Final vertex format - Position: " + Ogre::StringConverter::toString(this->vertexFormat.hasPosition) +
                                ", Normal: " + Ogre::StringConverter::toString(this->vertexFormat.hasNormal) + ", Tangent: " + Ogre::StringConverter::toString(this->vertexFormat.hasTangent) +
                                ", QTangent: " + Ogre::StringConverter::toString(this->vertexFormat.hasQTangent) + ", UV: " + Ogre::StringConverter::toString(this->vertexFormat.hasUV));

    return this->vertexFormat.hasPosition;
}

bool MeshModifyComponent::extractMeshData(void)
{
    Ogre::Item* originalItem = this->gameObjectPtr->getMovableObject<Ogre::Item>();
    if (nullptr == originalItem)
    {
        return false;
    }

    Ogre::MeshPtr mesh = originalItem->getMesh();
    if (nullptr == mesh)
    {
        return false;
    }

    // Count total vertices and indices across all submeshes
    unsigned int numVertices = 0;
    unsigned int numIndices = 0;

    for (const auto& subMesh : mesh->getSubMeshes())
    {
        if (subMesh->mVao[Ogre::VpNormal].empty())
        {
            continue;
        }

        numVertices += static_cast<unsigned int>(subMesh->mVao[Ogre::VpNormal][0]->getVertexBuffers()[0]->getNumElements());
        if (subMesh->mVao[Ogre::VpNormal][0]->getIndexBuffer())
        {
            numIndices += static_cast<unsigned int>(subMesh->mVao[Ogre::VpNormal][0]->getIndexBuffer()->getNumElements());
        }
    }

    if (numVertices == 0)
    {
        return false;
    }

    this->vertexCount = numVertices;
    this->indexCount = numIndices;

    // Allocate storage
    this->vertices.resize(this->vertexCount);
    this->normals.resize(this->vertexCount);
    this->tangents.resize(this->vertexCount, Ogre::Vector4(1, 0, 0, 1)); // Default tangent
    this->uvCoordinates.resize(this->vertexCount);
    this->indices.resize(this->indexCount);

    unsigned int addedIndices = 0;
    unsigned int index_offset = 0;
    unsigned int subMeshOffset = 0;

    for (const auto& subMesh : mesh->getSubMeshes())
    {
        if (subMesh->mVao[Ogre::VpNormal].empty())
        {
            continue;
        }

        Ogre::VertexArrayObject* vao = subMesh->mVao[Ogre::VpNormal][0];

        // Detect vertex format from first submesh
        if (vao->getIndexBuffer())
        {
            this->isIndices32 = (vao->getIndexBuffer()->getIndexType() == Ogre::IndexBufferPacked::IT_32BIT);
        }

        // Check index format
        if (subMeshOffset == 0)
        {
            this->detectVertexFormat(vao);
        }

        // Setup read requests for position, normal, and UV
        Ogre::VertexArrayObject::ReadRequestsVec requests;
        requests.push_back(Ogre::VertexArrayObject::ReadRequests(Ogre::VES_POSITION));
        requests.push_back(Ogre::VertexArrayObject::ReadRequests(Ogre::VES_NORMAL));
        requests.push_back(Ogre::VertexArrayObject::ReadRequests(Ogre::VES_TEXTURE_COORDINATES));

        vao->readRequests(requests);
        vao->mapAsyncTickets(requests);

        unsigned int subMeshVerticesNum = static_cast<unsigned int>(requests[0].vertexBuffer->getNumElements());

        // Check if the NORMAL is actually a QTangent (SHORT4_SNORM)
        bool isQTangent = (requests[1].type == Ogre::VET_SHORT4_SNORM);

        Ogre::LogManager::getSingletonPtr()->logMessage(
            Ogre::LML_TRIVIAL, "[MeshModifyComponent] SubMesh: " + Ogre::StringConverter::toString(subMeshVerticesNum) + " vertices, Position type: " + Ogre::StringConverter::toString(requests[0].type) +
                                   ", Normal type: " + Ogre::StringConverter::toString(requests[1].type) + ", isQTangent: " + Ogre::StringConverter::toString(isQTangent));

        // Extract data based on vertex format
        for (size_t i = 0; i < subMeshVerticesNum; ++i)
        {
            // ========== POSITION ==========
            if (requests[0].type == Ogre::VET_HALF4)
            {
                const Ogre::uint16* posData = reinterpret_cast<const Ogre::uint16*>(requests[0].data);
                this->vertices[i + subMeshOffset].x = Ogre::Bitwise::halfToFloat(posData[0]);
                this->vertices[i + subMeshOffset].y = Ogre::Bitwise::halfToFloat(posData[1]);
                this->vertices[i + subMeshOffset].z = Ogre::Bitwise::halfToFloat(posData[2]);
            }
            else // VET_FLOAT3
            {
                const float* posData = reinterpret_cast<const float*>(requests[0].data);
                this->vertices[i + subMeshOffset].x = posData[0];
                this->vertices[i + subMeshOffset].y = posData[1];
                this->vertices[i + subMeshOffset].z = posData[2];
            }

            // ========== NORMAL (may be QTangent) ==========
            if (isQTangent)
            {
                // SHORT4_SNORM is a QTangent - quaternion encoding both normal and tangent
                // Based on Ogre's dearrangeToInefficient():
                // Normal = qTangent.xAxis()
                // Tangent = qTangent.yAxis()
                // Reflection = sign(qTangent.w)
                const Ogre::int16* normData = reinterpret_cast<const Ogre::int16*>(requests[1].data);

                // Convert SHORT_SNORM to float: value / 32767.0f
                Ogre::Quaternion qTangent;
                qTangent.x = static_cast<float>(normData[0]) / 32767.0f;
                qTangent.y = static_cast<float>(normData[1]) / 32767.0f;
                qTangent.z = static_cast<float>(normData[2]) / 32767.0f;
                qTangent.w = static_cast<float>(normData[3]) / 32767.0f;

                // Extract reflection from w sign
                float reflection = (qTangent.w < 0) ? -1.0f : 1.0f;

                // Extract normal (x-axis of quaternion rotation)
                Ogre::Vector3 vNormal = qTangent.xAxis();
                // Extract tangent (y-axis of quaternion rotation)
                Ogre::Vector3 vTangent = qTangent.yAxis();

                this->normals[i + subMeshOffset] = vNormal;
                this->tangents[i + subMeshOffset] = Ogre::Vector4(vTangent.x, vTangent.y, vTangent.z, reflection);
                // Normal (HALF4)
                // Mark that we have real tangents
                this->vertexFormat.hasTangent = true;
            }
            else if (requests[1].type == Ogre::VET_HALF4)
            {
                const Ogre::uint16* normData = reinterpret_cast<const Ogre::uint16*>(requests[1].data);
                this->normals[i + subMeshOffset].x = Ogre::Bitwise::halfToFloat(normData[0]);
                this->normals[i + subMeshOffset].y = Ogre::Bitwise::halfToFloat(normData[1]);
                this->normals[i + subMeshOffset].z = Ogre::Bitwise::halfToFloat(normData[2]);
                // UV (HALF2)
            }
            else // VET_FLOAT3
            {
                // Normal (FLOAT3)
                const float* normData = reinterpret_cast<const float*>(requests[1].data);
                this->normals[i + subMeshOffset].x = normData[0];
                this->normals[i + subMeshOffset].y = normData[1];
                this->normals[i + subMeshOffset].z = normData[2];
            }

            // UV (FLOAT2)
            if (requests[2].type == Ogre::VET_HALF2)
            {
                const Ogre::uint16* uvData = reinterpret_cast<const Ogre::uint16*>(requests[2].data);
                this->uvCoordinates[i + subMeshOffset].x = Ogre::Bitwise::halfToFloat(uvData[0]);
                this->uvCoordinates[i + subMeshOffset].y = Ogre::Bitwise::halfToFloat(uvData[1]);
            }
            else // VET_FLOAT2
            {
                const float* uvData = reinterpret_cast<const float*>(requests[2].data);
                this->uvCoordinates[i + subMeshOffset].x = uvData[0];
                this->uvCoordinates[i + subMeshOffset].y = uvData[1];
            }

            // Advance pointers
            requests[0].data += requests[0].vertexBuffer->getBytesPerElement();
            requests[1].data += requests[1].vertexBuffer->getBytesPerElement();
            requests[2].data += requests[2].vertexBuffer->getBytesPerElement();
        }

        subMeshOffset += subMeshVerticesNum;
        vao->unmapAsyncTickets(requests);

        // Extract index data
        Ogre::IndexBufferPacked* indexBuffer = vao->getIndexBuffer();
        if (indexBuffer)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MeshModifyComponent] Reading index buffer: " + Ogre::StringConverter::toString(indexBuffer->getNumElements()) + " indices, " +
                                                                                   "isIndices32: " + Ogre::StringConverter::toString(this->isIndices32) +
                                                                                   ", index_offset: " + Ogre::StringConverter::toString(index_offset));

            // Try to use shadow copy first (faster, no GPU sync needed)
            const void* shadowData = indexBuffer->getShadowCopy();

            if (shadowData)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MeshModifyComponent] Using shadow copy for index buffer");

                if (this->isIndices32)
                {
                    const Ogre::uint32* pIndices = reinterpret_cast<const Ogre::uint32*>(shadowData);
                    /*Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
                        "[MeshModifyComponent] RAW indices (32-bit shadow): " +
                        Ogre::StringConverter::toString(pIndices[0]) + ", " +
                        Ogre::StringConverter::toString(pIndices[1]) + ", " +
                        Ogre::StringConverter::toString(pIndices[2]) + ", " +
                        Ogre::StringConverter::toString(pIndices[3]) + ", " +
                        Ogre::StringConverter::toString(pIndices[4]) + ", " +
                        Ogre::StringConverter::toString(pIndices[5]));*/

                    for (size_t i = 0; i < indexBuffer->getNumElements(); i++)
                    {
                        this->indices[addedIndices++] = pIndices[i] + index_offset;
                    }
                }
                else
                {
                    const Ogre::uint16* pShortIndices = reinterpret_cast<const Ogre::uint16*>(shadowData);
                    /*Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
                        "[MeshModifyComponent] RAW indices (16-bit shadow): " +
                        Ogre::StringConverter::toString(pShortIndices[0]) + ", " +
                        Ogre::StringConverter::toString(pShortIndices[1]) + ", " +
                        Ogre::StringConverter::toString(pShortIndices[2]) + ", " +
                        Ogre::StringConverter::toString(pShortIndices[3]) + ", " +
                        Ogre::StringConverter::toString(pShortIndices[4]) + ", " +
                        Ogre::StringConverter::toString(pShortIndices[5]));*/

                    for (size_t i = 0; i < indexBuffer->getNumElements(); i++)
                    {
                        this->indices[addedIndices++] = static_cast<Ogre::uint32>(pShortIndices[i]) + index_offset;
                    }
                }
            }
            else
            {
                // No shadow copy - use async ticket (requires GPU sync)
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MeshModifyComponent] Using async ticket for index buffer (no shadow copy)");

                Ogre::AsyncTicketPtr asyncTicket = indexBuffer->readRequest(0, indexBuffer->getNumElements());

                if (this->isIndices32)
                {
                    const Ogre::uint32* pIndices = reinterpret_cast<const Ogre::uint32*>(asyncTicket->map());

                    /*Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
                        "[MeshModifyComponent] RAW indices (32-bit async): " +
                        Ogre::StringConverter::toString(pIndices[0]) + ", " +
                        Ogre::StringConverter::toString(pIndices[1]) + ", " +
                        Ogre::StringConverter::toString(pIndices[2]) + ", " +
                        Ogre::StringConverter::toString(pIndices[3]) + ", " +
                        Ogre::StringConverter::toString(pIndices[4]) + ", " +
                        Ogre::StringConverter::toString(pIndices[5]));*/

                    for (size_t i = 0; i < indexBuffer->getNumElements(); i++)
                    {
                        this->indices[addedIndices++] = pIndices[i] + index_offset;
                    }
                }
                else
                {
                    const Ogre::uint16* pShortIndices = reinterpret_cast<const Ogre::uint16*>(asyncTicket->map());

                    /*Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
                        "[MeshModifyComponent] RAW indices (16-bit async): " +
                        Ogre::StringConverter::toString(pShortIndices[0]) + ", " +
                        Ogre::StringConverter::toString(pShortIndices[1]) + ", " +
                        Ogre::StringConverter::toString(pShortIndices[2]) + ", " +
                        Ogre::StringConverter::toString(pShortIndices[3]) + ", " +
                        Ogre::StringConverter::toString(pShortIndices[4]) + ", " +
                        Ogre::StringConverter::toString(pShortIndices[5]));*/

                    for (size_t i = 0; i < indexBuffer->getNumElements(); i++)
                    {
                        this->indices[addedIndices++] = static_cast<Ogre::uint32>(pShortIndices[i]) + index_offset;
                    }
                }

                asyncTicket->unmap();
            }
        }
        else
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MeshModifyComponent] ERROR: No index buffer found!");
        }

        index_offset += static_cast<unsigned int>(vao->getVertexBuffers()[0]->getNumElements());
    }

    // Store original data for reset
    this->originalVertices = this->vertices;
    this->originalNormals = this->normals;
    this->originalTangents = this->tangents;

    //// Debug: Print first few vertices and indices to verify data
    // Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MeshModifyComponent] === DEBUG: First 5 vertices ===");
    // for (size_t i = 0; i < std::min<size_t>(5, this->vertexCount); ++i)
    //{
    //	Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
    //		"[MeshModifyComponent] v[" + Ogre::StringConverter::toString(i) + "] = (" +
    //		Ogre::StringConverter::toString(this->vertices[i].x, 4) + ", " +
    //		Ogre::StringConverter::toString(this->vertices[i].y, 4) + ", " +
    //		Ogre::StringConverter::toString(this->vertices[i].z, 4) + ")");
    // }

    // Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
    //	"[MeshModifyComponent] === DEBUG: First 15 indices (5 triangles) ===");
    // for (size_t i = 0; i < std::min<size_t>(15, this->indexCount); i += 3)
    //{
    //	Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
    //		"[MeshModifyComponent] tri[" + Ogre::StringConverter::toString(i / 3) + "] = " +
    //		Ogre::StringConverter::toString(this->indices[i]) + ", " +
    //		Ogre::StringConverter::toString(this->indices[i + 1]) + ", " +
    //		Ogre::StringConverter::toString(this->indices[i + 2]));
    // }

    // Check if any index is out of bounds
    size_t maxIndex = 0;
    for (size_t i = 0; i < this->indexCount; ++i)
    {
        if (this->indices[i] > maxIndex)
        {
            maxIndex = this->indices[i];
        }
        if (this->indices[i] >= this->vertexCount)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MeshModifyComponent] ERROR: Index " + Ogre::StringConverter::toString(i) + " = " +
                                                                                    Ogre::StringConverter::toString(this->indices[i]) + " is >= vertexCount " + Ogre::StringConverter::toString(this->vertexCount));
        }
    }
    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
                                                    "[MeshModifyComponent] Max index value: " + Ogre::StringConverter::toString(maxIndex) + ", vertexCount: " + Ogre::StringConverter::toString(this->vertexCount));

    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MeshModifyComponent] Extracted " + Ogre::StringConverter::toString(this->vertexCount) + " vertices and " +
                                                                           Ogre::StringConverter::toString(this->indexCount) + " indices" +
                                                                           (this->vertexFormat.hasTangent ? " (with tangents from QTangent)" : " (no tangents)"));

    return true;
}

bool MeshModifyComponent::createDynamicBuffers(void)
{
    Ogre::Root* root = Ogre::Root::getSingletonPtr();
    Ogre::RenderSystem* renderSystem = root->getRenderSystem();
    Ogre::VaoManager* vaoManager = renderSystem->getVaoManager();

    // Create a unique name for the editable mesh
    Ogre::String meshName = this->gameObjectPtr->getName() + "_Editable_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());

    const Ogre::String groupName = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;

    this->editableMesh = Ogre::MeshManager::getSingleton().createManual(meshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    // Remove existing mesh if any (Ogre-Next: remove by ResourceHandle)
    // CRITICAL: Set the VaoManager - this is required for Ogre-Next!
    this->editableMesh->_setVaoManager(vaoManager);

    // Create submesh
    Ogre::SubMesh* subMesh = this->editableMesh->createSubMesh();

    // Check if the datablock needs tangents (for normal mapping)
    bool needsTangents = false;
    Ogre::Item* originalItem = this->gameObjectPtr->getMovableObject<Ogre::Item>();
    if (originalItem && originalItem->getNumSubItems() > 0)
    {
        Ogre::HlmsDatablock* datablock = originalItem->getSubItem(0)->getDatablock();
        if (datablock)
        {
            // Check if it's a PBS datablock with normal maps
            Ogre::HlmsPbsDatablock* pbsDatablock = dynamic_cast<Ogre::HlmsPbsDatablock*>(datablock);
            if (pbsDatablock)
            {
                // Check if any normal map texture is set
                Ogre::TextureGpu* normalTex = pbsDatablock->getTexture(Ogre::PBSM_NORMAL);
                if (normalTex)
                {
                    needsTangents = true;
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MeshModifyComponent] Datablock has normal map - tangents required");
                }
            }
        }
    }

    // If original mesh had tangents OR datablock needs them, we include tangents
    bool includeTangents = this->vertexFormat.hasTangent || needsTangents;

    // Create the mesh
    // If we need tangents but don't have them, generate them
    if (includeTangents && !this->vertexFormat.hasTangent)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MeshModifyComponent] Generating tangents (original mesh didn't have them)");

        // Create submesh
        this->tangents.resize(this->vertexCount, Ogre::Vector4(1, 0, 0, 1));
        this->originalTangents.resize(this->vertexCount, Ogre::Vector4(1, 0, 0, 1));
        this->vertexFormat.hasTangent = true;
        this->recalculateTangents();
        this->originalTangents = this->tangents; // Store as original
    }

    // Define vertex elements: Position + Normal + UV (interleaved float)
    Ogre::VertexElement2Vec vertexElements;
    vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_POSITION));
    vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_NORMAL));

    if (includeTangents)
    {
        vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT4, Ogre::VES_TANGENT));
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MeshModifyComponent] Adding TANGENT to vertex format (VET_FLOAT4)");
    }

    vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES));

    // Calculate floats per vertex
    size_t floatsPerVertex = 3 + 3 + 2; // pos + normal + uv
    if (includeTangents)
    {
        floatsPerVertex += 4; // tangent (xyzw)
    }

    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MeshModifyComponent] Creating vertex buffer with " + Ogre::StringConverter::toString(floatsPerVertex) + " floats per vertex, " +
                                                                           Ogre::StringConverter::toString(this->vertexCount) + " vertices");

    // Prepare interleaved vertex data
    const size_t vertexDataSize = this->vertexCount * floatsPerVertex * sizeof(float);
    float* vertexData = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(vertexDataSize, Ogre::MEMCATEGORY_GEOMETRY));

    for (size_t i = 0; i < this->vertexCount; ++i)
    {
        size_t offset = i * floatsPerVertex;

        // Position
        vertexData[offset + 0] = this->vertices[i].x;
        vertexData[offset + 1] = this->vertices[i].y;
        vertexData[offset + 2] = this->vertices[i].z;

        // Normal
        vertexData[offset + 3] = this->normals[i].x;
        vertexData[offset + 4] = this->normals[i].y;
        vertexData[offset + 5] = this->normals[i].z;

        size_t nextOffset = 6;

        // Tangent (if present)
        if (includeTangents)
        {
            vertexData[offset + nextOffset + 0] = this->tangents[i].x;
            vertexData[offset + nextOffset + 1] = this->tangents[i].y;
            vertexData[offset + nextOffset + 2] = this->tangents[i].z;
            vertexData[offset + nextOffset + 3] = this->tangents[i].w;
            nextOffset += 4;
        }

        // UV
        vertexData[offset + nextOffset + 0] = this->uvCoordinates[i].x;
        vertexData[offset + nextOffset + 1] = this->uvCoordinates[i].y;
    }

    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MeshModifyComponent] Vertex data allocated: " + Ogre::StringConverter::toString(vertexDataSize) + " bytes");

    // Check max index
    try
    {
        this->dynamicVertexBuffer = vaoManager->createVertexBuffer(vertexElements, this->vertexCount, Ogre::BT_DYNAMIC_DEFAULT, // Dynamic for updates!
                                                                   vertexData, false);
    }
    catch (Ogre::Exception& e)
    {
        // If exception, we must free the memory ourselves
        OGRE_FREE_SIMD(vertexData, Ogre::MEMCATEGORY_GEOMETRY);
        throw e;
    }

    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
                                                    "[MeshModifyComponent] Created vertex buffer: " + Ogre::StringConverter::toString(this->dynamicVertexBuffer->getBytesPerElement()) + " bytes per element");

    // Create index buffer
    const size_t indexDataSize = this->indexCount * sizeof(Ogre::uint16);
    Ogre::uint16* indexData = reinterpret_cast<Ogre::uint16*>(OGRE_MALLOC_SIMD(indexDataSize, Ogre::MEMCATEGORY_GEOMETRY));

    // Convert to 16-bit indices
    for (size_t i = 0; i < this->indexCount; ++i)
    {
        indexData[i] = static_cast<Ogre::uint16>(this->indices[i]);
    }

    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MeshModifyComponent] Index data allocated: " + Ogre::StringConverter::toString(indexDataSize) + " bytes, " +
                                                                           Ogre::StringConverter::toString(this->indexCount) + " indices");

    // Create index buffer - Ogre takes ownership of indexData when keepAsShadow=true
    try
    {
        this->dynamicIndexBuffer = vaoManager->createIndexBuffer(Ogre::IndexBufferPacked::IT_16BIT, this->indexCount, Ogre::BT_DEFAULT, indexData, true);
    }
    catch (Ogre::Exception& e)
    {
        // If exception, we must free the memory ourselves
        OGRE_FREE_SIMD(indexData, Ogre::MEMCATEGORY_GEOMETRY);
        throw e;
    }

    // Create VAO
    Ogre::VertexBufferPackedVec vertexBuffers;
    vertexBuffers.push_back(this->dynamicVertexBuffer);

    Ogre::VertexArrayObject* vao = vaoManager->createVertexArrayObject(vertexBuffers, this->dynamicIndexBuffer, Ogre::OT_TRIANGLE_LIST);

    // Assign to submesh
    subMesh->mVao[Ogre::VpNormal].push_back(vao);
    subMesh->mVao[Ogre::VpShadow].push_back(vao);

    // Calculate and set bounds
    Ogre::Vector3 minBounds(std::numeric_limits<float>::max());
    Ogre::Vector3 maxBounds(std::numeric_limits<float>::lowest());

    for (const auto& vertex : this->vertices)
    {
        minBounds.makeFloor(vertex);
        maxBounds.makeCeil(vertex);
    }

    Ogre::Aabb bounds;
    bounds.setExtents(minBounds, maxBounds);
    this->editableMesh->_setBounds(bounds, false);
    this->editableMesh->_setBoundingSphereRadius(bounds.getRadius());

    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MeshModifyComponent] Mesh bounds set: center(" + Ogre::StringConverter::toString(bounds.mCenter) + "), halfSize(" +
                                                                           Ogre::StringConverter::toString(bounds.mHalfSize) + ")");

    // Create the item using the editable mesh
    this->editableItem = this->gameObjectPtr->getSceneManager()->createItem(this->editableMesh, this->gameObjectPtr->isDynamic() ? Ogre::SCENE_DYNAMIC : Ogre::SCENE_STATIC);

    // Copy datablock/material from original item (first subitem)
    if (originalItem && originalItem->getNumSubItems() > 0)
    {
        Ogre::HlmsDatablock* datablock = originalItem->getSubItem(0)->getDatablock();
        if (datablock)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MeshModifyComponent] Applying datablock: " + datablock->getName().getFriendlyText());
            this->editableItem->getSubItem(0)->setDatablock(datablock);
        }
    }

    // Attach to scene node
    this->gameObjectPtr->getSceneNode()->attachObject(this->editableItem);

    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MeshModifyComponent] Created dynamic buffers successfully" + Ogre::String(includeTangents ? " (with tangents)" : " (no tangents)"));

    return true;
}

void MeshModifyComponent::uploadVertexData(void)
{
    if (nullptr == this->dynamicVertexBuffer)
    {
        return;
    }

    // Map the buffer for writing
    size_t floatsPerVertex = 8; // pos(3) + normal(3) + uv(2)
    if (this->vertexFormat.hasTangent)
    {
        floatsPerVertex = 12; // pos(3) + normal(3) + tangent(4) + uv(2)
    }
    // IMPORTANT: Never read from this pointer!
    const size_t dataSize = this->vertexCount * floatsPerVertex * sizeof(float);
    float* vertexData = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(dataSize, Ogre::MEMCATEGORY_GEOMETRY));

    // Write interleaved vertex data
    for (size_t i = 0; i < this->vertexCount; ++i)
    {
        size_t offset = i * floatsPerVertex;

        // Position
        vertexData[offset + 0] = this->vertices[i].x;
        vertexData[offset + 1] = this->vertices[i].y;
        vertexData[offset + 2] = this->vertices[i].z;

        // Normal
        vertexData[offset + 3] = this->normals[i].x;
        vertexData[offset + 4] = this->normals[i].y;
        vertexData[offset + 5] = this->normals[i].z;

        size_t nextOffset = 6;

        // Tangent (if present)
        if (this->vertexFormat.hasTangent)
        {
            vertexData[offset + nextOffset + 0] = this->tangents[i].x;
            vertexData[offset + nextOffset + 1] = this->tangents[i].y;
            vertexData[offset + nextOffset + 2] = this->tangents[i].z;
            vertexData[offset + nextOffset + 3] = this->tangents[i].w;
            nextOffset += 4;
        }

        // UV (unchanged)
        vertexData[offset + nextOffset + 0] = this->uvCoordinates[i].x;
        vertexData[offset + nextOffset + 1] = this->uvCoordinates[i].y;
    }

    // Unmap but keep persistent mapping
    this->dynamicVertexBuffer->upload(vertexData, 0, this->vertexCount);

    // Free the temporary buffer
    OGRE_FREE_SIMD(vertexData, Ogre::MEMCATEGORY_GEOMETRY);
}

Ogre::Real MeshModifyComponent::calculateBrushInfluence(Ogre::Real distance, Ogre::Real brushRadius) const
{
    if (distance >= brushRadius)
    {
        return 0.0f;
    }

    // Normalized distance [0, 1]
    Ogre::Real normalizedDist = distance / brushRadius;

    // Apply falloff curve: (1 - d)^falloff
    Ogre::Real falloff = this->brushFalloff->getReal();
    Ogre::Real influence = std::pow(1.0f - normalizedDist, falloff);

    // If using brush image, sample it
    if (!this->brushData.empty() && this->brushImageWidth > 0 && this->brushImageHeight > 0)
    {
        // Map normalized distance to brush texture (radial sampling)
        size_t centerX = this->brushImageWidth / 2;
        size_t centerY = this->brushImageHeight / 2;

        // Sample at distance from center
        size_t sampleX = static_cast<size_t>(centerX + normalizedDist * centerX);
        size_t sampleY = centerY; // Sample horizontally for radial brush

        sampleX = std::min(sampleX, this->brushImageWidth - 1);
        sampleY = std::min(sampleY, this->brushImageHeight - 1);

        size_t brushIndex = sampleY * this->brushImageWidth + sampleX;
        if (brushIndex < this->brushData.size())
        {
            influence *= this->brushData[brushIndex];
        }
    }

    return influence * this->brushIntensity->getReal();
}

void MeshModifyComponent::applyBrush(const Ogre::Vector3& brushCenterLocal, bool invertEffect)
{
    Ogre::Real brushRadius = this->brushSize->getReal();
    BrushMode mode = this->getBrushMode();

    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MeshModifyComponent] applyBrush called at: " + Ogre::StringConverter::toString(brushCenterLocal) +
                                                                           ", brushRadius: " + Ogre::StringConverter::toString(brushRadius) + ", mode: " + Ogre::StringConverter::toString(static_cast<int>(mode)));

    // Invert push/pull if requested
    if (invertEffect)
    {
        if (mode == BrushMode::PUSH)
        {
            mode = BrushMode::PULL;
        }
        else if (mode == BrushMode::PULL)
        {
            mode = BrushMode::PUSH;
        }
    }

    // For smooth and flatten, we need to calculate average position/plane
    Ogre::Vector3 averagePosition = Ogre::Vector3::ZERO;
    Ogre::Vector3 averageNormal = Ogre::Vector3::ZERO;
    size_t affectedCount = 0;

    // First pass: find affected vertices and calculate averages
    std::vector<std::pair<size_t, Ogre::Real>> affectedVertices;

    for (size_t i = 0; i < this->vertexCount; ++i)
    {
        Ogre::Real distance = this->vertices[i].distance(brushCenterLocal);

        if (distance < brushRadius)
        {
            Ogre::Real influence = this->calculateBrushInfluence(distance, brushRadius);
            affectedVertices.push_back({i, influence});

            averagePosition += this->vertices[i];
            averageNormal += this->normals[i];
            ++affectedCount;
        }
    }

    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MeshModifyComponent] Affected vertices: " + Ogre::StringConverter::toString(affectedCount));

    if (affectedCount == 0)
    {
        return;
    }

    averagePosition /= static_cast<Ogre::Real>(affectedCount);
    averageNormal.normalise();

    // Second pass: apply brush effect
    for (const auto& affected : affectedVertices)
    {
        size_t idx = affected.first;
        Ogre::Real influence = affected.second;

        switch (mode)
        {
        case BrushMode::PUSH:
        {
            // Push along vertex normal
            this->vertices[idx] += this->normals[idx] * influence;
            break;
        }

        case BrushMode::PULL:
        {
            // Pull along vertex normal (inward)
            this->vertices[idx] -= this->normals[idx] * influence;
            break;
        }

        case BrushMode::SMOOTH:
        {
            // Average with neighbors
            if (idx < this->vertexNeighbors.size() && !this->vertexNeighbors[idx].empty())
            {
                Ogre::Vector3 neighborAvg = Ogre::Vector3::ZERO;
                for (size_t neighborIdx : this->vertexNeighbors[idx])
                {
                    neighborAvg += this->vertices[neighborIdx];
                }
                neighborAvg /= static_cast<Ogre::Real>(this->vertexNeighbors[idx].size());

                // Blend toward neighbor average
                this->vertices[idx] = this->vertices[idx] * (1.0f - influence) + neighborAvg * influence;
            }
            break;
        }

        case BrushMode::FLATTEN:
        {
            // Project onto average plane
            Ogre::Real distToPlane = averageNormal.dotProduct(this->vertices[idx] - averagePosition);
            this->vertices[idx] -= averageNormal * distToPlane * influence;
            break;
        }

        case BrushMode::PINCH:
        {
            // Pull toward brush center
            Ogre::Vector3 toCenter = brushCenterLocal - this->vertices[idx];
            toCenter.normalise();
            this->vertices[idx] += toCenter * influence;
            break;
        }

        case BrushMode::INFLATE:
        {
            // Push along own normal (scaled by distance from average)
            this->vertices[idx] += this->normals[idx] * influence;
            break;
        }
        }
    }

    // Recalculate normals for affected area
    this->recalculateNormals();

    // Recalculate tangents if the mesh has them
    this->recalculateTangents();

    GraphicsModule::RenderCommand renderCommand = [this, brushCenterLocal, invertEffect]()
    {
        // Upload to GPU
        this->uploadVertexData();
    };
    NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "MeshModifyComponent::applyBrush");
}

void MeshModifyComponent::recalculateNormals(void)
{
    // Reset normals
    for (auto& normal : this->normals)
    {
        normal = Ogre::Vector3::ZERO;
    }

    // Accumulate face normals
    for (size_t i = 0; i < this->indexCount; i += 3)
    {
        size_t i0 = this->indices[i];
        size_t i1 = this->indices[i + 1];
        size_t i2 = this->indices[i + 2];

        const Ogre::Vector3& v0 = this->vertices[i0];
        const Ogre::Vector3& v1 = this->vertices[i1];
        const Ogre::Vector3& v2 = this->vertices[i2];

        Ogre::Vector3 edge1 = v1 - v0;
        Ogre::Vector3 edge2 = v2 - v0;
        Ogre::Vector3 faceNormal = edge1.crossProduct(edge2);

        // Accumulate (area-weighted by not normalizing face normal)
        this->normals[i0] += faceNormal;
        this->normals[i1] += faceNormal;
        this->normals[i2] += faceNormal;
    }

    // Normalize all normals
    for (auto& normal : this->normals)
    {
        normal.normalise();
    }
}

void MeshModifyComponent::recalculateTangents(void)
{
    if (false == this->vertexFormat.hasTangent)
    {
        return;
    }

    // Reset tangents
    std::vector<Ogre::Vector3> tan1(this->vertexCount, Ogre::Vector3::ZERO);
    std::vector<Ogre::Vector3> tan2(this->vertexCount, Ogre::Vector3::ZERO);

    // Calculate tangents per triangle
    for (size_t i = 0; i < this->indexCount; i += 3)
    {
        size_t i0 = this->indices[i];
        size_t i1 = this->indices[i + 1];
        size_t i2 = this->indices[i + 2];

        const Ogre::Vector3& v0 = this->vertices[i0];
        const Ogre::Vector3& v1 = this->vertices[i1];
        const Ogre::Vector3& v2 = this->vertices[i2];

        const Ogre::Vector2& uv0 = this->uvCoordinates[i0];
        const Ogre::Vector2& uv1 = this->uvCoordinates[i1];
        const Ogre::Vector2& uv2 = this->uvCoordinates[i2];

        Ogre::Vector3 edge1 = v1 - v0;
        Ogre::Vector3 edge2 = v2 - v0;

        Ogre::Real deltaU1 = uv1.x - uv0.x;
        Ogre::Real deltaV1 = uv1.y - uv0.y;
        Ogre::Real deltaU2 = uv2.x - uv0.x;
        Ogre::Real deltaV2 = uv2.y - uv0.y;

        Ogre::Real denom = deltaU1 * deltaV2 - deltaU2 * deltaV1;
        if (std::abs(denom) < 1e-6f)
        {
            denom = 1e-6f;
        }

        Ogre::Real r = 1.0f / denom;

        Ogre::Vector3 sdir((deltaV2 * edge1.x - deltaV1 * edge2.x) * r, (deltaV2 * edge1.y - deltaV1 * edge2.y) * r, (deltaV2 * edge1.z - deltaV1 * edge2.z) * r);

        Ogre::Vector3 tdir((deltaU1 * edge2.x - deltaU2 * edge1.x) * r, (deltaU1 * edge2.y - deltaU2 * edge1.y) * r, (deltaU1 * edge2.z - deltaU2 * edge1.z) * r);

        tan1[i0] += sdir;
        tan1[i1] += sdir;
        tan1[i2] += sdir;

        tan2[i0] += tdir;
        tan2[i1] += tdir;
        tan2[i2] += tdir;
    }

    // Orthonormalize and calculate handedness
    for (size_t i = 0; i < this->vertexCount; ++i)
    {
        const Ogre::Vector3& n = this->normals[i];
        const Ogre::Vector3& t = tan1[i];

        // Gram-Schmidt orthogonalize
        Ogre::Vector3 tangent = (t - n * n.dotProduct(t));
        tangent.normalise();

        // Calculate handedness
        Ogre::Real handedness = (n.crossProduct(t).dotProduct(tan2[i]) < 0.0f) ? -1.0f : 1.0f;

        this->tangents[i] = Ogre::Vector4(tangent.x, tangent.y, tangent.z, handedness);
    }
}

void MeshModifyComponent::buildVertexAdjacency(void)
{
    this->vertexNeighbors.clear();
    this->vertexNeighbors.resize(this->vertexCount);

    // Build adjacency from triangles
    for (size_t i = 0; i < this->indexCount; i += 3)
    {
        size_t i0 = this->indices[i];
        size_t i1 = this->indices[i + 1];
        size_t i2 = this->indices[i + 2];

        // Add each vertex as neighbor of the others
        auto addNeighbor = [this](size_t vertex, size_t neighbor)
        {
            auto& neighbors = this->vertexNeighbors[vertex];
            if (std::find(neighbors.begin(), neighbors.end(), neighbor) == neighbors.end())
            {
                neighbors.push_back(neighbor);
            }
        };

        addNeighbor(i0, i1);
        addNeighbor(i0, i2);
        addNeighbor(i1, i0);
        addNeighbor(i1, i2);
        addNeighbor(i2, i0);
        addNeighbor(i2, i1);
    }
}

bool MeshModifyComponent::raycastMesh(Ogre::Real screenX, Ogre::Real screenY, Ogre::Vector3& hitPosition, Ogre::Vector3& hitNormal)
{
    Ogre::Camera* camera = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
    if (nullptr == camera)
    {
        return false;
    }

    Ogre::Ray ray = camera->getCameraToViewportRay(screenX, screenY);

    // Transform ray to local space
    Ogre::Matrix4 worldMatrix = this->gameObjectPtr->getSceneNode()->_getFullTransform();
    Ogre::Matrix4 invWorldMatrix = worldMatrix.inverse();

    Ogre::Vector3 localRayOrigin = invWorldMatrix * ray.getOrigin();
    Ogre::Matrix3 invWorld3x3;
    invWorldMatrix.extract3x3Matrix(invWorld3x3);
    Ogre::Vector3 localRayDir = invWorld3x3 * ray.getDirection();
    localRayDir.normalise();

    Ogre::Ray localRay(localRayOrigin, localRayDir);

    // Test against all triangles
    Ogre::Real closestDistance = std::numeric_limits<Ogre::Real>::max();
    bool hit = false;

    for (size_t i = 0; i < this->indexCount; i += 3)
    {
        const Ogre::Vector3& v0 = this->vertices[this->indices[i]];
        const Ogre::Vector3& v1 = this->vertices[this->indices[i + 1]];
        const Ogre::Vector3& v2 = this->vertices[this->indices[i + 2]];

        std::pair<bool, Ogre::Real> result = Ogre::Math::intersects(localRay, v0, v1, v2, true, false);

        if (result.first && result.second < closestDistance)
        {
            closestDistance = result.second;
            hitPosition = localRay.getPoint(closestDistance);

            // Calculate face normal
            Ogre::Vector3 edge1 = v1 - v0;
            Ogre::Vector3 edge2 = v2 - v0;
            hitNormal = edge1.crossProduct(edge2);
            hitNormal.normalise();

            hit = true;
        }
    }

    if (true == hit)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MeshModifyComponent] Raycast HIT at local pos: " + Ogre::StringConverter::toString(hitPosition));
    }

    return hit;
}

Ogre::Vector3 MeshModifyComponent::worldToLocal(const Ogre::Vector3& worldPos) const
{
    Ogre::Matrix4 invWorldMatrix = this->gameObjectPtr->getSceneNode()->_getFullTransform().inverse();
    return invWorldMatrix * worldPos;
}

Ogre::Vector3 MeshModifyComponent::localToWorld(const Ogre::Vector3& localPos) const
{
    return this->gameObjectPtr->getSceneNode()->_getFullTransform() * localPos;
}

void MeshModifyComponent::handleMeshModifyMode(NOWA::EventDataPtr eventData)
{
    boost::shared_ptr<NOWA::EventDataEditorMode> castEventData = boost::static_pointer_cast<NOWA::EventDataEditorMode>(eventData);

    if (NOWA::EditorManager::EDITOR_MESH_MODIFY_MODE == castEventData->getManipulationMode())
    {
        this->canModify = true;
    }
    else
    {
        this->canModify = false;
    }
}

void MeshModifyComponent::handleGameObjectSelected(NOWA::EventDataPtr eventData)
{
    boost::shared_ptr<NOWA::EventDataGameObjectSelected> castEventData = boost::static_pointer_cast<NOWA::EventDataGameObjectSelected>(eventData);

    if (nullptr != this->gameObjectPtr)
    {
        if (castEventData->getGameObjectId() == this->gameObjectPtr->getId())
        {
            if (true == this->canModify && true == castEventData->getIsSelected())
            {
                this->addInputListener();
            }
            else
            {
                this->removeInputListener();
            }
        }
    }
}

void MeshModifyComponent::addInputListener(void)
{
    const Ogre::String listenerName = MeshModifyComponent::getStaticClassName() + "_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
    NOWA::GraphicsModule::RenderCommand renderCommand = [this, listenerName]()
    {
        if (auto* core = InputDeviceCore::getSingletonPtr())
        {
            if (auto* core = InputDeviceCore::getSingletonPtr())
            {
                core->addMouseListener(this, listenerName);
            }
        }
    };
    NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MeshModifyComponent::addInputListener");
}

void MeshModifyComponent::removeInputListener(void)
{
    const Ogre::String listenerName = MeshModifyComponent::getStaticClassName() + "_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
    NOWA::GraphicsModule::RenderCommand renderCommand = [this, listenerName]()
    {
        if (auto* core = InputDeviceCore::getSingletonPtr())
        {
            core->removeMouseListener(listenerName);
        }
    };
    NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MeshModifyComponent::removeInputListener");
}

// ==================== MOUSE HANDLERS ====================

bool MeshModifyComponent::mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
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

    if (nullptr == this->editableItem)
    {
        return false;
    }

    // Get normalized screen coordinates
    Ogre::Real screenX = 0.0f;
    Ogre::Real screenY = 0.0f;
    MathHelper::getInstance()->mouseToViewPort(evt.state.X.abs, evt.state.Y.abs, screenX, screenY, Core::getSingletonPtr()->getOgreRenderWindow());

    // Raycast to find brush position
    Ogre::Vector3 hitPosition, hitNormal;
    if (this->raycastMesh(screenX, screenY, hitPosition, hitNormal))
    {
        this->isPressing = true;
        this->lastBrushPosition = hitPosition;

        // Apply brush
        this->applyBrush(hitPosition, this->isCtrlPressed);
    }

    return false;
}

bool MeshModifyComponent::mouseMoved(const OIS::MouseEvent& evt)
{
    if (false == this->activated->getBool())
    {
        return false;
    }

    if (false == this->isPressing)
    {
        return false;
    }

    if (nullptr == this->editableItem)
    {
        return false;
    }

    // Get normalized screen coordinates
    Ogre::Real screenX = 0.0f;
    Ogre::Real screenY = 0.0f;
    MathHelper::getInstance()->mouseToViewPort(evt.state.X.abs, evt.state.Y.abs, screenX, screenY, Core::getSingletonPtr()->getOgreRenderWindow());

    // Raycast to find brush position
    Ogre::Vector3 hitPosition, hitNormal;
    if (this->raycastMesh(screenX, screenY, hitPosition, hitNormal))
    {
        // Only apply if moved enough distance
        Ogre::Real minDistance = this->brushSize->getReal() * 0.1f;
        Ogre::Real currentDistance = hitPosition.distance(this->lastBrushPosition);
        if (currentDistance > minDistance)
        {
            this->lastBrushPosition = hitPosition;
            this->applyBrush(hitPosition, this->isCtrlPressed);
        }
    }

    return false;
}

bool MeshModifyComponent::mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
{
    if (id == OIS::MB_Left)
    {
        this->isPressing = false;

        if (nullptr != this->dynamicVertexBuffer)
        {
            boost::shared_ptr<NOWA::EventDataGeometryModified> eventDataGeometryModified(new NOWA::EventDataGeometryModified());
            NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataGeometryModified);
        }
    }

    return false;
}

// ==================== STATIC METHODS ====================

bool MeshModifyComponent::canStaticAddComponent(GameObject* gameObject)
{
    // Can only add to GameObjects with an Item that has exactly one SubItem
    Ogre::Item* item = gameObject->getMovableObjectUnsafe<Ogre::Item>();
    if (item && item->getNumSubItems() == 1)
    {
        // Check that the component isn't already added
        if (gameObject->getComponentCount<MeshModifyComponent>() == 0)
        {
            return true;
        }
    }
    return false;
}

// ==================== LUA API ====================

MeshModifyComponent* getMeshModifyComponent(GameObject* gameObject, unsigned int occurrenceIndex)
{
    return makeStrongPtr<MeshModifyComponent>(gameObject->getComponentWithOccurrence<MeshModifyComponent>(occurrenceIndex)).get();
}

MeshModifyComponent* getMeshModifyComponent(GameObject* gameObject)
{
    return makeStrongPtr<MeshModifyComponent>(gameObject->getComponent<MeshModifyComponent>()).get();
}

MeshModifyComponent* getMeshModifyComponentFromName(GameObject* gameObject, const Ogre::String& name)
{
    return makeStrongPtr<MeshModifyComponent>(gameObject->getComponentFromName<MeshModifyComponent>(name)).get();
}

void MeshModifyComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
{
#if 0
		module(lua)
			[
				class_<MeshModifyComponent, GameObjectComponent>("MeshModifyComponent")
					.def("setActivated", &MeshModifyComponent::setActivated)
					.def("isActivated", &MeshModifyComponent::isActivated)
					.def("setBrushSize", &MeshModifyComponent::setBrushSize)
					.def("getBrushSize", &MeshModifyComponent::getBrushSize)
					.def("setBrushIntensity", &MeshModifyComponent::setBrushIntensity)
					.def("getBrushIntensity", &MeshModifyComponent::getBrushIntensity)
					.def("setBrushFalloff", &MeshModifyComponent::setBrushFalloff)
					.def("getBrushFalloff", &MeshModifyComponent::getBrushFalloff)
					.def("setBrushMode", &MeshModifyComponent::setBrushMode)
					.def("getBrushMode", &MeshModifyComponent::getBrushModeString)
					.def("resetMesh", &MeshModifyComponent::resetMesh)
					.def("exportMesh", &MeshModifyComponent::exportMesh)
					.def("getVertexCount", &MeshModifyComponent::getVertexCount)
					.def("getIndexCount", &MeshModifyComponent::getIndexCount)
			];

		LuaScriptApi::getInstance()->addClassToCollection("MeshModifyComponent", "class inherits GameObjectComponent", MeshModifyComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("MeshModifyComponent", "void setActivated(bool activated)", "Sets whether this component should be activated or not.");
		LuaScriptApi::getInstance()->addClassToCollection("MeshModifyComponent", "bool isActivated()", "Gets whether this component is activated.");
		LuaScriptApi::getInstance()->addClassToCollection("MeshModifyComponent", "void setBrushSize(float size)", "Sets the brush radius in world units.");
		LuaScriptApi::getInstance()->addClassToCollection("MeshModifyComponent", "float getBrushSize()", "Gets the brush radius.");
		LuaScriptApi::getInstance()->addClassToCollection("MeshModifyComponent", "void setBrushIntensity(float intensity)", "Sets the brush intensity (0.0 - 1.0).");
		LuaScriptApi::getInstance()->addClassToCollection("MeshModifyComponent", "float getBrushIntensity()", "Gets the brush intensity.");
		LuaScriptApi::getInstance()->addClassToCollection("MeshModifyComponent", "void setBrushFalloff(float falloff)", "Sets the brush falloff exponent.");
		LuaScriptApi::getInstance()->addClassToCollection("MeshModifyComponent", "float getBrushFalloff()", "Gets the brush falloff.");
		LuaScriptApi::getInstance()->addClassToCollection("MeshModifyComponent", "void setBrushMode(String mode)", "Sets brush mode: 'Push', 'Pull', 'Smooth', 'Flatten', 'Pinch', 'Inflate'.");
		LuaScriptApi::getInstance()->addClassToCollection("MeshModifyComponent", "String getBrushMode()", "Gets the current brush mode name.");
		LuaScriptApi::getInstance()->addClassToCollection("MeshModifyComponent", "void resetMesh()", "Resets the mesh to its original state.");
		LuaScriptApi::getInstance()->addClassToCollection("MeshModifyComponent", "bool exportMesh(String fileName)", "Exports the modified mesh to a file.");
		LuaScriptApi::getInstance()->addClassToCollection("MeshModifyComponent", "int getVertexCount()", "Gets the number of vertices in the mesh.");
		LuaScriptApi::getInstance()->addClassToCollection("MeshModifyComponent", "int getIndexCount()", "Gets the number of indices in the mesh.");

		gameObjectClass.def("getMeshModifyComponentFromName", &getMeshModifyComponentFromName);
		gameObjectClass.def("getMeshModifyComponent", (MeshModifyComponent * (*)(GameObject*)) & getMeshModifyComponent);
		gameObjectClass.def("getMeshModifyComponentFromIndex", (MeshModifyComponent * (*)(GameObject*, unsigned int)) & getMeshModifyComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "MeshModifyComponent getMeshModifyComponent()", "Gets the component.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "MeshModifyComponent getMeshModifyComponentFromIndex(unsigned int occurrenceIndex)", "Gets the component by occurrence index.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "MeshModifyComponent getMeshModifyComponentFromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castMeshModifyComponent", &GameObjectController::cast<MeshModifyComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "MeshModifyComponent castMeshModifyComponent(MeshModifyComponent other)", "Casts an incoming type from function for lua auto completion.");
#endif
}

}; // namespace NOWA
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

    MeshModifyComponent::MeshModifyComponent() :
        GameObjectComponent(),
        name("MeshModifyComponent"),
        editableItem(nullptr),
        originalItem(nullptr),
        dynamicVertexBuffer(nullptr),
        dynamicIndexBuffer(nullptr),
        isIndices32(false),
        vertexCount(0),
        indexCount(0),
        bytesPerVertex(0),
        brushImageWidth(0),
        brushImageHeight(0),
        raySceneQuery(nullptr),
        isPressing(false),
        isCtrlPressed(false),
        lastBrushPosition(Ogre::Vector3::ZERO),
        pendingUpload(false),
        canModify(false),
        physicsActiveComponent(nullptr),
        currentDamage(0.0f),
        activated(new Variant(MeshModifyComponent::AttrActivated(), true, this->attributes)),
        brushName(new Variant(MeshModifyComponent::AttrBrushName(), this->attributes)),
        brushSize(new Variant(MeshModifyComponent::AttrBrushSize(), 1.0f, this->attributes)),
        brushIntensity(new Variant(MeshModifyComponent::AttrBrushIntensity(), 0.1f, this->attributes)),
        brushFalloff(new Variant(MeshModifyComponent::AttrBrushFalloff(), 2.0f, this->attributes)),
        brushMode(new Variant(MeshModifyComponent::AttrBrushMode(), Ogre::String("Push"), this->attributes)),
        category(new Variant(MeshModifyComponent::AttrCategory(), Ogre::String(), this->attributes)),
        maxDamage(new NOWA::Variant(MeshModifyComponent::AttrMaxDamage(), 1.0f, this->attributes))
    {
        this->maxDamage->setDescription("Maximum damage as a factor (0.0 - 1.0). 1.0 = fully deformable like a scrap press, 0.7 = 70% max damage.");
        this->maxDamage->setConstraints(0.0f, 1.0f);
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
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == MeshModifyComponent::AttrMaxDamage())
        {
            this->maxDamage->setValue(XMLConverter::getAttrib(propertyElement, "data"));
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
        clonedCompPtr->setMaxDamage(this->maxDamage->getReal());

        clonedGameObjectPtr->addComponent(clonedCompPtr);
        clonedCompPtr->setOwner(clonedGameObjectPtr);

        GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));

        return clonedCompPtr;
    }

    bool MeshModifyComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MeshModifyComponent] Init component for game object: " + this->gameObjectPtr->getName());

        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &MeshModifyComponent::handleMeshModifyMode), NOWA::EventDataEditorMode::getStaticEventType());
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &MeshModifyComponent::handleGameObjectSelected), NOWA::EventDataGameObjectSelected::getStaticEventType());

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

        // Get PhysicsActiveComponent if exists
        const auto& physicsActiveCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsActiveComponent>());
        if (physicsActiveCompPtr)
        {
            this->physicsActiveComponent = physicsActiveCompPtr.get();
        }

        // Slow part: extract mesh data and create editable item — but DON'T swap yet.
        // originalItem stays as movableObject so serializer always sees Viper.mesh.
        if (false == this->prepareEditableMesh())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MeshModifyComponent] Failed to prepare editable mesh for: " + this->gameObjectPtr->getName());
            return false;
        }

        return true;
    }

    bool MeshModifyComponent::connect(void)
    {
        this->addInputListener();

        this->currentDamage = 0.0f;

        if (this->editableItem == nullptr)
        {
            if (false == this->prepareEditableMesh())
            {
                return false;
            }
        }

        // Swap visual to editable
        Ogre::MovableObject* old = nullptr;
        this->gameObjectPtr->swapMovableObject(this->editableItem, old);
        this->originalItem = static_cast<Ogre::Item*>(old);

        this->physicsActiveComponent->reCreateDynamicBodyForItem(this->editableItem);

        return true;
    }

    bool MeshModifyComponent::disconnect(void)
    {
        this->removeInputListener();

        this->currentDamage = 0.0f;

        if (this->originalItem != nullptr)
        {
            // Swap original back
            Ogre::MovableObject* old = nullptr;
            this->gameObjectPtr->swapMovableObject(this->originalItem, old);
            // old == editableItem, keep alive for next connect

            // Rebuild physics from original BEFORE nulling the pointer
            this->physicsActiveComponent->reCreateDynamicBodyForItem(this->originalItem);
            this->originalItem = nullptr;
        }

        // Reset CPU data
        if (!this->originalVertices.empty())
        {
            this->vertices = this->originalVertices;
            this->normals = this->originalNormals;
            this->tangents = this->originalTangents;
            GraphicsModule::RenderCommand cmd = [this]()
            {
                this->uploadVertexData();
            };
            NOWA::GraphicsModule::getInstance()->enqueue(std::move(cmd), "MeshModifyComponent::disconnect::reset");
        }

        return true;
    }

    void MeshModifyComponent::rebuildCollision(void)
    {
        if (this->physicsActiveComponent && this->editableItem)
        {
            this->physicsActiveComponent->reCreateCollision(this->editableItem);
        }
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
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &MeshModifyComponent::handleGameObjectSelected), NOWA::EventDataGameObjectSelected::getStaticEventType());

        this->removeInputListener();

        this->physicsActiveComponent = nullptr;

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
        if (nullptr != this->physicsActiveComponent && index == this->physicsActiveComponent->getIndex())
        {
            this->physicsActiveComponent = nullptr;
        }
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
        else if (MeshModifyComponent::AttrMaxDamage() == attribute->getName())
        {
            this->setMaxDamage(attribute->getReal());
        }
    }

    void MeshModifyComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
    {
        GameObjectComponent::writeXML(propertiesXML, doc);

        xml_node<>* propertyXML = nullptr;

        // Activated
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, MeshModifyComponent::AttrActivated())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
        propertiesXML->append_node(propertyXML);

        // Brush Name
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, MeshModifyComponent::AttrBrushName())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->brushName->getListSelectedValue())));
        propertiesXML->append_node(propertyXML);

        // Brush Size
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, MeshModifyComponent::AttrBrushSize())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->brushSize->getReal())));
        propertiesXML->append_node(propertyXML);

        // Brush Intensity
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, MeshModifyComponent::AttrBrushIntensity())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->brushIntensity->getReal())));
        propertiesXML->append_node(propertyXML);

        // Brush Falloff
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, MeshModifyComponent::AttrBrushFalloff())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->brushFalloff->getReal())));
        propertiesXML->append_node(propertyXML);

        // Brush Mode
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, MeshModifyComponent::AttrBrushMode())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->brushMode->getListSelectedValue())));
        propertiesXML->append_node(propertyXML);

        // Category
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, MeshModifyComponent::AttrCategory())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->category->getString())));
        propertiesXML->append_node(propertyXML);

        // Max Damage
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, MeshModifyComponent::AttrMaxDamage())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxDamage->getReal())));
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

    void MeshModifyComponent::setMaxDamage(Ogre::Real maxDamage)
    {
        this->maxDamage->setValue(Ogre::Math::Clamp(maxDamage, 0.0f, 1.0f));
    }

    Ogre::Real MeshModifyComponent::getMaxDamage() const
    {
        return this->maxDamage->getReal();
    }

    Ogre::Real MeshModifyComponent::getCurrentDamage() const
    {
        return this->currentDamage;
    }

    void MeshModifyComponent::resetDamage(void)
    {
        this->currentDamage = 0.0f;
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

    void MeshModifyComponent::deformAtWorldPosition(const Ogre::Vector3& worldPos, bool invertEffect)
    {
        if (nullptr == this->editableItem)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MeshModifyComponent] deformAtWorldPosition: editableItem is null, skipping.");
            return;
        }

        // Convert world position to local/object space
        Ogre::Vector3 localPos = this->worldToLocal(worldPos);

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MeshModifyComponent] deformAtWorldPosition: world=" + Ogre::StringConverter::toString(worldPos) + " -> local=" + Ogre::StringConverter::toString(localPos));

        this->applyBrush(localPos, invertEffect);
    }

    void MeshModifyComponent::deformAtWorldPositionByForce(const Ogre::Vector3& worldPos, Ogre::Real impactStrength, Ogre::Real maxForceForFullDeform, bool invertEffect)
    {
        if (nullptr == this->editableItem)
        {
            return;
        }
        if (maxForceForFullDeform <= 0.0f)
        {
            maxForceForFullDeform = 1000.0f;
        }
        // Compute a [0,1] scale factor from impact strength
        Ogre::Real intensityScale = Ogre::Math::Clamp(impactStrength / maxForceForFullDeform, 0.0f, 1.0f);
        if (intensityScale < 0.01f)
        {
            // Impact too weak to matter
            return;
        }
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MeshModifyComponent] deformAtWorldPositionByForce: force=" + Ogre::StringConverter::toString(impactStrength) + ", scale=" + Ogre::StringConverter::toString(intensityScale) +
                                                                               ", effectiveIntensity=" + Ogre::StringConverter::toString(intensityScale));
        // Use intensityScale directly as intensity — do NOT multiply by brushIntensity,
        // that double-scales and makes deformation invisible for typical impact values
        Ogre::Real savedIntensity = this->brushIntensity->getReal();
        this->brushIntensity->setValue(intensityScale);
        this->deformAtWorldPosition(worldPos, invertEffect);
        this->brushIntensity->setValue(savedIntensity);
    }

    void MeshModifyComponent::deformAtWorldPositionByForceAndNormal(const Ogre::Vector3& worldPos, const Ogre::Vector3& worldNormal, Ogre::Real impactStrength, Ogre::Real maxForceForFullDeform, bool invertEffect)
    {
        if (nullptr == this->editableItem)
        {
            return;
        }

        if (maxForceForFullDeform <= 0.0f)
        {
            maxForceForFullDeform = 1000.0f;
        }

        Ogre::Real intensityScale = Ogre::Math::Clamp(impactStrength / maxForceForFullDeform, 0.0f, 1.0f);
        if (intensityScale < 0.01f)
        {
            return;
        }

        Ogre::Vector3 localPos = this->worldToLocal(worldPos);

        // Transform the world normal into local space (no translation, only rotation+scale)
        Ogre::Matrix4 worldTransform = this->gameObjectPtr->getSceneNode()->_getFullTransform();
        Ogre::Matrix3 rotScale;
        worldTransform.extract3x3Matrix(rotScale);
        Ogre::Vector3 localNormal = rotScale.Inverse().Transpose() * worldNormal;
        localNormal.normalise();

        Ogre::Real savedIntensity = this->brushIntensity->getReal();
        this->brushIntensity->setValue(intensityScale);
        this->applyBrushAlongDirection(localPos, localNormal, invertEffect);
        this->brushIntensity->setValue(savedIntensity);
    }
    // ==================== PRIVATE METHODS ====================

    bool MeshModifyComponent::prepareEditableMesh(void)
    {
        Ogre::Item* originalItem = this->gameObjectPtr->getMovableObject<Ogre::Item>();
        if (nullptr == originalItem)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MeshModifyComponent] GameObject has no Item: " + this->gameObjectPtr->getName());
            return false;
        }

        Ogre::MeshPtr originalMesh = originalItem->getMesh();
        if (nullptr == originalMesh)
        {
            return false;
        }

        bool success = true;

        GraphicsModule::RenderCommand renderCommand = [this, originalItem, &success]()
        {
            if (false == this->extractMeshData())
            {
                success = false;
                return;
            }
            this->buildVertexAdjacency();
            if (false == this->createDynamicBuffers())
            {
                success = false;
                return;
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MeshModifyComponent::createEditableMesh");

        return success;
    }

    void MeshModifyComponent::destroyEditableMesh(void)
    {
        GraphicsModule::RenderCommand renderCommand = [this]()
        {
            // Step 1: restore original item — this detaches AND destroys editableItem (destroyCurrent=true)
            // gameObjectPtr->movableObject points to editableItem, restoreMovableObject
            // detaches it from scene node and destroys it cleanly before it goes dangling
            if (this->originalItem)
            {
                this->gameObjectPtr->restoreMovableObject(this->originalItem, /*destroyCurrent=*/true);
                this->originalItem = nullptr;
            }
            this->editableItem = nullptr; // already destroyed by restoreMovableObject above

            // Step 2: unmap and release buffer references
            // Item is gone so VAOs are no longer in flight
            for (SubMeshInfo& smInfo : this->subMeshInfoList)
            {
                if (smInfo.dynamicVertexBuffer && smInfo.dynamicVertexBuffer->getMappingState() != Ogre::MS_UNMAPPED)
                {
                    smInfo.dynamicVertexBuffer->unmap(Ogre::UO_UNMAP_ALL);
                }
                smInfo.dynamicVertexBuffer = nullptr;
                smInfo.dynamicIndexBuffer = nullptr;
            }
            this->subMeshInfoList.clear();
            this->dynamicVertexBuffer = nullptr;
            this->dynamicIndexBuffer = nullptr;

            // Step 3: remove mesh — item is already gone so no dangling VAO references
            if (this->editableMesh)
            {
                Ogre::MeshManager::getSingleton().remove(this->editableMesh);
                this->editableMesh.reset();
            }

            // Step 4: clear CPU data
            this->vertices.clear();
            this->normals.clear();
            this->tangents.clear();
            this->uvCoordinates.clear();
            this->indices.clear();
            this->originalVertices.clear();
            this->originalNormals.clear();
            this->originalTangents.clear();
            this->vertexNeighbors.clear();

            // Step 5: rebuild physics from original mesh
            if (this->physicsActiveComponent)
            {
                this->physicsActiveComponent->reCreateCollision(true);
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

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                "[MeshModifyComponent] Buffer " + Ogre::StringConverter::toString(bufIdx) + " has " + Ogre::StringConverter::toString(elements.size()) + " elements, " + Ogre::StringConverter::toString(vb->getBytesPerElement()) + " bytes per vertex");

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

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MeshModifyComponent] Final vertex format - Position: " + Ogre::StringConverter::toString(this->vertexFormat.hasPosition) +
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

        this->vertices.resize(this->vertexCount);
        this->normals.resize(this->vertexCount);
        this->tangents.resize(this->vertexCount, Ogre::Vector4(1, 0, 0, 1));
        this->uvCoordinates.resize(this->vertexCount);
        this->indices.resize(this->indexCount);

        // NEW: clear per-submesh tracking
        this->subMeshInfoList.clear();

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

            if (vao->getIndexBuffer())
            {
                this->isIndices32 = (vao->getIndexBuffer()->getIndexType() == Ogre::IndexBufferPacked::IT_32BIT);
            }

            // Detect format PER submesh (stores into this->vertexFormat)
            this->detectVertexFormat(vao);

            // NEW: record start positions for this submesh
            const size_t smVertexStart = subMeshOffset;
            const size_t smIndexStart = addedIndices;

            Ogre::VertexArrayObject::ReadRequestsVec requests;
            requests.push_back(Ogre::VertexArrayObject::ReadRequests(Ogre::VES_POSITION));
            requests.push_back(Ogre::VertexArrayObject::ReadRequests(Ogre::VES_NORMAL));
            requests.push_back(Ogre::VertexArrayObject::ReadRequests(Ogre::VES_TEXTURE_COORDINATES));
            vao->readRequests(requests);
            vao->mapAsyncTickets(requests);

            unsigned int subMeshVerticesNum = static_cast<unsigned int>(requests[0].vertexBuffer->getNumElements());
            bool isQTangent = (requests[1].type == Ogre::VET_SHORT4_SNORM);

            // --- vertex extraction loop (unchanged from your original) ---
            for (size_t i = 0; i < subMeshVerticesNum; ++i)
            {
                if (requests[0].type == Ogre::VET_HALF4)
                {
                    const Ogre::uint16* posData = reinterpret_cast<const Ogre::uint16*>(requests[0].data);
                    this->vertices[i + subMeshOffset].x = Ogre::Bitwise::halfToFloat(posData[0]);
                    this->vertices[i + subMeshOffset].y = Ogre::Bitwise::halfToFloat(posData[1]);
                    this->vertices[i + subMeshOffset].z = Ogre::Bitwise::halfToFloat(posData[2]);
                }
                else
                {
                    const float* posData = reinterpret_cast<const float*>(requests[0].data);
                    this->vertices[i + subMeshOffset].x = posData[0];
                    this->vertices[i + subMeshOffset].y = posData[1];
                    this->vertices[i + subMeshOffset].z = posData[2];
                }

                if (isQTangent)
                {
                    const Ogre::int16* normData = reinterpret_cast<const Ogre::int16*>(requests[1].data);
                    Ogre::Quaternion qTangent;
                    qTangent.x = static_cast<float>(normData[0]) / 32767.0f;
                    qTangent.y = static_cast<float>(normData[1]) / 32767.0f;
                    qTangent.z = static_cast<float>(normData[2]) / 32767.0f;
                    qTangent.w = static_cast<float>(normData[3]) / 32767.0f;
                    float reflection = (qTangent.w < 0) ? -1.0f : 1.0f;
                    this->normals[i + subMeshOffset] = qTangent.xAxis();
                    this->tangents[i + subMeshOffset] = Ogre::Vector4(qTangent.yAxis().x, qTangent.yAxis().y, qTangent.yAxis().z, reflection);
                    this->vertexFormat.hasTangent = true;
                }
                else if (requests[1].type == Ogre::VET_HALF4)
                {
                    const Ogre::uint16* normData = reinterpret_cast<const Ogre::uint16*>(requests[1].data);
                    this->normals[i + subMeshOffset].x = Ogre::Bitwise::halfToFloat(normData[0]);
                    this->normals[i + subMeshOffset].y = Ogre::Bitwise::halfToFloat(normData[1]);
                    this->normals[i + subMeshOffset].z = Ogre::Bitwise::halfToFloat(normData[2]);
                }
                else
                {
                    const float* normData = reinterpret_cast<const float*>(requests[1].data);
                    this->normals[i + subMeshOffset].x = normData[0];
                    this->normals[i + subMeshOffset].y = normData[1];
                    this->normals[i + subMeshOffset].z = normData[2];
                }

                if (requests[2].type == Ogre::VET_HALF2)
                {
                    const Ogre::uint16* uvData = reinterpret_cast<const Ogre::uint16*>(requests[2].data);
                    this->uvCoordinates[i + subMeshOffset].x = Ogre::Bitwise::halfToFloat(uvData[0]);
                    this->uvCoordinates[i + subMeshOffset].y = Ogre::Bitwise::halfToFloat(uvData[1]);
                }
                else
                {
                    const float* uvData = reinterpret_cast<const float*>(requests[2].data);
                    this->uvCoordinates[i + subMeshOffset].x = uvData[0];
                    this->uvCoordinates[i + subMeshOffset].y = uvData[1];
                }

                requests[0].data += requests[0].vertexBuffer->getBytesPerElement();
                requests[1].data += requests[1].vertexBuffer->getBytesPerElement();
                requests[2].data += requests[2].vertexBuffer->getBytesPerElement();
            }

            subMeshOffset += subMeshVerticesNum;
            vao->unmapAsyncTickets(requests);

            // --- index extraction (unchanged) ---
            Ogre::IndexBufferPacked* indexBuffer = vao->getIndexBuffer();
            if (indexBuffer)
            {
                const void* shadowData = indexBuffer->getShadowCopy();
                if (shadowData)
                {
                    if (this->isIndices32)
                    {
                        const Ogre::uint32* pIndices = reinterpret_cast<const Ogre::uint32*>(shadowData);
                        for (size_t i = 0; i < indexBuffer->getNumElements(); ++i)
                        {
                            this->indices[addedIndices++] = pIndices[i] + index_offset;
                        }
                    }
                    else
                    {
                        const Ogre::uint16* pShortIndices = reinterpret_cast<const Ogre::uint16*>(shadowData);
                        for (size_t i = 0; i < indexBuffer->getNumElements(); ++i)
                        {
                            this->indices[addedIndices++] = static_cast<Ogre::uint32>(pShortIndices[i]) + index_offset;
                        }
                    }
                }
                else
                {
                    Ogre::AsyncTicketPtr asyncTicket = indexBuffer->readRequest(0, indexBuffer->getNumElements());
                    if (this->isIndices32)
                    {
                        const Ogre::uint32* pIndices = reinterpret_cast<const Ogre::uint32*>(asyncTicket->map());
                        for (size_t i = 0; i < indexBuffer->getNumElements(); ++i)
                        {
                            this->indices[addedIndices++] = pIndices[i] + index_offset;
                        }
                    }
                    else
                    {
                        const Ogre::uint16* pShortIndices = reinterpret_cast<const Ogre::uint16*>(asyncTicket->map());
                        for (size_t i = 0; i < indexBuffer->getNumElements(); ++i)
                        {
                            this->indices[addedIndices++] = static_cast<Ogre::uint32>(pShortIndices[i]) + index_offset;
                        }
                    }
                    asyncTicket->unmap();
                }
            }

            index_offset += static_cast<unsigned int>(vao->getVertexBuffers()[0]->getNumElements());

            // NEW: record this submesh's range
            SubMeshInfo smInfo;
            smInfo.vertexOffset = smVertexStart;
            smInfo.vertexCount = subMeshVerticesNum;
            smInfo.indexOffset = smIndexStart;
            smInfo.indexCount = indexBuffer ? indexBuffer->getNumElements() : 0;
            smInfo.hasTangent = this->vertexFormat.hasTangent;
            smInfo.floatsPerVertex = smInfo.hasTangent ? 12u : 8u;
            this->subMeshInfoList.push_back(smInfo);
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MeshModifyComponent] Extracted " + Ogre::StringConverter::toString(this->vertexCount) + " vertices and " + Ogre::StringConverter::toString(this->indexCount) +
                                                                               " indices across " + Ogre::StringConverter::toString(this->subMeshInfoList.size()) + " submesh(es)" +
                                                                               (this->vertexFormat.hasTangent ? " (with tangents)" : " (no tangents)"));

        this->originalVertices = this->vertices;
        this->originalNormals = this->normals;
        this->originalTangents = this->tangents;

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MeshModifyComponent] Extracted " + Ogre::StringConverter::toString(this->vertexCount) + " vertices, " + Ogre::StringConverter::toString(this->indexCount) +
                                                                               " indices across " + Ogre::StringConverter::toString(this->subMeshInfoList.size()) + " submesh(es)");

        return true;
    }

    bool MeshModifyComponent::createDynamicBuffers(void)
    {
        Ogre::Root* root = Ogre::Root::getSingletonPtr();
        Ogre::RenderSystem* renderSystem = root->getRenderSystem();
        Ogre::VaoManager* vaoManager = renderSystem->getVaoManager();

        Ogre::Item* originalItem = this->gameObjectPtr->getMovableObject<Ogre::Item>();

        Ogre::String meshName = this->gameObjectPtr->getName() + "_Editable_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());

        this->editableMesh = Ogre::MeshManager::getSingleton().createManual(meshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
        this->editableMesh->_setVaoManager(vaoManager);

        // Global AABB across all submeshes
        Ogre::Vector3 minBounds(std::numeric_limits<float>::max());
        Ogre::Vector3 maxBounds(-std::numeric_limits<float>::max());

        for (size_t smIdx = 0; smIdx < this->subMeshInfoList.size(); ++smIdx)
        {
            SubMeshInfo& smInfo = this->subMeshInfoList[smIdx];

            // ---- Does this submesh's datablock need tangents? ----
            bool needsTangents = false;
            if (originalItem && smIdx < originalItem->getNumSubItems())
            {
                Ogre::HlmsDatablock* db = originalItem->getSubItem(smIdx)->getDatablock();
                Ogre::HlmsPbsDatablock* pbsDb = dynamic_cast<Ogre::HlmsPbsDatablock*>(db);
                if (pbsDb && pbsDb->getTexture(Ogre::PBSM_NORMAL))
                {
                    needsTangents = true;
                }
            }

            bool includeTangents = smInfo.hasTangent || needsTangents;

            // Generate tangents if needed but absent
            if (includeTangents && !smInfo.hasTangent)
            {
                smInfo.hasTangent = true;
                this->vertexFormat.hasTangent = true; // also set global flag
                this->recalculateTangents();
                // Patch originals too so reset works correctly
                for (size_t i = smInfo.vertexOffset; i < smInfo.vertexOffset + smInfo.vertexCount; ++i)
                {
                    this->originalTangents[i] = this->tangents[i];
                }
            }

            smInfo.floatsPerVertex = includeTangents ? 12u : 8u;

            // ---- Build vertex element declaration ----
            Ogre::VertexElement2Vec vertexElements;
            vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_POSITION));
            vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_NORMAL));
            if (includeTangents)
            {
                vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT4, Ogre::VES_TANGENT));
            }
            vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES));

            // ---- Pack interleaved vertex data for this submesh ----
            const size_t fpv = smInfo.floatsPerVertex;
            const size_t vStart = smInfo.vertexOffset;
            const size_t vCount = smInfo.vertexCount;
            const size_t vDataSz = vCount * fpv * sizeof(float);

            float* vertexData = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(vDataSz, Ogre::MEMCATEGORY_GEOMETRY));

            for (size_t i = 0; i < vCount; ++i)
            {
                const size_t gi = vStart + i; // global index
                const size_t offset = i * fpv;

                vertexData[offset + 0] = this->vertices[gi].x;
                vertexData[offset + 1] = this->vertices[gi].y;
                vertexData[offset + 2] = this->vertices[gi].z;
                vertexData[offset + 3] = this->normals[gi].x;
                vertexData[offset + 4] = this->normals[gi].y;
                vertexData[offset + 5] = this->normals[gi].z;

                size_t nextOff = 6;
                if (includeTangents)
                {
                    vertexData[offset + nextOff + 0] = this->tangents[gi].x;
                    vertexData[offset + nextOff + 1] = this->tangents[gi].y;
                    vertexData[offset + nextOff + 2] = this->tangents[gi].z;
                    vertexData[offset + nextOff + 3] = this->tangents[gi].w;
                    nextOff += 4;
                }
                vertexData[offset + nextOff + 0] = this->uvCoordinates[gi].x;
                vertexData[offset + nextOff + 1] = this->uvCoordinates[gi].y;

                // Accumulate bounds
                minBounds.makeFloor(this->vertices[gi]);
                maxBounds.makeCeil(this->vertices[gi]);
            }

            try
            {
                smInfo.dynamicVertexBuffer = vaoManager->createVertexBuffer(vertexElements, vCount, Ogre::BT_DYNAMIC_DEFAULT, vertexData, false);
            }
            catch (Ogre::Exception& e)
            {
                OGRE_FREE_SIMD(vertexData, Ogre::MEMCATEGORY_GEOMETRY);
                throw;
            }

            // ---- Build LOCAL index buffer (indices relative to this submesh's vertex 0) ----
            const size_t iCount = smInfo.indexCount;
            const size_t iStart = smInfo.indexOffset;
            const size_t iDataSz = iCount * sizeof(Ogre::uint16);

            Ogre::uint16* indexData = reinterpret_cast<Ogre::uint16*>(OGRE_MALLOC_SIMD(iDataSz, Ogre::MEMCATEGORY_GEOMETRY));

            for (size_t i = 0; i < iCount; ++i)
            {
                // Subtract vertexOffset to convert global -> local index
                const Ogre::uint32 localIdx = this->indices[iStart + i] - static_cast<Ogre::uint32>(vStart);
                indexData[i] = static_cast<Ogre::uint16>(localIdx);
            }

            try
            {
                smInfo.dynamicIndexBuffer = vaoManager->createIndexBuffer(Ogre::IndexBufferPacked::IT_16BIT, iCount, Ogre::BT_DEFAULT, indexData, true);
            }
            catch (Ogre::Exception& e)
            {
                OGRE_FREE_SIMD(indexData, Ogre::MEMCATEGORY_GEOMETRY);
                throw;
            }

            // ---- Create VAO and SubMesh ----
            Ogre::VertexBufferPackedVec vbVec;
            vbVec.push_back(smInfo.dynamicVertexBuffer);

            Ogre::VertexArrayObject* vao = vaoManager->createVertexArrayObject(vbVec, smInfo.dynamicIndexBuffer, Ogre::OT_TRIANGLE_LIST);

            Ogre::SubMesh* subMesh = this->editableMesh->createSubMesh();
            subMesh->mVao[Ogre::VpNormal].push_back(vao);
            subMesh->mVao[Ogre::VpShadow].push_back(vao);

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
                "[MeshModifyComponent] SubMesh " + Ogre::StringConverter::toString(smIdx) + ": " + Ogre::StringConverter::toString(vCount) + " verts, " + Ogre::StringConverter::toString(iCount) + " indices" + (includeTangents ? " (tangents)" : ""));
        }

        // ---- Set mesh bounds ----
        Ogre::Aabb bounds;
        bounds.setExtents(minBounds, maxBounds);
        this->editableMesh->_setBounds(bounds, false);
        this->editableMesh->_setBoundingSphereRadius(bounds.getRadius());

        // ---- Create Item ----
        this->editableItem = this->gameObjectPtr->getSceneManager()->createItem(this->editableMesh, this->gameObjectPtr->isDynamic() ? Ogre::SCENE_DYNAMIC : Ogre::SCENE_STATIC);
        this->editableItem->setName(this->gameObjectPtr->getName() + "_Editable");
        // ---- Apply datablocks from original subitems ----
        for (size_t smIdx = 0; smIdx < this->subMeshInfoList.size(); ++smIdx)
        {
            if (originalItem && smIdx < originalItem->getNumSubItems() && smIdx < this->editableItem->getNumSubItems())
            {
                Ogre::HlmsDatablock* db = originalItem->getSubItem(smIdx)->getDatablock();
                if (db)
                {
                    this->editableItem->getSubItem(smIdx)->setDatablock(db);
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MeshModifyComponent] SubItem " + Ogre::StringConverter::toString(smIdx) + " datablock: " + db->getName().getFriendlyText());
                }
            }
        }

        // Keep legacy pointer valid for null-checks in mouseReleased etc.
        this->dynamicVertexBuffer = this->subMeshInfoList.empty() ? nullptr : this->subMeshInfoList[0].dynamicVertexBuffer;
        this->dynamicIndexBuffer = this->subMeshInfoList.empty() ? nullptr : this->subMeshInfoList[0].dynamicIndexBuffer;

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MeshModifyComponent] Created editable mesh with " + Ogre::StringConverter::toString(this->subMeshInfoList.size()) + " submesh(es)");

        return true;
    }

    void MeshModifyComponent::uploadVertexData(void)
    {
        if (this->subMeshInfoList.empty())
        {
            return;
        }

        for (SubMeshInfo& smInfo : this->subMeshInfoList)
        {
            if (nullptr == smInfo.dynamicVertexBuffer)
            {
                continue;
            }

            const size_t fpv = smInfo.floatsPerVertex;
            const size_t vStart = smInfo.vertexOffset;
            const size_t vCount = smInfo.vertexCount;
            const size_t dataSz = vCount * fpv * sizeof(float);

            float* vertexData = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(dataSz, Ogre::MEMCATEGORY_GEOMETRY));

            for (size_t i = 0; i < vCount; ++i)
            {
                const size_t gi = vStart + i;
                const size_t offset = i * fpv;

                vertexData[offset + 0] = this->vertices[gi].x;
                vertexData[offset + 1] = this->vertices[gi].y;
                vertexData[offset + 2] = this->vertices[gi].z;
                vertexData[offset + 3] = this->normals[gi].x;
                vertexData[offset + 4] = this->normals[gi].y;
                vertexData[offset + 5] = this->normals[gi].z;

                size_t nextOff = 6;
                if (smInfo.hasTangent)
                {
                    vertexData[offset + nextOff + 0] = this->tangents[gi].x;
                    vertexData[offset + nextOff + 1] = this->tangents[gi].y;
                    vertexData[offset + nextOff + 2] = this->tangents[gi].z;
                    vertexData[offset + nextOff + 3] = this->tangents[gi].w;
                    nextOff += 4;
                }
                vertexData[offset + nextOff + 0] = this->uvCoordinates[gi].x;
                vertexData[offset + nextOff + 1] = this->uvCoordinates[gi].y;
            }

            smInfo.dynamicVertexBuffer->upload(vertexData, 0, vCount);
            OGRE_FREE_SIMD(vertexData, Ogre::MEMCATEGORY_GEOMETRY);
        }
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
        // Check if max damage has been reached
        if (this->currentDamage >= this->maxDamage->getReal())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MeshModifyComponent] applyBrush: max damage reached (" + Ogre::StringConverter::toString(this->currentDamage * 100.0f) + "%), skipping.");
            return;
        }

        Ogre::Real brushRadius = this->brushSize->getReal();
        BrushMode mode = this->getBrushMode();

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
            "[MeshModifyComponent] applyBrush called at: " + Ogre::StringConverter::toString(brushCenterLocal) + ", brushRadius: " + Ogre::StringConverter::toString(brushRadius) + ", mode: " + Ogre::StringConverter::toString(static_cast<int>(mode)));

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

    void MeshModifyComponent::applyBrushAlongDirection(const Ogre::Vector3& brushCenterLocal, const Ogre::Vector3& direction, bool invertEffect)
    {
        // Check if max damage has been reached
        if (this->currentDamage >= this->maxDamage->getReal())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MeshModifyComponent] applyBrush: max damage reached (" + Ogre::StringConverter::toString(this->currentDamage * 100.0f) + "%), skipping.");
            return;
        }

        Ogre::Real brushRadius = this->brushSize->getReal();
        Ogre::Real intensity = this->brushIntensity->getReal();

        // Contact normal points FROM wall TOWARD car (outward).
        // To dent the car we must push INWARD = opposite to normal.
        // invertEffect=false -> dent inward (default for crashes)
        // invertEffect=true  -> push outward
        Ogre::Vector3 deformDir = invertEffect ? direction : -direction;
        deformDir.normalise();

        size_t affectedCount = 0;
        for (size_t i = 0; i < this->vertexCount; ++i)
        {
            Ogre::Real distance = this->vertices[i].distance(brushCenterLocal);
            if (distance < brushRadius)
            {
                Ogre::Real influence = this->calculateBrushInfluence(distance, brushRadius) * intensity;
                // All vertices move along the same impact direction — no spikes
                this->vertices[i] += deformDir * influence;
                ++affectedCount;
            }
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MeshModifyComponent] applyBrushAlongDirection: affected=" + Ogre::StringConverter::toString(affectedCount));

        if (affectedCount == 0)
        {
            return;
        }

        // Accumulate damage — affected vertex ratio * intensity
        Ogre::Real damageFraction = (static_cast<Ogre::Real>(affectedCount) / static_cast<Ogre::Real>(this->vertexCount)) * intensity;
        this->currentDamage = Ogre::Math::Clamp(this->currentDamage + damageFraction, 0.0f, 1.0f);
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MeshModifyComponent] currentDamage=" + Ogre::StringConverter::toString(this->currentDamage * 100.0f) + "%");

        this->recalculateNormals();
        this->recalculateTangents();

        GraphicsModule::RenderCommand renderCommand = [this]()
        {
            this->uploadVertexData();
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MeshModifyComponent::applyBrushAlongDirection");
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
        // Check if ANY submesh needs tangents
        bool anyHasTangent = false;
        for (const SubMeshInfo& smInfo : this->subMeshInfoList)
        {
            if (smInfo.hasTangent)
            {
                anyHasTangent = true;
                break;
            }
        }
        // Fallback for when subMeshInfoList isn't populated yet (e.g. called during init)
        if (!anyHasTangent && !this->vertexFormat.hasTangent)
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

        // Build with sets for O(1) duplicate rejection, then convert once
        std::vector<std::unordered_set<size_t>> sets(this->vertexCount);

        for (size_t i = 0; i < this->indexCount; i += 3)
        {
            const size_t i0 = this->indices[i];
            const size_t i1 = this->indices[i + 1];
            const size_t i2 = this->indices[i + 2];

            sets[i0].insert(i1);
            sets[i0].insert(i2);
            sets[i1].insert(i0);
            sets[i1].insert(i2);
            sets[i2].insert(i0);
            sets[i2].insert(i1);
        }

        for (size_t i = 0; i < this->vertexCount; ++i)
        {
            this->vertexNeighbors[i].assign(sets[i].begin(), sets[i].end());
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
        if (false == this->activated->getBool())
        {
            return;
        }

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

        if (nullptr == this->editableItem)
        {
            return true; // not handled -> bubble
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

        return true; // not handled -> bubble
    }

    bool MeshModifyComponent::mouseMoved(const OIS::MouseEvent& evt)
    {
        if (false == this->activated->getBool())
        {
            return true; // not handled -> bubble
        }

        if (false == this->isPressing)
        {
            return true; // not handled -> bubble
        }

        if (nullptr == this->editableItem)
        {
            return true; // not handled -> bubble
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

        return true; // not handled -> bubble
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

        return true; // not handled -> bubble
    }

    // ==================== STATIC METHODS ====================

    bool MeshModifyComponent::canStaticAddComponent(GameObject* gameObject)
    {
        // Can only add to GameObjects with an Item that has exactly one SubItem
        Ogre::Item* item = gameObject->getMovableObjectUnsafe<Ogre::Item>();
        if (nullptr != item)
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

    MeshModifyComponent* getMeshModifyComponent(GameObject* gameObject)
    {
        return makeStrongPtr<MeshModifyComponent>(gameObject->getComponent<MeshModifyComponent>()).get();
    }

    MeshModifyComponent* getMeshModifyComponentFromIndex(GameObject* gameObject, unsigned int occurrenceIndex)
    {
        return makeStrongPtr<MeshModifyComponent>(gameObject->getComponentWithOccurrence<MeshModifyComponent>(occurrenceIndex)).get();
    }

    MeshModifyComponent* getMeshModifyComponentFromName(GameObject* gameObject, const Ogre::String& name)
    {
        return makeStrongPtr<MeshModifyComponent>(gameObject->getComponentFromName<MeshModifyComponent>(name)).get();
    }

    void MeshModifyComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
    {
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
            .def("deformAtWorldPosition", &MeshModifyComponent::deformAtWorldPosition)
            .def("deformAtWorldPositionByForce", &MeshModifyComponent::deformAtWorldPositionByForce)
            .def("deformAtWorldPositionByForceAndNormal", &MeshModifyComponent::deformAtWorldPositionByForceAndNormal)
            .def("rebuildCollision", &MeshModifyComponent::rebuildCollision)
            .def("setMaxDamage", &MeshModifyComponent::setMaxDamage)
            .def("getMaxDamage", &MeshModifyComponent::getMaxDamage)
            .def("getCurrentDamage", &MeshModifyComponent::getCurrentDamage)
            .def("resetDamage", &MeshModifyComponent::resetDamage)
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

        LuaScriptApi::getInstance()->addClassToCollection("MeshModifyComponent", "void setMaxDamage(Real maxDamage)",
            "Sets the maximum damage factor (0.0 - 1.0). 1.0 = fully deformable (scrap press), "
            "0.7 = stops deforming after 70% damage. Default is 1.0.");
        LuaScriptApi::getInstance()->addClassToCollection("MeshModifyComponent", "Real getMaxDamage()", "Gets the maximum damage factor (0.0 - 1.0).");
        LuaScriptApi::getInstance()->addClassToCollection("MeshModifyComponent", "Real getCurrentDamage()",
            "Gets the current accumulated damage as a factor (0.0 - 1.0). "
            "Can be used to trigger game events, e.g. if meshModify:getCurrentDamage() > 0.5 then explode() end");
        LuaScriptApi::getInstance()->addClassToCollection("MeshModifyComponent", "void resetDamage()", "Resets the accumulated damage back to 0. Call this after repairing the vehicle or restarting.");

        LuaScriptApi::getInstance()->addClassToCollection("MeshModifyComponent", "void deformAtWorldPosition(Vector3 worldPos, bool invertEffect)",
            "Deforms the mesh at a world-space position (e.g. physics contact point). invertEffect=true pulls instead of pushes.");
        LuaScriptApi::getInstance()->addClassToCollection("MeshModifyComponent", "void deformAtWorldPositionByForce(Vector3 worldPos, Real impactStrength, Real maxForceForFullDeform, bool invertEffect)",
            "Deforms mesh scaled by impact force. impactStrength/maxForceForFullDeform = intensity multiplier [0,1].");
        LuaScriptApi::getInstance()->addClassToCollection("MeshModifyComponent", "void rebuildCollision()", "Rebuilds collision. Can be called in lua script if e.g. vehicle hit an obstacle and is deformed and then calc new collision shape at runtime.");
        LuaScriptApi::getInstance()->addClassToCollection("MeshModifyComponent", "void deformAtWorldPositionByForceAndNormal(Vector3 worldPos, Vector3 worldNormal, Real impactStrength, Real maxForceForFullDeform, bool invertEffect)",
            "Deforms the mesh at a world-space contact position along the contact normal, scaled by impact force. "
            "All affected vertices move along the same direction, producing a clean dent without spiking artifacts. "
            "Use this instead of deformAtWorldPositionByForce when you have the contact normal available. "
            "worldPos: contact point in world space. "
            "worldNormal: contact normal in world space (from contact:getPositionAndNormal()). "
            "impactStrength: impact force magnitude contact:getNormalSpeed(). "
            "maxForceForFullDeform: force value that produces maximum deformation — tune this to match your physics scale. "
            "invertEffect: true=dent inward (default for crashes), false=push outward. "
            "Usage: local data = contact:getPositionAndNormal(); "
            "meshModify:deformAtWorldPositionByForceAndNormal(data[0], data[1], contact:getNormalSpeed(), 80.0, true);");

        gameObjectClass.def("getMeshModifyComponent", (MeshModifyComponent * (*)(GameObject*)) & getMeshModifyComponent);
        gameObjectClass.def("getMeshModifyComponentFromIndex", (MeshModifyComponent * (*)(GameObject*, unsigned int)) & getMeshModifyComponentFromIndex);
        gameObjectClass.def("getMeshModifyComponentFromName", &getMeshModifyComponentFromName);

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "MeshModifyComponent getMeshModifyComponent()", "Gets the component.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "MeshModifyComponent getMeshModifyComponentFromIndex(unsigned int occurrenceIndex)", "Gets the component by occurrence index.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "MeshModifyComponent getMeshModifyComponentFromName(String name)", "Gets the component from name.");

        gameObjectControllerClass.def("castMeshModifyComponent", &GameObjectController::cast<MeshModifyComponent>);
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "MeshModifyComponent castMeshModifyComponent(MeshModifyComponent other)", "Casts an incoming type from function for lua auto completion.");
    }

}; // namespace NOWA
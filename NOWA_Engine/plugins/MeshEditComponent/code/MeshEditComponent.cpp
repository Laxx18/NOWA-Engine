/*
Copyright (c) 2026 Lukas Kalinowski

GPL v3
*/

#include "NOWAPrecompiled.h"
#include "MeshEditComponent.h"
#include "gameobject/GameObjectFactory.h"
#include "main/AppStateManager.h"
#include "main/EventManager.h"
#include "modules/LuaScriptApi.h"
#include "utilities/XMLConverter.h"

#include "OgreAbiUtils.h"
#include "OgreBitwise.h"
#include "OgreHlmsPbsDatablock.h"
#include "OgreItem.h"
#include "OgreManualObject2.h"
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

    // =========================================================================
    //  Constructor
    // =========================================================================

    MeshEditComponent::MeshEditComponent() :
        GameObjectComponent(),
        componentName("MeshEditComponent"),
        editableItem(nullptr),
        dynamicVertexBuffer(nullptr),
        dynamicIndexBuffer(nullptr),
        isIndices32(false),
        vertexCount(0),
        indexCount(0),
        brushImageWidth(0),
        brushImageHeight(0),
        activeTool(ActiveTool::SELECT),
        isGrabbing(false),
        isBrushArmed(false),
        overlayNode(nullptr),
        overlayObject(nullptr),
        overlayDirty(false),
        isPressing(false),
        isShiftPressed(false),
        isCtrlPressed(false),
        isEditorMeshModifyMode(false),
        isSelected(false),
        isSimulating(false),
        // ── Attributes — list Variants are initialized inline with their options ──
        activated(new Variant(MeshEditComponent::AttrActivated(), true, this->attributes)),
        editMode(new Variant(MeshEditComponent::AttrEditMode(), std::vector<Ogre::String>{"Object", "Vertex", "Edge", "Face"}, this->attributes)),
        showWireframe(new Variant(MeshEditComponent::AttrShowWireframe(), true, this->attributes)),
        xRayOverlay(new Variant(MeshEditComponent::AttrXRayOverlay(), true, this->attributes)),
        vertexMarkerSize(new Variant(MeshEditComponent::AttrVertexMarkerSize(), 0.04f, this->attributes)),
        outputFileName(new Variant(MeshEditComponent::AttrOutputFileName(), Ogre::String(""), this->attributes)),
        applyMeshButton(new Variant(MeshEditComponent::AttrApplyMesh(), Ogre::String("Apply Mesh"), this->attributes)),
        brushName(new Variant(MeshEditComponent::AttrBrushName(), this->attributes)),
        brushSize(new Variant(MeshEditComponent::AttrBrushSize(), 1.0f, this->attributes)),
        brushIntensity(new Variant(MeshEditComponent::AttrBrushIntensity(), 0.1f, this->attributes)),
        brushFalloff(new Variant(MeshEditComponent::AttrBrushFalloff(), 2.0f, this->attributes)),
        brushMode(new Variant(MeshEditComponent::AttrBrushMode(), std::vector<Ogre::String>{"Push", "Pull", "Smooth", "Flatten", "Pinch", "Inflate"}, this->attributes)),
        cancelEditButton(new Variant(MeshEditComponent::AttrCancelEdit(), Ogre::String("Cancel Edit"), this->attributes))
    {
        // Apply button triggers executeAction via the editor
        this->applyMeshButton->addUserData(GameObject::AttrActionExec());
        this->applyMeshButton->addUserData(GameObject::AttrActionExecId(), MeshEditComponent::ActionApplyMesh());
        this->applyMeshButton->setDescription("Exports the edited mesh to <Output File Name>.mesh. "
                                              "Use this file as a standalone mesh asset after editing.");

        this->outputFileName->setDescription("Output file name (no extension). "
                                             "Defaults to <original>_edit. Never overwrites the original mesh.");

        this->vertexMarkerSize->setConstraints(0.005f, 1.0f);
        this->vertexMarkerSize->addUserData(GameObject::AttrActionNoUndo());

        this->cancelEditButton->addUserData(GameObject::AttrActionExec());
        this->cancelEditButton->addUserData(GameObject::AttrActionExecId(), MeshEditComponent::ActionCancelEdit());
        this->cancelEditButton->setDescription("Discards all edits and restores the original mesh.");
    }

    MeshEditComponent::~MeshEditComponent()
    {
    }

    // =========================================================================
    //  Ogre::Plugin
    // =========================================================================

    const Ogre::String& MeshEditComponent::getName() const
    {
        return this->componentName;
    }

    void MeshEditComponent::install(const Ogre::NameValuePairList* options)
    {
        GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<MeshEditComponent>(MeshEditComponent::getStaticClassId(), MeshEditComponent::getStaticClassName());
    }

    void MeshEditComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
    {
        outAbiCookie = Ogre::generateAbiCookie();
    }

    // =========================================================================
    //  init (XML read-back)
    // =========================================================================

    bool MeshEditComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        GameObjectComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrActivated())
        {
            this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrEditMode())
        {
            this->editMode->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", "Object"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrShowWireframe())
        {
            this->showWireframe->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrXRayOverlay())
        {
            this->xRayOverlay->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrVertexMarkerSize())
        {
            this->vertexMarkerSize->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.04f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrOutputFileName())
        {
            this->outputFileName->setValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrBrushName())
        {
            this->brushName->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrBrushSize())
        {
            this->brushSize->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrBrushIntensity())
        {
            this->brushIntensity->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.1f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrBrushFalloff())
        {
            this->brushFalloff->setValue(XMLConverter::getAttribReal(propertyElement, "data", 2.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrBrushMode())
        {
            this->brushMode->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", "Push"));
            propertyElement = propertyElement->next_sibling("property");
        }
        return true;
    }

    // =========================================================================
    //  postInit
    // =========================================================================

    bool MeshEditComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MeshEditComponent] Init for: " + this->gameObjectPtr->getName());

        // ── Register event listeners ──────────────────────────────────────────
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &MeshEditComponent::handleMeshModifyMode), NOWA::EventDataEditorMode::getStaticEventType());
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &MeshEditComponent::handleGameObjectSelected), NOWA::EventDataGameObjectSelected::getStaticEventType());

        // ── Brush name list (loaded from resource group) ──────────────────────
        Ogre::StringVectorPtr brushNames = Ogre::ResourceGroupManager::getSingleton().findResourceNames("Brushes", "*.png");
        std::vector<Ogre::String> brushList;
        if (brushNames.get())
        {
            for (const auto& f : *brushNames)
            {
                brushList.push_back(f);
            }
        }
        if (!brushList.empty())
        {
            this->brushName->setValue(brushList);
            this->brushName->setListSelectedValue(brushList[0]);
        }
        this->brushName->addUserData(GameObject::AttrActionImage());
        this->brushName->addUserData(GameObject::AttrActionNoUndo());

        // ── Brush parameter constraints ───────────────────────────────────────
        this->brushSize->setConstraints(0.01f, 100.0f);
        this->brushSize->addUserData(GameObject::AttrActionNoUndo());
        this->brushIntensity->setConstraints(0.001f, 1.0f);
        this->brushIntensity->addUserData(GameObject::AttrActionNoUndo());
        this->brushFalloff->setConstraints(0.1f, 10.0f);
        this->brushFalloff->addUserData(GameObject::AttrActionNoUndo());

        // ── Store original mesh name, build default output name ───────────────
        Ogre::Item* existingItem = this->gameObjectPtr->getMovableObject<Ogre::Item>();
        if (!existingItem)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MeshEditComponent] No Item on: " + this->gameObjectPtr->getName());
            return false;
        }
        this->originalMeshName = existingItem->getMesh()->getName();

        if (this->outputFileName->getString().empty())
        {
            this->outputFileName->setValue(this->buildDefaultOutputName());
        }

        // ── Build editable mesh ───────────────────────────────────────────────
        if (!this->prepareEditableMesh())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MeshEditComponent] prepareEditableMesh failed for: " + this->gameObjectPtr->getName());
            return false;
        }

        // ── Swap original item out, editable item in — atomically on render thread ──
        NOWA::GraphicsModule::RenderCommand swapCmd = [this]()
        {
            Ogre::MovableObject* old = nullptr;
            this->gameObjectPtr->swapMovableObject(this->editableItem, old);
            if (old)
            {
                // destroy in the same render command — zero frames where both exist
                this->gameObjectPtr->getSceneManager()->destroyItem(static_cast<Ogre::Item*>(old));
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(swapCmd), "MeshEditComponent::postInit::swapAndDestroyOriginal");

        // ── Create overlay ────────────────────────────────────────────────────
        this->createOverlay();

        // Do NOT add input listener here — updateModificationState() will do it
        // once the editor fires EDITOR_MESH_MODIFY_MODE and selects this object.

        return true;
    }

    // =========================================================================
    //  connect / disconnect
    // =========================================================================

    bool MeshEditComponent::connect(void)
    {
        this->isSimulating = true;
        this->removeInputListener();

        NOWA::GraphicsModule::RenderCommand cmd = [this]()
        {
            if (this->overlayNode)
            {
                this->overlayNode->setVisible(false);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueue(std::move(cmd), "MeshEditComponent::connect::hideOverlay");
        return true;
    }

    bool MeshEditComponent::disconnect(void)
    {
        this->isSimulating = false;
        this->updateModificationState();

        const bool show = (this->getEditMode() != EditMode::OBJECT);
        NOWA::GraphicsModule::RenderCommand cmd = [this, show]()
        {
            if (this->overlayNode)
            {
                this->overlayNode->setVisible(show);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueue(std::move(cmd), "MeshEditComponent::disconnect::overlay");
        if (show)
        {
            this->scheduleOverlayUpdate();
        }
        return true;
    }

    // =========================================================================
    //  onAddComponent / onRemoveComponent
    // =========================================================================

    void MeshEditComponent::onAddComponent(void)
    {
        // Tell the editor to enter mesh-modify mode
        boost::shared_ptr<EventDataEditorMode> evt(new EventDataEditorMode(EditorManager::EDITOR_MESH_MODIFY_MODE));
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(evt);
    }

    void MeshEditComponent::onRemoveComponent(void)
    {
        GameObjectComponent::onRemoveComponent();

        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &MeshEditComponent::handleMeshModifyMode), NOWA::EventDataEditorMode::getStaticEventType());
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &MeshEditComponent::handleGameObjectSelected), NOWA::EventDataGameObjectSelected::getStaticEventType());

        this->removeInputListener();
        this->destroyOverlay();

        // The editable item is now the truth — do NOT restore the original.
        // Only cancelEdit() restores the original mesh.
        // Just release CPU-side data and buffer references.
        for (SubMeshInfo& sm : this->subMeshInfoList)
        {
            sm.dynamicVertexBuffer = nullptr;
            sm.dynamicIndexBuffer = nullptr;
        }
        this->subMeshInfoList.clear();
        this->dynamicVertexBuffer = nullptr;
        this->dynamicIndexBuffer = nullptr;
        this->vertices.clear();
        this->normals.clear();
        this->tangents.clear();
        this->uvCoordinates.clear();
        this->indices.clear();
        this->vertexNeighbors.clear();
        this->undoStack.clear();

        // editableItem and editableMesh stay alive — they ARE the GameObject's mesh now.
        this->editableItem = nullptr;
        this->editableMesh.reset(); // releases our shared_ptr; Ogre still owns the mesh
    }

    bool MeshEditComponent::onCloned(void)
    {
        return true;
    }
    void MeshEditComponent::onOtherComponentRemoved(unsigned int)
    {
    }
    void MeshEditComponent::onOtherComponentAdded(unsigned int)
    {
    }

    // =========================================================================
    //  clone
    // =========================================================================

    GameObjectCompPtr MeshEditComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        MeshEditComponentPtr c(boost::make_shared<MeshEditComponent>());
        c->setActivated(this->activated->getBool());
        c->setEditMode(this->getEditModeString());
        c->setBrushSize(this->brushSize->getReal());
        c->setBrushIntensity(this->brushIntensity->getReal());
        c->setBrushFalloff(this->brushFalloff->getReal());
        c->setBrushMode(this->getBrushModeString());
        c->setOutputFileName(this->outputFileName->getString());
        clonedGameObjectPtr->addComponent(c);
        c->setOwner(clonedGameObjectPtr);
        GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(c));
        return c;
    }

    // =========================================================================
    //  OIS::KeyListener
    // =========================================================================

    bool MeshEditComponent::keyPressed(const OIS::KeyEvent& evt)
    {
        if (!this->activated->getBool())
        {
            return true;
        }

        if (MyGUI::InputManager::getInstance().getMouseFocusWidget() != nullptr)
        {
            return true;
        }

        if (this->getEditMode() == EditMode::OBJECT)
        {
            return true;
        }

        if (evt.key == OIS::KC_LSHIFT || evt.key == OIS::KC_RSHIFT)
        {
            this->isShiftPressed = true;
            return false;
        }
        if (evt.key == OIS::KC_LCONTROL || evt.key == OIS::KC_RCONTROL)
        {
            this->isCtrlPressed = true;
            return false;
        }

        // Ctrl+Z : undo
        if (evt.key == OIS::KC_Z && this->isCtrlPressed && !this->isGrabbing)
        {
            this->undo();
            return false;
        }

        // G : begin grab
        if (evt.key == OIS::KC_G && !this->isGrabbing)
        {
            bool hasSel = !this->selectedVertices.empty() || !this->selectedEdges.empty() || !this->selectedFaces.empty();
            if (hasSel)
            {
                this->beginGrab();
            }
            return false;
        }

        // X : delete
        if (evt.key == OIS::KC_X && !this->isGrabbing)
        {
            this->deleteSelected();
            return false;
        }

        // A : select all / deselect all
        if (true == this->isCtrlPressed && evt.key == OIS::KC_A && !this->isGrabbing)
        {
            bool hasSel = !this->selectedVertices.empty() || !this->selectedEdges.empty() || !this->selectedFaces.empty();
            if (hasSel)
            {
                this->deselectAll();
            }
            else
            {
                this->selectAll();
            }
            return false;
        }

        // Enter : confirm grab
        if (evt.key == OIS::KC_RETURN && this->isGrabbing)
        {
            this->confirmGrab();
            return false;
        }

        // B : toggle brush mode (only in VERTEX mode)
        if (evt.key == OIS::KC_B && !this->isGrabbing)
        {
            if (this->getEditMode() != EditMode::OBJECT)
            {
                this->isBrushArmed = !this->isBrushArmed;
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, Ogre::String("[MeshEditComponent] Brush ") + (this->isBrushArmed ? "ARMED" : "disarmed"));
            }
            return false;
        }

        // Inside the grab-active block in keyPressed:
        if (this->isGrabbing)
        {
            // Ogre: X=right, Y=up, Z=depth  (Blender: X=right, Z=up, Y=depth)
            if (evt.key == OIS::KC_X)
            {
                this->grabAxis = (this->grabAxis == GrabAxis::X) ? GrabAxis::FREE : GrabAxis::X;
                return false;
            }
            if (evt.key == OIS::KC_Y)
            {
                this->grabAxis = (this->grabAxis == GrabAxis::Y) ? GrabAxis::FREE : GrabAxis::Y;
                return false;
            }
            if (evt.key == OIS::KC_Z)
            {
                this->grabAxis = (this->grabAxis == GrabAxis::Z) ? GrabAxis::FREE : GrabAxis::Z;
                return false;
            }
        }

        // Esc : cancel grab or deselect
        if (evt.key == OIS::KC_ESCAPE)
        {
            if (this->isGrabbing)
            {
                this->cancelGrab();
            }
            else
            {
                this->deselectAll();
            }
            return false;
        }

        return true;
    }

    bool MeshEditComponent::keyReleased(const OIS::KeyEvent& evt)
    {
        if (!this->activated->getBool())
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

        if (true == this->isEditorMeshModifyMode)
        {
            return false;
        }

        return true;
    }

    // =========================================================================
    //  OIS::MouseListener
    // =========================================================================

    bool MeshEditComponent::mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
    {
        if (!this->activated->getBool())
        {
            return true;
        }
        if (MyGUI::InputManager::getInstance().getMouseFocusWidget() != nullptr)
        {
            return true;
        }
        if (this->getEditMode() == EditMode::OBJECT)
        {
            return true;
        }

        if (id == OIS::MB_Right && this->isGrabbing)
        {
            this->cancelGrab();
            return false;
        }
        if (id == OIS::MB_Left && this->isGrabbing)
        {
            this->confirmGrab();
            return false;
        }
        if (id != OIS::MB_Left)
        {
            return true;
        }

        this->isPressing = true;
        const bool add = this->isShiftPressed;
        const bool rem = this->isCtrlPressed;

        Ogre::Real sx = 0, sy = 0;
        MathHelper::getInstance()->mouseToViewPort(evt.state.X.abs, evt.state.Y.abs, sx, sy, Core::getSingletonPtr()->getOgreRenderWindow());

        const EditMode mode = this->getEditMode();

        if (mode == EditMode::VERTEX)
        {
            // Brush mode armed (B was held) — paint on click
            if (this->isBrushArmed)
            {
                Ogre::Vector3 hp, hn;
                size_t ht;
                if (this->raycastMesh(sx, sy, hp, hn, ht))
                {
                    this->applyBrush(hp, rem); // Ctrl inverts brush
                }
                return false;
            }

            // Normal selection
            size_t idx;
            if (this->pickVertex(sx, sy, idx))
            {
                if (rem)
                {
                    this->selectedVertices.erase(idx);
                    this->scheduleOverlayUpdate();
                }
                else
                {
                    this->selectVertex(idx, add);
                    // scheduleOverlayUpdate is called inside selectVertex
                }
            }
            // No fallback to brush on missed click — just a miss
        }
        else if (mode == EditMode::EDGE)
        {
            EdgeKey e(0, 0);
            if (this->pickEdge(sx, sy, e))
            {
                if (rem)
                {
                    this->selectedEdges.erase(e);
                    this->scheduleOverlayUpdate();
                }
                else
                {
                    this->selectEdge(e.a, e.b, add);
                }
            }
        }
        else if (mode == EditMode::FACE)
        {
            size_t fi;
            if (this->pickFace(sx, sy, fi))
            {
                if (rem)
                {
                    this->selectedFaces.erase(fi);
                    this->scheduleOverlayUpdate();
                }
                else
                {
                    this->selectFace(fi, add);
                }
            }
        }
        return false;
    }

    bool MeshEditComponent::mouseMoved(const OIS::MouseEvent& evt)
    {
        if (!this->activated->getBool())
        {
            return true;
        }
        if (MyGUI::InputManager::getInstance().getMouseFocusWidget() != nullptr)
        {
            return true;
        }
        if (this->getEditMode() == EditMode::OBJECT)
        {
            return true;
        }

        if (this->isGrabbing)
        {
            this->applyGrabMouseDelta(evt.state.X.rel, evt.state.Y.rel);
            return false;
        }

        // Brush stroke: only when explicitly armed (B held) + LMB dragging in vertex mode
        if (this->isBrushArmed && this->isPressing && this->getEditMode() == EditMode::VERTEX)
        {
            Ogre::Real sx = 0, sy = 0;
            MathHelper::getInstance()->mouseToViewPort(evt.state.X.abs, evt.state.Y.abs, sx, sy, Core::getSingletonPtr()->getOgreRenderWindow());
            Ogre::Vector3 hp, hn;
            size_t ht;
            if (this->raycastMesh(sx, sy, hp, hn, ht))
            {
                this->applyBrush(hp, this->isCtrlPressed);
            }
            return false;
        }

        return true;
    }

    bool MeshEditComponent::mouseReleased(const OIS::MouseEvent&, OIS::MouseButtonID id)
    {
        if (id == OIS::MB_Left)
        {
            this->isPressing = false;
        }

        return true;
    }

    // =========================================================================
    //  update — nothing keyboard-related here; handled via OIS::KeyListener
    // =========================================================================

    void MeshEditComponent::update(Ogre::Real /*dt*/, bool /*notSimulating*/)
    {
        if (nullptr == this->overlayObject)
        {
            return;
        }

        const Ogre::String closureId = this->gameObjectPtr->getName() + "_MeshEditOverlay_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());

        auto closure = [this](Ogre::Real)
        {
            if (!this->overlayDirty.exchange(false) || !this->overlayObject)
            {
                return;
            }

            const EditMode mode = this->getEditMode();
            this->overlayObject->clear();
            if (mode == EditMode::OBJECT)
            {
                return;
            }

            // ── Camera for depth-offset ─────────────────────────────────────────
            Ogre::Camera* cam = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
            const float pushDist = 0.004f; // world-space nudge toward camera

            // Nudges a world-space point slightly toward the camera so overlay
            // lines never z-fight with the mesh surface.
            auto push = [&](const Ogre::Vector3& wp) -> Ogre::Vector3
            {
                if (!cam)
                {
                    return wp;
                }
                return wp + (cam->getDerivedPosition() - wp).normalisedCopy() * pushDist;
            };

            const bool doWire = (this->showWireframe->getBool() || mode == EditMode::EDGE);

            // ── Wireframe ──────────────────────────────────────────────────────
            if (doWire)
            {
                this->overlayObject->begin("WhiteNoLightingBackground", Ogre::OT_LINE_LIST);
                const Ogre::ColourValue cWire(0.35f, 0.35f, 0.35f, 1.0f);
                const Ogre::ColourValue cSel(1.0f, 0.75f, 0.0f, 1.0f);
                std::set<std::pair<size_t, size_t>> drawn;
                uint32_t idx = 0;
                for (size_t i = 0; i < this->indexCount; i += 3)
                {
                    for (int e = 0; e < 3; ++e)
                    {
                        size_t a = this->indices[i + e];
                        size_t b = this->indices[i + (e + 1) % 3];
                        if (!drawn.insert({std::min(a, b), std::max(a, b)}).second)
                        {
                            continue;
                        }
                        bool sel = (mode == EditMode::EDGE) && this->selectedEdges.count(EdgeKey(a, b));
                        const Ogre::ColourValue& c = sel ? cSel : cWire;
                        this->overlayObject->position(push(this->localToWorld(this->vertices[a])));
                        this->overlayObject->colour(c);
                        this->overlayObject->index(idx++);
                        this->overlayObject->position(push(this->localToWorld(this->vertices[b])));
                        this->overlayObject->colour(c);
                        this->overlayObject->index(idx++);
                    }
                }
                this->overlayObject->end();
            }

            // ── Vertex crosses ─────────────────────────────────────────────────
            if (mode == EditMode::VERTEX)
            {
                this->overlayObject->begin("WhiteNoLightingBackground", Ogre::OT_LINE_LIST);
                const float s = this->vertexMarkerSize->getReal();
                const Ogre::ColourValue cV(1.0f, 1.0f, 1.0f, 1.0f);
                const Ogre::ColourValue cVS(1.0f, 0.75f, 0.0f, 1.0f);
                uint32_t idx = 0;
                for (size_t i = 0; i < this->vertexCount; ++i)
                {
                    bool sel = this->selectedVertices.count(i) > 0;
                    // bool sel = this->selectedVertices.size() > 0;
                    const Ogre::ColourValue& c = sel ? cVS : cV;
                    // Crosses are offset from the mesh surface, so push each arm tip individually
                    Ogre::Vector3 wp = this->localToWorld(this->vertices[i]);
                    Ogre::Vector3 wp_pushed = push(wp); // centre already pushed

                    this->overlayObject->position(push(wp + Ogre::Vector3(-s, 0, 0)));
                    this->overlayObject->colour(c);
                    this->overlayObject->index(idx++);
                    this->overlayObject->position(push(wp + Ogre::Vector3(s, 0, 0)));
                    this->overlayObject->colour(c);
                    this->overlayObject->index(idx++);

                    this->overlayObject->position(push(wp + Ogre::Vector3(0, -s, 0)));
                    this->overlayObject->colour(c);
                    this->overlayObject->index(idx++);
                    this->overlayObject->position(push(wp + Ogre::Vector3(0, s, 0)));
                    this->overlayObject->colour(c);
                    this->overlayObject->index(idx++);

                    this->overlayObject->position(push(wp + Ogre::Vector3(0, 0, -s)));
                    this->overlayObject->colour(c);
                    this->overlayObject->index(idx++);
                    this->overlayObject->position(push(wp + Ogre::Vector3(0, 0, s)));
                    this->overlayObject->colour(c);
                    this->overlayObject->index(idx++);
                }
                this->overlayObject->end();
            }

            // ── Face centre squares ────────────────────────────────────────────
            if (mode == EditMode::FACE)
            {
                this->overlayObject->begin("WhiteNoLightingBackground", Ogre::OT_LINE_LIST);
                const float s = this->vertexMarkerSize->getReal() * 1.5f;
                const Ogre::ColourValue cF(0.5f, 0.5f, 1.0f, 1.0f);
                const Ogre::ColourValue cFS(1.0f, 0.5f, 0.0f, 1.0f);

                const Ogre::Matrix4 worldMat = this->gameObjectPtr->getSceneNode()->_getFullTransform();
                Ogre::Matrix3 worldRot3;
                worldMat.extract3x3Matrix(worldRot3);

                uint32_t idx = 0;
                for (size_t t = 0; t < this->indexCount / 3; ++t)
                {
                    const Ogre::Vector3& lv0 = this->vertices[this->indices[t * 3]];
                    const Ogre::Vector3& lv1 = this->vertices[this->indices[t * 3 + 1]];
                    const Ogre::Vector3& lv2 = this->vertices[this->indices[t * 3 + 2]];

                    Ogre::Vector3 localNorm = (lv1 - lv0).crossProduct(lv2 - lv0).normalisedCopy();
                    Ogre::Vector3 wNorm = (worldRot3 * localNorm).normalisedCopy();

                    Ogre::Vector3 ref = (std::abs(wNorm.dotProduct(Ogre::Vector3::UNIT_Y)) < 0.99f) ? Ogre::Vector3::UNIT_Y : Ogre::Vector3::UNIT_X;
                    Ogre::Vector3 wTan = wNorm.crossProduct(ref).normalisedCopy();
                    Ogre::Vector3 wBit = wNorm.crossProduct(wTan).normalisedCopy();

                    Ogre::Vector3 wCen = this->localToWorld((lv0 + lv1 + lv2) / 3.0f);
                    bool sel = this->selectedFaces.count(t) > 0;
                    const Ogre::ColourValue& c = sel ? cFS : cF;

                    // Square corners in the face's tangent plane — push each corner
                    Ogre::Vector3 c00 = push(wCen + (-wTan - wBit) * s);
                    Ogre::Vector3 c10 = push(wCen + (wTan - wBit) * s);
                    Ogre::Vector3 c11 = push(wCen + (wTan + wBit) * s);
                    Ogre::Vector3 c01 = push(wCen + (-wTan + wBit) * s);

                    this->overlayObject->position(c00);
                    this->overlayObject->colour(c);
                    this->overlayObject->index(idx++);
                    this->overlayObject->position(c10);
                    this->overlayObject->colour(c);
                    this->overlayObject->index(idx++);

                    this->overlayObject->position(c10);
                    this->overlayObject->colour(c);
                    this->overlayObject->index(idx++);
                    this->overlayObject->position(c11);
                    this->overlayObject->colour(c);
                    this->overlayObject->index(idx++);

                    this->overlayObject->position(c11);
                    this->overlayObject->colour(c);
                    this->overlayObject->index(idx++);
                    this->overlayObject->position(c01);
                    this->overlayObject->colour(c);
                    this->overlayObject->index(idx++);

                    this->overlayObject->position(c01);
                    this->overlayObject->colour(c);
                    this->overlayObject->index(idx++);
                    this->overlayObject->position(c00);
                    this->overlayObject->colour(c);
                    this->overlayObject->index(idx++);
                }
                this->overlayObject->end();
            }
        };

        NOWA::GraphicsModule::getInstance()->updateTrackedClosure(closureId, closure, false);
    }

    // =========================================================================
    //  executeAction — called by editor when the button Variant fires
    // =========================================================================

    bool MeshEditComponent::executeAction(const Ogre::String& actionId, NOWA::Variant* /*attribute*/)
    {
        if (MeshEditComponent::ActionApplyMesh() == actionId)
        {
            return this->applyMesh();
        }

        if (MeshEditComponent::ActionCancelEdit() == actionId)
        {
            this->cancelEdit();
            return true;
        }
        return true;
    }

    void MeshEditComponent::cancelEdit(void)
    {
        if (this->originalMeshName.empty())
        {
            return;
        }

        this->removeInputListener();
        this->destroyOverlay();

        NOWA::GraphicsModule::RenderCommand cmd = [this]()
        {
            try
            {
                Ogre::Item* restored =
                    this->gameObjectPtr->getSceneManager()->createItem(this->originalMeshName, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME, this->gameObjectPtr->isDynamic() ? Ogre::SCENE_DYNAMIC : Ogre::SCENE_STATIC);

                Ogre::MovableObject* oldEditable = nullptr;
                this->gameObjectPtr->swapMovableObject(restored, oldEditable);

                if (oldEditable)
                {
                    this->gameObjectPtr->getSceneManager()->destroyItem(static_cast<Ogre::Item*>(oldEditable));
                }

                this->editableItem = nullptr;

                if (this->editableMesh)
                {
                    Ogre::MeshManager::getSingleton().remove(this->editableMesh);
                    this->editableMesh.reset();
                }
            }
            catch (Ogre::Exception& e)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MeshEditComponent] cancelEdit: restore failed: " + e.getDescription());
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "MeshEditComponent::cancelEdit");

        // Remove this component from the GameObject — fires onRemoveComponent but
        // editableItem is now nullptr so nothing bad happens there.
        const unsigned int myIndex = this->getIndex();
        boost::shared_ptr<EventDataDeleteComponent> evt(new EventDataDeleteComponent(this->gameObjectPtr->getId(), MeshEditComponent::getStaticClassName(), myIndex));
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(evt);
        this->gameObjectPtr->deleteComponentByIndex(myIndex);
    }

    // =========================================================================
    //  actualizeValue
    // =========================================================================

    void MeshEditComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);

        if (AttrActivated() == attribute->getName())
        {
            this->setActivated(attribute->getBool());
        }
        else if (AttrEditMode() == attribute->getName())
        {
            this->setEditMode(attribute->getListSelectedValue());
        }
        else if (AttrShowWireframe() == attribute->getName())
        {
            this->showWireframe->setValue(attribute->getBool());
            this->scheduleOverlayUpdate();
        }
        else if (AttrXRayOverlay() == attribute->getName())
        {
            this->xRayOverlay->setValue(attribute->getBool());
            this->scheduleOverlayUpdate();
        }
        else if (AttrVertexMarkerSize() == attribute->getName())
        {
            this->vertexMarkerSize->setValue(attribute->getReal());
            this->scheduleOverlayUpdate();
        }
        else if (AttrOutputFileName() == attribute->getName())
        {
            this->setOutputFileName(attribute->getString());
        }
        else if (AttrBrushName() == attribute->getName())
        {
            this->setBrushName(attribute->getListSelectedValue());
        }
        else if (AttrBrushSize() == attribute->getName())
        {
            this->setBrushSize(attribute->getReal());
        }
        else if (AttrBrushIntensity() == attribute->getName())
        {
            this->setBrushIntensity(attribute->getReal());
        }
        else if (AttrBrushFalloff() == attribute->getName())
        {
            this->setBrushFalloff(attribute->getReal());
        }
        else if (AttrBrushMode() == attribute->getName())
        {
            this->setBrushMode(attribute->getListSelectedValue());
        }
    }

    // =========================================================================
    //  writeXML
    // =========================================================================

    void MeshEditComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
    {
        GameObjectComponent::writeXML(propertiesXML, doc);

        auto add = [&](const char* type, const Ogre::String& nm, const Ogre::String& data)
        {
            xml_node<>* n = doc.allocate_node(node_element, "property");
            n->append_attribute(doc.allocate_attribute("type", type));
            n->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, nm)));
            n->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, data)));
            propertiesXML->append_node(n);
        };

        add("12", AttrActivated(), XMLConverter::ConvertString(doc, this->activated->getBool()));
        add("7", AttrEditMode(), XMLConverter::ConvertString(doc, this->editMode->getListSelectedValue()));
        add("12", AttrShowWireframe(), XMLConverter::ConvertString(doc, this->showWireframe->getBool()));
        add("12", AttrXRayOverlay(), XMLConverter::ConvertString(doc, this->xRayOverlay->getBool()));
        add("6", AttrVertexMarkerSize(), XMLConverter::ConvertString(doc, this->vertexMarkerSize->getReal()));
        add("7", AttrOutputFileName(), XMLConverter::ConvertString(doc, this->outputFileName->getString()));
        add("7", AttrBrushName(), XMLConverter::ConvertString(doc, this->brushName->getListSelectedValue()));
        add("6", AttrBrushSize(), XMLConverter::ConvertString(doc, this->brushSize->getReal()));
        add("6", AttrBrushIntensity(), XMLConverter::ConvertString(doc, this->brushIntensity->getReal()));
        add("6", AttrBrushFalloff(), XMLConverter::ConvertString(doc, this->brushFalloff->getReal()));
        add("7", AttrBrushMode(), XMLConverter::ConvertString(doc, this->brushMode->getListSelectedValue()));
    }

    // =========================================================================
    //  Trivial accessors
    // =========================================================================

    Ogre::String MeshEditComponent::getClassName(void) const
    {
        return "MeshEditComponent";
    }
    Ogre::String MeshEditComponent::getParentClassName(void) const
    {
        return "GameObjectComponent";
    }

    void MeshEditComponent::setActivated(bool v)
    {
        this->activated->setValue(v);
        this->updateModificationState();
    }
    bool MeshEditComponent::isActivated(void) const
    {
        return this->activated->getBool();
    }

    // =========================================================================
    //  Edit mode
    // =========================================================================

    void MeshEditComponent::setEditMode(const Ogre::String& mode)
    {
        this->isBrushArmed = false;

        this->editMode->setListSelectedValue(mode);

        if (this->isGrabbing)
        {
            this->cancelGrab();
        }
        this->deselectAll();

        const bool show = (mode != "Object");
        NOWA::GraphicsModule::RenderCommand cmd = [this, show]()
        {
            if (this->overlayNode)
            {
                this->overlayNode->setVisible(show);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueue(std::move(cmd), "MeshEditComponent::setEditMode");
        if (show)
        {
            this->scheduleOverlayUpdate();
        }
    }

    Ogre::String MeshEditComponent::getEditModeString(void) const
    {
        return this->editMode->getListSelectedValue();
    }

    MeshEditComponent::EditMode MeshEditComponent::getEditMode(void) const
    {
        const Ogre::String& m = this->editMode->getListSelectedValue();
        if (m == "Vertex")
        {
            return EditMode::VERTEX;
        }
        if (m == "Edge")
        {
            return EditMode::EDGE;
        }
        if (m == "Face")
        {
            return EditMode::FACE;
        }
        return EditMode::OBJECT;
    }

    // =========================================================================
    //  Selection
    // =========================================================================

    void MeshEditComponent::selectAll(void)
    {
        switch (this->getEditMode())
        {
        case EditMode::VERTEX:
            for (size_t i = 0; i < this->vertexCount; ++i)
            {
                this->selectedVertices.insert(i);
            }
            break;
        case EditMode::EDGE:
            for (size_t i = 0; i < this->indexCount; i += 3)
            {
                for (int e = 0; e < 3; ++e)
                {
                    this->selectedEdges.insert(EdgeKey(this->indices[i + e], this->indices[i + (e + 1) % 3]));
                }
            }
            break;
        case EditMode::FACE:
            for (size_t i = 0; i < this->indexCount / 3; ++i)
            {
                this->selectedFaces.insert(i);
            }
            break;
        default:
            break;
        }
        this->scheduleOverlayUpdate();
    }

    void MeshEditComponent::deselectAll(void)
    {
        this->selectedVertices.clear();
        this->selectedEdges.clear();
        this->selectedFaces.clear();
        this->scheduleOverlayUpdate();
    }

    void MeshEditComponent::selectVertex(size_t idx, bool add)
    {
        if (!add)
        {
            this->selectedVertices.clear();
        }
        this->selectedVertices.insert(idx);
        this->scheduleOverlayUpdate();
    }

    void MeshEditComponent::selectEdge(size_t a, size_t b, bool add)
    {
        if (!add)
        {
            this->selectedEdges.clear();
        }
        this->selectedEdges.insert(EdgeKey(a, b));
        this->scheduleOverlayUpdate();
    }

    void MeshEditComponent::selectFace(size_t faceIdx, bool add)
    {
        if (!add)
        {
            this->selectedFaces.clear();
        }
        this->selectedFaces.insert(faceIdx);
        this->scheduleOverlayUpdate();
    }

    const std::set<size_t>& MeshEditComponent::getSelectedVertices(void) const
    {
        return this->selectedVertices;
    }
    const std::set<MeshEditComponent::EdgeKey>& MeshEditComponent::getSelectedEdges(void) const
    {
        return this->selectedEdges;
    }
    const std::set<size_t>& MeshEditComponent::getSelectedFaces(void) const
    {
        return this->selectedFaces;
    }

    // =========================================================================
    //  Delete
    // =========================================================================

    void MeshEditComponent::deleteSelected(void)
    {
        switch (this->getEditMode())
        {
        case EditMode::VERTEX:
            this->deleteSelectedVertices();
            break;
        case EditMode::EDGE:
            this->deleteSelectedEdges();
            break;
        case EditMode::FACE:
            this->deleteSelectedFaces();
            break;
        default:
            break;
        }
    }

    void MeshEditComponent::deleteSelectedVertices(void)
    {
        if (this->selectedVertices.empty())
        {
            return;
        }
        this->pushUndoSnapshot();

        // Remove all faces that touch any deleted vertex
        std::vector<Ogre::uint32> newIdx;
        for (size_t i = 0; i < this->indexCount; i += 3)
        {
            if (this->selectedVertices.count(this->indices[i]) || this->selectedVertices.count(this->indices[i + 1]) || this->selectedVertices.count(this->indices[i + 2]))
            {
                continue;
            }
            newIdx.push_back(this->indices[i]);
            newIdx.push_back(this->indices[i + 1]);
            newIdx.push_back(this->indices[i + 2]);
        }

        // Remap
        std::vector<int32_t> remap(this->vertexCount, -1);
        std::vector<Ogre::Vector3> nv, nn;
        std::vector<Ogre::Vector4> nt;
        std::vector<Ogre::Vector2> nu;
        size_t ni = 0;
        for (size_t i = 0; i < this->vertexCount; ++i)
        {
            if (!this->selectedVertices.count(i))
            {
                remap[i] = static_cast<int32_t>(ni++);
                nv.push_back(this->vertices[i]);
                nn.push_back(this->normals[i]);
                nt.push_back(this->tangents[i]);
                nu.push_back(this->uvCoordinates[i]);
            }
        }
        for (auto& idx : newIdx)
        {
            idx = static_cast<Ogre::uint32>(remap[idx]);
        }

        this->vertices = std::move(nv);
        this->normals = std::move(nn);
        this->tangents = std::move(nt);
        this->uvCoordinates = std::move(nu);
        this->indices = std::move(newIdx);
        this->vertexCount = this->vertices.size();
        this->indexCount = this->indices.size();
        this->selectedVertices.clear();

        this->buildVertexAdjacency();
        this->rebuildDynamicBuffers();
        this->scheduleOverlayUpdate();
    }

#if 1
    // To aggressive deletion
    void MeshEditComponent::deleteSelectedEdges(void)
    {
        if (this->selectedEdges.empty())
        {
            return;
        }
        this->pushUndoSnapshot();

        std::vector<Ogre::uint32> newIdx;
        for (size_t i = 0; i < this->indexCount; i += 3)
        {
            EdgeKey e01(this->indices[i], this->indices[i + 1]);
            EdgeKey e12(this->indices[i + 1], this->indices[i + 2]);
            EdgeKey e20(this->indices[i + 2], this->indices[i]);
            if (this->selectedEdges.count(e01) || this->selectedEdges.count(e12) || this->selectedEdges.count(e20))
            {
                continue;
            }
            newIdx.push_back(this->indices[i]);
            newIdx.push_back(this->indices[i + 1]);
            newIdx.push_back(this->indices[i + 2]);
        }
        this->indices = std::move(newIdx);
        this->indexCount = this->indices.size();
        this->selectedEdges.clear();

        this->buildVertexAdjacency();
        this->rebuildDynamicBuffers();
        this->scheduleOverlayUpdate();
    }
#else
    void MeshEditComponent::deleteSelectedEdges(void)
    {
        if (this->selectedEdges.empty())
        {
            return;
        }
        this->pushUndoSnapshot();

        std::vector<Ogre::uint32> newIdx;
        for (size_t i = 0; i < this->indexCount; i += 3)
        {
            EdgeKey e01(this->indices[i], this->indices[i + 1]);
            EdgeKey e12(this->indices[i + 1], this->indices[i + 2]);
            EdgeKey e20(this->indices[i + 2], this->indices[i]);

            // Only remove a face if ALL its edges are selected
            // (selecting 3 edges of a triangle → only that triangle deleted)
            // (selecting 1 edge → adjacent faces preserved)
            bool allSelected = this->selectedEdges.count(e01) && this->selectedEdges.count(e12) && this->selectedEdges.count(e20);
            if (allSelected)
            {
                continue;
            }

            newIdx.push_back(this->indices[i]);
            newIdx.push_back(this->indices[i + 1]);
            newIdx.push_back(this->indices[i + 2]);
        }
        this->indices = std::move(newIdx);
        this->indexCount = this->indices.size();
        this->selectedEdges.clear();
        this->buildVertexAdjacency();
        this->rebuildDynamicBuffers();
        this->scheduleOverlayUpdate();
    }
#endif

    void MeshEditComponent::deleteSelectedFaces(void)
    {
        if (this->selectedFaces.empty())
        {
            return;
        }
        this->pushUndoSnapshot();

        std::vector<Ogre::uint32> newIdx;
        const size_t triCount = this->indexCount / 3;
        for (size_t t = 0; t < triCount; ++t)
        {
            if (this->selectedFaces.count(t))
            {
                continue;
            }
            newIdx.push_back(this->indices[t * 3]);
            newIdx.push_back(this->indices[t * 3 + 1]);
            newIdx.push_back(this->indices[t * 3 + 2]);
        }
        this->indices = std::move(newIdx);
        this->indexCount = this->indices.size();
        this->selectedFaces.clear();

        this->buildVertexAdjacency();
        this->rebuildDynamicBuffers();
        this->scheduleOverlayUpdate();
    }

    // =========================================================================
    //  Move
    // =========================================================================

    void MeshEditComponent::moveSelected(const Ogre::Vector3& localDelta)
    {
        std::unordered_set<size_t> toMove;
        for (size_t i : this->selectedVertices)
        {
            toMove.insert(i);
        }
        for (const EdgeKey& e : this->selectedEdges)
        {
            toMove.insert(e.a);
            toMove.insert(e.b);
        }
        for (size_t t : this->selectedFaces)
        {
            toMove.insert(this->indices[t * 3]);
            toMove.insert(this->indices[t * 3 + 1]);
            toMove.insert(this->indices[t * 3 + 2]);
        }
        if (toMove.empty())
        {
            return;
        }

        for (size_t i : toMove)
        {
            this->vertices[i] += localDelta;
        }

        this->recalculateNormals_internal();
        this->recalculateTangents();

        NOWA::GraphicsModule::RenderCommand cmd = [this]()
        {
            this->uploadVertexData();
        };
        NOWA::GraphicsModule::getInstance()->enqueue(std::move(cmd), "MeshEditComponent::moveSelected");
        this->scheduleOverlayUpdate();
    }

    // =========================================================================
    //  Extra operations
    // =========================================================================

    void MeshEditComponent::mergeByDistance(Ogre::Real threshold)
    {
        this->pushUndoSnapshot();
        std::vector<size_t> remap(this->vertexCount);
        std::iota(remap.begin(), remap.end(), 0);
        for (size_t i = 0; i < this->vertexCount; ++i)
        {
            for (size_t j = 0; j < i; ++j)
            {
                if (this->vertices[i].distance(this->vertices[j]) <= threshold)
                {
                    remap[i] = remap[j];
                    break;
                }
            }
        }

        std::unordered_map<size_t, size_t> map;
        std::vector<Ogre::Vector3> nv, nn;
        std::vector<Ogre::Vector4> nt;
        std::vector<Ogre::Vector2> nu;
        std::vector<size_t> fr(this->vertexCount);
        for (size_t i = 0; i < this->vertexCount; ++i)
        {
            size_t rep = remap[i];
            auto it = map.find(rep);
            if (it == map.end())
            {
                fr[i] = nv.size();
                map[rep] = nv.size();
                nv.push_back(this->vertices[rep]);
                nn.push_back(this->normals[rep]);
                nt.push_back(this->tangents[rep]);
                nu.push_back(this->uvCoordinates[rep]);
            }
            else
            {
                fr[i] = it->second;
            }
        }
        for (auto& idx : this->indices)
        {
            idx = static_cast<Ogre::uint32>(fr[idx]);
        }
        this->vertices = std::move(nv);
        this->normals = std::move(nn);
        this->tangents = std::move(nt);
        this->uvCoordinates = std::move(nu);
        this->vertexCount = this->vertices.size();
        this->deselectAll();
        this->buildVertexAdjacency();
        this->rebuildDynamicBuffers();
        this->scheduleOverlayUpdate();
    }

    void MeshEditComponent::flipNormals(void)
    {
        this->pushUndoSnapshot();
        for (auto& n : this->normals)
        {
            n = -n;
        }
        for (size_t i = 0; i < this->indexCount; i += 3)
        {
            std::swap(this->indices[i + 1], this->indices[i + 2]);
        }
        NOWA::GraphicsModule::RenderCommand cmd = [this]()
        {
            this->uploadVertexData();
        };
        NOWA::GraphicsModule::getInstance()->enqueue(std::move(cmd), "MeshEditComponent::flipNormals");
    }

    void MeshEditComponent::recalculateNormals(void)
    {
        this->recalculateNormals_internal();
        NOWA::GraphicsModule::RenderCommand cmd = [this]()
        {
            this->uploadVertexData();
        };
        NOWA::GraphicsModule::getInstance()->enqueue(std::move(cmd), "MeshEditComponent::recalcNormals");
    }

    void MeshEditComponent::subdivideSelectedFaces(void)
    {
        if (this->selectedFaces.empty())
        {
            return;
        }
        this->pushUndoSnapshot();

        std::vector<Ogre::uint32> newIdx;
        std::vector<Ogre::Vector3> nv = this->vertices, nn = this->normals;
        std::vector<Ogre::Vector4> nt = this->tangents;
        std::vector<Ogre::Vector2> nu = this->uvCoordinates;

        auto addMid = [&](size_t a, size_t b) -> size_t
        {
            size_t m = nv.size();
            nv.push_back((this->vertices[a] + this->vertices[b]) * 0.5f);
            nn.push_back(((this->normals[a] + this->normals[b]) * 0.5f).normalisedCopy());
            nt.push_back((this->tangents[a] + this->tangents[b]) * 0.5f);
            nu.push_back((this->uvCoordinates[a] + this->uvCoordinates[b]) * 0.5f);
            return m;
        };

        const size_t triCount = this->indexCount / 3;
        for (size_t t = 0; t < triCount; ++t)
        {
            size_t i0 = this->indices[t * 3], i1 = this->indices[t * 3 + 1], i2 = this->indices[t * 3 + 2];
            if (!this->selectedFaces.count(t))
            {
                newIdx.push_back(static_cast<Ogre::uint32>(i0));
                newIdx.push_back(static_cast<Ogre::uint32>(i1));
                newIdx.push_back(static_cast<Ogre::uint32>(i2));
                continue;
            }
            size_t m01 = addMid(i0, i1), m12 = addMid(i1, i2), m20 = addMid(i2, i0);
            newIdx.push_back(static_cast<Ogre::uint32>(i0));
            newIdx.push_back(static_cast<Ogre::uint32>(m01));
            newIdx.push_back(static_cast<Ogre::uint32>(m20));
            newIdx.push_back(static_cast<Ogre::uint32>(m01));
            newIdx.push_back(static_cast<Ogre::uint32>(i1));
            newIdx.push_back(static_cast<Ogre::uint32>(m12));
            newIdx.push_back(static_cast<Ogre::uint32>(m20));
            newIdx.push_back(static_cast<Ogre::uint32>(m12));
            newIdx.push_back(static_cast<Ogre::uint32>(i2));
            newIdx.push_back(static_cast<Ogre::uint32>(m01));
            newIdx.push_back(static_cast<Ogre::uint32>(m12));
            newIdx.push_back(static_cast<Ogre::uint32>(m20));
        }
        this->vertices = std::move(nv);
        this->normals = std::move(nn);
        this->tangents = std::move(nt);
        this->uvCoordinates = std::move(nu);
        this->indices = std::move(newIdx);
        this->vertexCount = this->vertices.size();
        this->indexCount = this->indices.size();
        this->selectedFaces.clear();
        this->buildVertexAdjacency();
        this->rebuildDynamicBuffers();
        this->scheduleOverlayUpdate();
    }

    void MeshEditComponent::extrudeSelectedFaces(Ogre::Real amount)
    {
        if (this->selectedFaces.empty())
        {
            return;
        }
        this->pushUndoSnapshot();

        std::vector<Ogre::uint32> newIdx(this->indices.begin(), this->indices.end());
        std::vector<Ogre::Vector3> nv = this->vertices, nn = this->normals;
        std::vector<Ogre::Vector4> nt = this->tangents;
        std::vector<Ogre::Vector2> nu = this->uvCoordinates;

        for (size_t t : this->selectedFaces)
        {
            size_t i0 = this->indices[t * 3], i1 = this->indices[t * 3 + 1], i2 = this->indices[t * 3 + 2];
            Ogre::Vector3 fn = ((this->vertices[i1] - this->vertices[i0]).crossProduct(this->vertices[i2] - this->vertices[i0])).normalisedCopy();
            Ogre::Vector3 off = fn * amount;

            size_t n0 = nv.size();
            nv.push_back(this->vertices[i0] + off);
            nn.push_back(fn);
            nt.push_back(this->tangents[i0]);
            nu.push_back(this->uvCoordinates[i0]);
            size_t n1 = nv.size();
            nv.push_back(this->vertices[i1] + off);
            nn.push_back(fn);
            nt.push_back(this->tangents[i1]);
            nu.push_back(this->uvCoordinates[i1]);
            size_t n2 = nv.size();
            nv.push_back(this->vertices[i2] + off);
            nn.push_back(fn);
            nt.push_back(this->tangents[i2]);
            nu.push_back(this->uvCoordinates[i2]);

            newIdx.push_back(static_cast<Ogre::uint32>(n0));
            newIdx.push_back(static_cast<Ogre::uint32>(n1));
            newIdx.push_back(static_cast<Ogre::uint32>(n2));

            auto addSide = [&](size_t a, size_t b, size_t na, size_t nb)
            {
                newIdx.push_back(static_cast<Ogre::uint32>(a));
                newIdx.push_back(static_cast<Ogre::uint32>(b));
                newIdx.push_back(static_cast<Ogre::uint32>(na));
                newIdx.push_back(static_cast<Ogre::uint32>(b));
                newIdx.push_back(static_cast<Ogre::uint32>(nb));
                newIdx.push_back(static_cast<Ogre::uint32>(na));
            };
            addSide(i0, i1, n0, n1);
            addSide(i1, i2, n1, n2);
            addSide(i2, i0, n2, n0);
        }
        this->vertices = std::move(nv);
        this->normals = std::move(nn);
        this->tangents = std::move(nt);
        this->uvCoordinates = std::move(nu);
        this->indices = std::move(newIdx);
        this->vertexCount = this->vertices.size();
        this->indexCount = this->indices.size();
        this->selectedFaces.clear();
        this->buildVertexAdjacency();
        this->rebuildDynamicBuffers();
        this->scheduleOverlayUpdate();
    }

    // =========================================================================
    //  Undo
    // =========================================================================

    void MeshEditComponent::pushUndoSnapshot(void)
    {
        MeshSnapshot s;
        s.vertices = this->vertices;
        s.normals = this->normals;
        s.tangents = this->tangents;
        s.uvCoordinates = this->uvCoordinates;
        s.indices = this->indices;
        this->undoStack.push_back(std::move(s));
        if (this->undoStack.size() > MAX_UNDO_LEVELS)
        {
            this->undoStack.erase(this->undoStack.begin());
        }
    }

    void MeshEditComponent::undo(void)
    {
        if (this->undoStack.empty())
        {
            return;
        }
        MeshSnapshot& s = this->undoStack.back();
        this->vertices = s.vertices;
        this->normals = s.normals;
        this->tangents = s.tangents;
        this->uvCoordinates = s.uvCoordinates;
        this->indices = s.indices;
        this->vertexCount = this->vertices.size();
        this->indexCount = this->indices.size();
        this->undoStack.pop_back();
        this->deselectAll();
        this->buildVertexAdjacency();
        this->rebuildDynamicBuffers();
        this->scheduleOverlayUpdate();
    }

    bool MeshEditComponent::canUndo(void) const
    {
        return !this->undoStack.empty();
    }

    // =========================================================================
    //  Brush
    // =========================================================================

    void MeshEditComponent::setBrushName(const Ogre::String& name)
    {
        this->brushName->setListSelectedValue(name);
        this->brushData.clear();
        this->brushImageWidth = 0;
        this->brushImageHeight = 0;
        if (name.empty() || name == "Default (Smooth)")
        {
            return;
        }
        try
        {
            Ogre::Image2 img;
            img.load(name, "Brushes");
            this->brushImageWidth = img.getWidth();
            this->brushImageHeight = img.getHeight();
            this->brushData.resize(this->brushImageWidth * this->brushImageHeight, 0.0f);
            for (Ogre::uint32 y = 0; y < this->brushImageHeight; ++y)
            {
                for (Ogre::uint32 x = 0; x < this->brushImageWidth; ++x)
                {
                    this->brushData[y * this->brushImageWidth + x] = img.getColourAt(x, y, 0).r;
                }
            }
        }
        catch (Ogre::Exception& e)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MeshEditComponent] Brush load failed: " + e.getDescription());
        }
    }

    Ogre::String MeshEditComponent::getBrushName(void) const
    {
        return this->brushName->getListSelectedValue();
    }
    void MeshEditComponent::setBrushSize(Ogre::Real v)
    {
        this->brushSize->setValue(v);
    }
    Ogre::Real MeshEditComponent::getBrushSize(void) const
    {
        return this->brushSize->getReal();
    }
    void MeshEditComponent::setBrushIntensity(Ogre::Real v)
    {
        this->brushIntensity->setValue(v);
    }
    Ogre::Real MeshEditComponent::getBrushIntensity(void) const
    {
        return this->brushIntensity->getReal();
    }
    void MeshEditComponent::setBrushFalloff(Ogre::Real v)
    {
        this->brushFalloff->setValue(v);
    }
    Ogre::Real MeshEditComponent::getBrushFalloff(void) const
    {
        return this->brushFalloff->getReal();
    }
    void MeshEditComponent::setBrushMode(const Ogre::String& m)
    {
        this->brushMode->setListSelectedValue(m);
    }
    Ogre::String MeshEditComponent::getBrushModeString(void) const
    {
        return this->brushMode->getListSelectedValue();
    }
    MeshEditComponent::BrushMode MeshEditComponent::getBrushMode(void) const
    {
        const Ogre::String& m = this->brushMode->getListSelectedValue();
        if (m == "Pull")
        {
            return BrushMode::PULL;
        }
        if (m == "Smooth")
        {
            return BrushMode::SMOOTH;
        }
        if (m == "Flatten")
        {
            return BrushMode::FLATTEN;
        }
        if (m == "Pinch")
        {
            return BrushMode::PINCH;
        }
        if (m == "Inflate")
        {
            return BrushMode::INFLATE;
        }
        return BrushMode::PUSH;
    }

    // =========================================================================
    //  Output / Apply
    // =========================================================================

    Ogre::String MeshEditComponent::buildDefaultOutputName(void) const
    {
        Ogre::String base = this->originalMeshName;
        size_t dot = base.rfind('.');
        if (dot != Ogre::String::npos)
        {
            base = base.substr(0, dot);
        }
        return base + "_edit";
    }

    void MeshEditComponent::setOutputFileName(const Ogre::String& name)
    {
        this->outputFileName->setValue(name);
    }

    Ogre::String MeshEditComponent::getOutputFileName(void) const
    {
        return this->outputFileName->getString();
    }

    bool MeshEditComponent::exportMesh(const Ogre::String& fileNameOverride)
    {
        if (!this->editableMesh)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MeshEditComponent] exportMesh: no editable mesh.");
            return false;
        }

        Ogre::String outName = fileNameOverride.empty() ? this->outputFileName->getString() : fileNameOverride;
        if (outName.empty())
        {
            outName = this->buildDefaultOutputName();
        }

        // Safety: never overwrite the original mesh
        {
            Ogre::String base = this->originalMeshName;
            size_t dot = base.rfind('.');
            if (dot != Ogre::String::npos)
            {
                base = base.substr(0, dot);
            }
            if (outName == base || outName == this->originalMeshName)
            {
                outName = base + "_edit";
            }
        }

        // Avoid collision with already-loaded mesh resources
        Ogre::String finalName = outName;
        {
            int counter = 1;
            while (Ogre::MeshManager::getSingleton().resourceExists(finalName + ".mesh") && counter < 1000)
            {
                finalName = outName + "_" + Ogre::StringConverter::toString(counter++);
            }
        }

        // Full path to Procedural folder
        auto filePathNames = Core::getSingletonPtr()->getSectionPath("Procedural");

        if (true == filePathNames.empty())
        {
            return false;
        }
        Ogre::String fullPath = filePathNames[0] + "/" + finalName + ".mesh";

        bool success = false;

        NOWA::GraphicsModule::RenderCommand cmd = [this, fullPath, &success]()
        {
            try
            {
                Ogre::MeshSerializer serializer(Ogre::Root::getSingletonPtr()->getRenderSystem()->getVaoManager());
                serializer.exportMesh(this->editableMesh.get(), fullPath, Ogre::MESH_VERSION_LATEST, Ogre::Serializer::ENDIAN_NATIVE);
                success = true;
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MeshEditComponent] Exported mesh to: " + fullPath);
            }
            catch (Ogre::Exception& e)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MeshEditComponent] exportMesh failed: " + e.getDescription());
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "MeshEditComponent::exportMesh");
        return success;
    }

    bool MeshEditComponent::applyMesh(void)
    {
        return this->exportMesh();
    }

    // =========================================================================
    //  Info
    // =========================================================================

    size_t MeshEditComponent::getVertexCount(void) const
    {
        return this->vertexCount;
    }
    size_t MeshEditComponent::getIndexCount(void) const
    {
        return this->indexCount;
    }
    size_t MeshEditComponent::getTriangleCount(void) const
    {
        return this->indexCount / 3;
    }

    // =========================================================================
    //  Mesh preparation
    // =========================================================================

    bool MeshEditComponent::prepareEditableMesh(void)
    {
        bool success = false;
        NOWA::GraphicsModule::RenderCommand cmd = [this, &success]()
        {
            if (!this->extractMeshData())
            {
                return;
            }
            this->buildVertexAdjacency();
            if (!this->createDynamicBuffers())
            {
                return;
            }
            success = true;
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "MeshEditComponent::prepareEditableMesh");
        return success;
    }

    bool MeshEditComponent::detectVertexFormat(Ogre::VertexArrayObject* vao)
    {
        this->vertexFormat = VertexElementInfo();
        for (const Ogre::VertexBufferPacked* vb : vao->getVertexBuffers())
        {
            for (const Ogre::VertexElement2& e : vb->getVertexElements())
            {
                switch (e.mSemantic)
                {
                case Ogre::VES_POSITION:
                    this->vertexFormat.hasPosition = true;
                    this->vertexFormat.positionType = e.mType;
                    break;
                case Ogre::VES_NORMAL:
                    this->vertexFormat.hasNormal = true;
                    this->vertexFormat.normalType = e.mType;
                    if (e.mType == Ogre::VET_SHORT4_SNORM)
                    {
                        this->vertexFormat.hasQTangent = true;
                    }
                    break;
                case Ogre::VES_TANGENT:
                    this->vertexFormat.hasTangent = true;
                    this->vertexFormat.tangentType = e.mType;
                    if (e.mType == Ogre::VET_SHORT4_SNORM)
                    {
                        this->vertexFormat.hasQTangent = true;
                    }
                    break;
                case Ogre::VES_TEXTURE_COORDINATES:
                    this->vertexFormat.hasUV = true;
                    this->vertexFormat.uvType = e.mType;
                    break;
                default:
                    break;
                }
            }
        }
        return this->vertexFormat.hasPosition;
    }

    bool MeshEditComponent::extractMeshData(void)
    {
        Ogre::Item* item = this->gameObjectPtr->getMovableObject<Ogre::Item>();
        if (!item)
        {
            return false;
        }
        Ogre::MeshPtr mesh = item->getMesh();
        if (!mesh)
        {
            return false;
        }

        unsigned int numV = 0, numI = 0;
        for (const auto& sm : mesh->getSubMeshes())
        {
            if (sm->mVao[Ogre::VpNormal].empty())
            {
                continue;
            }
            numV += static_cast<unsigned int>(sm->mVao[Ogre::VpNormal][0]->getVertexBuffers()[0]->getNumElements());
            if (sm->mVao[Ogre::VpNormal][0]->getIndexBuffer())
            {
                numI += static_cast<unsigned int>(sm->mVao[Ogre::VpNormal][0]->getIndexBuffer()->getNumElements());
            }
        }
        if (numV == 0)
        {
            return false;
        }

        this->vertexCount = numV;
        this->indexCount = numI;
        this->vertices.resize(numV);
        this->normals.resize(numV);
        this->tangents.resize(numV, Ogre::Vector4(1, 0, 0, 1));
        this->uvCoordinates.resize(numV);
        this->indices.resize(numI);
        this->subMeshInfoList.clear();

        unsigned int addedI = 0, iOff = 0, vOff = 0;
        for (const auto& sm : mesh->getSubMeshes())
        {
            if (sm->mVao[Ogre::VpNormal].empty())
            {
                continue;
            }
            Ogre::VertexArrayObject* vao = sm->mVao[Ogre::VpNormal][0];
            if (vao->getIndexBuffer())
            {
                this->isIndices32 = (vao->getIndexBuffer()->getIndexType() == Ogre::IndexBufferPacked::IT_32BIT);
            }
            this->detectVertexFormat(vao);

            const size_t smVS = vOff, smIS = addedI;
            Ogre::VertexArrayObject::ReadRequestsVec reqs;
            reqs.push_back(Ogre::VertexArrayObject::ReadRequests(Ogre::VES_POSITION));
            reqs.push_back(Ogre::VertexArrayObject::ReadRequests(Ogre::VES_NORMAL));
            reqs.push_back(Ogre::VertexArrayObject::ReadRequests(Ogre::VES_TEXTURE_COORDINATES));
            vao->readRequests(reqs);
            vao->mapAsyncTickets(reqs);

            unsigned int smVC = static_cast<unsigned int>(reqs[0].vertexBuffer->getNumElements());
            bool isQT = (reqs[1].type == Ogre::VET_SHORT4_SNORM);

            for (size_t i = 0; i < smVC; ++i)
            {
                const size_t gi = vOff + i;
                if (reqs[0].type == Ogre::VET_HALF4)
                {
                    const Ogre::uint16* p = reinterpret_cast<const Ogre::uint16*>(reqs[0].data);
                    this->vertices[gi] = {Ogre::Bitwise::halfToFloat(p[0]), Ogre::Bitwise::halfToFloat(p[1]), Ogre::Bitwise::halfToFloat(p[2])};
                }
                else
                {
                    const float* p = reinterpret_cast<const float*>(reqs[0].data);
                    this->vertices[gi] = {p[0], p[1], p[2]};
                }

                if (isQT)
                {
                    const Ogre::int16* p = reinterpret_cast<const Ogre::int16*>(reqs[1].data);
                    Ogre::Quaternion q;
                    q.x = p[0] / 32767.0f;
                    q.y = p[1] / 32767.0f;
                    q.z = p[2] / 32767.0f;
                    q.w = p[3] / 32767.0f;
                    float r = (q.w < 0) ? -1.0f : 1.0f;
                    this->normals[gi] = q.xAxis();
                    this->tangents[gi] = {q.yAxis().x, q.yAxis().y, q.yAxis().z, r};
                    this->vertexFormat.hasTangent = true;
                }
                else if (reqs[1].type == Ogre::VET_HALF4)
                {
                    const Ogre::uint16* p = reinterpret_cast<const Ogre::uint16*>(reqs[1].data);
                    this->normals[gi] = {Ogre::Bitwise::halfToFloat(p[0]), Ogre::Bitwise::halfToFloat(p[1]), Ogre::Bitwise::halfToFloat(p[2])};
                }
                else
                {
                    const float* p = reinterpret_cast<const float*>(reqs[1].data);
                    this->normals[gi] = {p[0], p[1], p[2]};
                }

                if (reqs[2].type == Ogre::VET_HALF2)
                {
                    const Ogre::uint16* p = reinterpret_cast<const Ogre::uint16*>(reqs[2].data);
                    this->uvCoordinates[gi] = {Ogre::Bitwise::halfToFloat(p[0]), Ogre::Bitwise::halfToFloat(p[1])};
                }
                else
                {
                    const float* p = reinterpret_cast<const float*>(reqs[2].data);
                    this->uvCoordinates[gi] = {p[0], p[1]};
                }

                reqs[0].data += reqs[0].vertexBuffer->getBytesPerElement();
                reqs[1].data += reqs[1].vertexBuffer->getBytesPerElement();
                reqs[2].data += reqs[2].vertexBuffer->getBytesPerElement();
            }
            vOff += smVC;
            vao->unmapAsyncTickets(reqs);

            Ogre::IndexBufferPacked* ib = vao->getIndexBuffer();
            if (ib)
            {
                const void* shadow = ib->getShadowCopy();
                auto readIdx = [&](const void* data)
                {
                    if (this->isIndices32)
                    {
                        const Ogre::uint32* p = reinterpret_cast<const Ogre::uint32*>(data);
                        for (size_t i = 0; i < ib->getNumElements(); ++i)
                        {
                            this->indices[addedI++] = p[i] + iOff;
                        }
                    }
                    else
                    {
                        const Ogre::uint16* p = reinterpret_cast<const Ogre::uint16*>(data);
                        for (size_t i = 0; i < ib->getNumElements(); ++i)
                        {
                            this->indices[addedI++] = static_cast<Ogre::uint32>(p[i]) + iOff;
                        }
                    }
                };
                if (shadow)
                {
                    readIdx(shadow);
                }
                else
                {
                    Ogre::AsyncTicketPtr t = ib->readRequest(0, ib->getNumElements());
                    readIdx(t->map());
                    t->unmap();
                }
            }
            iOff += smVC;

            SubMeshInfo si;
            si.vertexOffset = smVS;
            si.vertexCount = smVC;
            si.indexOffset = smIS;
            si.indexCount = ib ? ib->getNumElements() : 0;
            si.hasTangent = this->vertexFormat.hasTangent;
            si.floatsPerVertex = si.hasTangent ? 12u : 8u;
            this->subMeshInfoList.push_back(si);
        }
        return true;
    }

    bool MeshEditComponent::createDynamicBuffers(void)
    {
        Ogre::VaoManager* vm = Ogre::Root::getSingletonPtr()->getRenderSystem()->getVaoManager();
        Ogre::Item* srcItem = this->gameObjectPtr->getMovableObject<Ogre::Item>();

        Ogre::String meshName = this->gameObjectPtr->getName() + "_MeshEdit_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
        this->editableMesh = Ogre::MeshManager::getSingleton().createManual(meshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
        this->editableMesh->_setVaoManager(vm);

        Ogre::Vector3 minBB(std::numeric_limits<float>::max());
        Ogre::Vector3 maxBB(-std::numeric_limits<float>::max());

        for (size_t smIdx = 0; smIdx < this->subMeshInfoList.size(); ++smIdx)
        {
            SubMeshInfo& si = this->subMeshInfoList[smIdx];
            bool needsTangent = false;
            if (srcItem && smIdx < srcItem->getNumSubItems())
            {
                auto* pb = dynamic_cast<Ogre::HlmsPbsDatablock*>(srcItem->getSubItem(smIdx)->getDatablock());
                if (pb && pb->getTexture(Ogre::PBSM_NORMAL))
                {
                    needsTangent = true;
                }
            }
            bool incTan = si.hasTangent || needsTangent;
            if (incTan && !si.hasTangent)
            {
                si.hasTangent = true;
                this->vertexFormat.hasTangent = true;
                this->recalculateTangents();
            }
            si.floatsPerVertex = incTan ? 12u : 8u;

            Ogre::VertexElement2Vec elems;
            elems.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_POSITION));
            elems.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_NORMAL));
            if (incTan)
            {
                elems.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT4, Ogre::VES_TANGENT));
            }
            elems.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES));

            const size_t fpv = si.floatsPerVertex, vS = si.vertexOffset, vC = si.vertexCount;
            float* vd = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(vC * fpv * sizeof(float), Ogre::MEMCATEGORY_GEOMETRY));
            for (size_t i = 0; i < vC; ++i)
            {
                const size_t gi = vS + i;
                size_t o = i * fpv;
                vd[o++] = this->vertices[gi].x;
                vd[o++] = this->vertices[gi].y;
                vd[o++] = this->vertices[gi].z;
                vd[o++] = this->normals[gi].x;
                vd[o++] = this->normals[gi].y;
                vd[o++] = this->normals[gi].z;
                if (incTan)
                {
                    vd[o++] = this->tangents[gi].x;
                    vd[o++] = this->tangents[gi].y;
                    vd[o++] = this->tangents[gi].z;
                    vd[o++] = this->tangents[gi].w;
                }
                vd[o++] = this->uvCoordinates[gi].x;
                vd[o] = this->uvCoordinates[gi].y;
                minBB.makeFloor(this->vertices[gi]);
                maxBB.makeCeil(this->vertices[gi]);
            }
            si.dynamicVertexBuffer = vm->createVertexBuffer(elems, vC, Ogre::BT_DYNAMIC_DEFAULT, vd, false);

            const size_t iC = si.indexCount, iS = si.indexOffset;
            Ogre::uint16* id = reinterpret_cast<Ogre::uint16*>(OGRE_MALLOC_SIMD(iC * sizeof(Ogre::uint16), Ogre::MEMCATEGORY_GEOMETRY));
            for (size_t i = 0; i < iC; ++i)
            {
                id[i] = static_cast<Ogre::uint16>(this->indices[iS + i] - vS);
            }
            si.dynamicIndexBuffer = vm->createIndexBuffer(Ogre::IndexBufferPacked::IT_16BIT, iC, Ogre::BT_DEFAULT, id, true);

            Ogre::VertexBufferPackedVec vbv;
            vbv.push_back(si.dynamicVertexBuffer);
            Ogre::VertexArrayObject* vao = vm->createVertexArrayObject(vbv, si.dynamicIndexBuffer, Ogre::OT_TRIANGLE_LIST);
            Ogre::SubMesh* subMesh = this->editableMesh->createSubMesh();
            subMesh->mVao[Ogre::VpNormal].push_back(vao);
            subMesh->mVao[Ogre::VpShadow].push_back(vao);
        }

        Ogre::Aabb bounds;
        bounds.setExtents(minBB, maxBB);
        this->editableMesh->_setBounds(bounds, false);
        this->editableMesh->_setBoundingSphereRadius(bounds.getRadius());

        this->editableItem = this->gameObjectPtr->getSceneManager()->createItem(this->editableMesh, this->gameObjectPtr->isDynamic() ? Ogre::SCENE_DYNAMIC : Ogre::SCENE_STATIC);
        this->editableItem->setName(this->gameObjectPtr->getName() + "_MeshEdit");

        for (size_t i = 0; i < this->subMeshInfoList.size(); ++i)
        {
            if (srcItem && i < srcItem->getNumSubItems() && i < this->editableItem->getNumSubItems())
            {
                Ogre::HlmsDatablock* db = srcItem->getSubItem(i)->getDatablock();
                if (db)
                {
                    this->editableItem->getSubItem(i)->setDatablock(db);
                }
            }
        }

        this->dynamicVertexBuffer = this->subMeshInfoList.empty() ? nullptr : this->subMeshInfoList[0].dynamicVertexBuffer;
        this->dynamicIndexBuffer = this->subMeshInfoList.empty() ? nullptr : this->subMeshInfoList[0].dynamicIndexBuffer;
        return true;
    }

    // =========================================================================
    //  rebuildDynamicBuffers — safe swap, no crash
    // =========================================================================

    bool MeshEditComponent::rebuildDynamicBuffers(void)
    {
        bool success = false;
        NOWA::GraphicsModule::RenderCommand cmd = [this, &success]()
        {
            Ogre::VaoManager* vm = Ogre::Root::getSingletonPtr()->getRenderSystem()->getVaoManager();
            this->recalculateNormals_internal();
            this->recalculateTangents();

            const bool hasTan = this->vertexFormat.hasTangent;
            const size_t fpv = hasTan ? 12u : 8u;

            // Build new mesh with a temporary name
            Ogre::String tmpName = this->gameObjectPtr->getName() + "_MeshEditTmp_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
            Ogre::MeshPtr newMesh = Ogre::MeshManager::getSingleton().createManual(tmpName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
            newMesh->_setVaoManager(vm);

            Ogre::Vector3 minBB(std::numeric_limits<float>::max());
            Ogre::Vector3 maxBB(-std::numeric_limits<float>::max());

            float* vd = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(this->vertexCount * fpv * sizeof(float), Ogre::MEMCATEGORY_GEOMETRY));
            for (size_t i = 0; i < this->vertexCount; ++i)
            {
                size_t o = i * fpv;
                vd[o++] = this->vertices[i].x;
                vd[o++] = this->vertices[i].y;
                vd[o++] = this->vertices[i].z;
                vd[o++] = this->normals[i].x;
                vd[o++] = this->normals[i].y;
                vd[o++] = this->normals[i].z;
                if (hasTan)
                {
                    vd[o++] = this->tangents[i].x;
                    vd[o++] = this->tangents[i].y;
                    vd[o++] = this->tangents[i].z;
                    vd[o++] = this->tangents[i].w;
                }
                vd[o++] = this->uvCoordinates[i].x;
                vd[o] = this->uvCoordinates[i].y;
                minBB.makeFloor(this->vertices[i]);
                maxBB.makeCeil(this->vertices[i]);
            }

            Ogre::VertexElement2Vec elems;
            elems.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_POSITION));
            elems.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_NORMAL));
            if (hasTan)
            {
                elems.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT4, Ogre::VES_TANGENT));
            }
            elems.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES));

            Ogre::VertexBufferPacked* newVB = vm->createVertexBuffer(elems, this->vertexCount, Ogre::BT_DYNAMIC_DEFAULT, vd, false);

            Ogre::uint16* id = reinterpret_cast<Ogre::uint16*>(OGRE_MALLOC_SIMD(this->indexCount * sizeof(Ogre::uint16), Ogre::MEMCATEGORY_GEOMETRY));
            for (size_t i = 0; i < this->indexCount; ++i)
            {
                id[i] = static_cast<Ogre::uint16>(this->indices[i]);
            }

            Ogre::IndexBufferPacked* newIB = vm->createIndexBuffer(Ogre::IndexBufferPacked::IT_16BIT, this->indexCount, Ogre::BT_DEFAULT, id, true);

            Ogre::VertexBufferPackedVec vbv;
            vbv.push_back(newVB);
            Ogre::VertexArrayObject* newVao = vm->createVertexArrayObject(vbv, newIB, Ogre::OT_TRIANGLE_LIST);
            Ogre::SubMesh* newSM = newMesh->createSubMesh();
            newSM->mVao[Ogre::VpNormal].push_back(newVao);
            newSM->mVao[Ogre::VpShadow].push_back(newVao);

            Ogre::Aabb bounds;
            bounds.setExtents(minBB, maxBB);
            newMesh->_setBounds(bounds, false);
            newMesh->_setBoundingSphereRadius(bounds.getRadius());

            Ogre::Item* newItem = this->gameObjectPtr->getSceneManager()->createItem(newMesh, this->gameObjectPtr->isDynamic() ? Ogre::SCENE_DYNAMIC : Ogre::SCENE_STATIC);
            newItem->setName(this->gameObjectPtr->getName() + "_MeshEdit");

            // Carry over datablock
            if (this->editableItem && this->editableItem->getNumSubItems() > 0 && newItem->getNumSubItems() > 0)
            {
                Ogre::HlmsDatablock* db = this->editableItem->getSubItem(0)->getDatablock();
                if (db)
                {
                    newItem->getSubItem(0)->setDatablock(db);
                }
            }

            // Safe swap: detach old (still valid), attach new, get old back
            Ogre::MovableObject* oldMovable = nullptr;
            this->gameObjectPtr->swapMovableObject(newItem, oldMovable);

            // After swap, carry over ALL original datablocks by blending
            // Since we collapse to 1 submesh, use the FIRST non-null datablock found
            if (newItem->getNumSubItems() > 0)
            {
                Ogre::HlmsDatablock* db = nullptr;
                if (this->editableItem)
                {
                    for (size_t s = 0; s < this->editableItem->getNumSubItems() && !db; ++s)
                    {
                        db = this->editableItem->getSubItem(s)->getDatablock();
                    }
                }
                if (db)
                {
                    newItem->getSubItem(0)->setDatablock(db);
                }
            }

            // Now destroy old item (already detached by swap)
            if (oldMovable)
            {
                this->gameObjectPtr->getSceneManager()->destroyItem(static_cast<Ogre::Item*>(oldMovable));
            }

            // Remove old mesh resource
            if (this->editableMesh)
            {
                Ogre::MeshManager::getSingleton().remove(this->editableMesh);
                this->editableMesh.reset();
            }

            // Adopt new resources
            this->editableMesh = newMesh;
            this->editableItem = newItem;

            // Rebuild SubMeshInfo (topology changes collapse to 1 submesh)
            this->subMeshInfoList.clear();
            SubMeshInfo si;
            si.vertexOffset = 0;
            si.vertexCount = this->vertexCount;
            si.indexOffset = 0;
            si.indexCount = this->indexCount;
            si.hasTangent = hasTan;
            si.floatsPerVertex = fpv;
            si.dynamicVertexBuffer = newVB;
            si.dynamicIndexBuffer = newIB;
            this->subMeshInfoList.push_back(si);
            this->dynamicVertexBuffer = newVB;
            this->dynamicIndexBuffer = newIB;

            success = true;
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "MeshEditComponent::rebuildDynamicBuffers");
        return success;
    }

    void MeshEditComponent::destroyEditableMesh(void)
    {
        NOWA::GraphicsModule::RenderCommand cmd = [this]()
        {
            for (SubMeshInfo& sm : this->subMeshInfoList)
            {
                if (sm.dynamicVertexBuffer && sm.dynamicVertexBuffer->getMappingState() != Ogre::MS_UNMAPPED)
                {
                    sm.dynamicVertexBuffer->unmap(Ogre::UO_UNMAP_ALL);
                }
                sm.dynamicVertexBuffer = nullptr;
                sm.dynamicIndexBuffer = nullptr;
            }
            this->subMeshInfoList.clear();
            this->dynamicVertexBuffer = nullptr;
            this->dynamicIndexBuffer = nullptr;

            if (this->editableItem)
            {
                this->gameObjectPtr->getSceneManager()->destroyItem(this->editableItem);
                this->editableItem = nullptr;
            }
            if (this->editableMesh)
            {
                Ogre::MeshManager::getSingleton().remove(this->editableMesh);
                this->editableMesh.reset();
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "MeshEditComponent::destroyEditableMesh");
    }

    void MeshEditComponent::uploadVertexData(void)
    {
        for (SubMeshInfo& sm : this->subMeshInfoList)
        {
            if (!sm.dynamicVertexBuffer)
            {
                continue;
            }
            const size_t fpv = sm.floatsPerVertex, vS = sm.vertexOffset, vC = sm.vertexCount;
            float* vd = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(vC * fpv * sizeof(float), Ogre::MEMCATEGORY_GEOMETRY));
            for (size_t i = 0; i < vC; ++i)
            {
                const size_t gi = vS + i;
                size_t o = i * fpv;
                vd[o++] = this->vertices[gi].x;
                vd[o++] = this->vertices[gi].y;
                vd[o++] = this->vertices[gi].z;
                vd[o++] = this->normals[gi].x;
                vd[o++] = this->normals[gi].y;
                vd[o++] = this->normals[gi].z;
                if (sm.hasTangent)
                {
                    vd[o++] = this->tangents[gi].x;
                    vd[o++] = this->tangents[gi].y;
                    vd[o++] = this->tangents[gi].z;
                    vd[o++] = this->tangents[gi].w;
                }
                vd[o++] = this->uvCoordinates[gi].x;
                vd[o] = this->uvCoordinates[gi].y;
            }
            sm.dynamicVertexBuffer->upload(vd, 0, vC);
            OGRE_FREE_SIMD(vd, Ogre::MEMCATEGORY_GEOMETRY);
        }
    }

    // =========================================================================
    //  Normals / tangents
    // =========================================================================

    void MeshEditComponent::recalculateNormals_internal(void)
    {
        for (auto& n : this->normals)
        {
            n = Ogre::Vector3::ZERO;
        }
        for (size_t i = 0; i < this->indexCount; i += 3)
        {
            size_t i0 = this->indices[i], i1 = this->indices[i + 1], i2 = this->indices[i + 2];
            Ogre::Vector3 fn = (this->vertices[i1] - this->vertices[i0]).crossProduct(this->vertices[i2] - this->vertices[i0]);
            this->normals[i0] += fn;
            this->normals[i1] += fn;
            this->normals[i2] += fn;
        }
        for (auto& n : this->normals)
        {
            n.normalise();
        }
    }

    void MeshEditComponent::recalculateTangents(void)
    {
        bool anyTan = false;
        for (const SubMeshInfo& sm : this->subMeshInfoList)
        {
            if (sm.hasTangent)
            {
                anyTan = true;
                break;
            }
        }
        if (!anyTan && !this->vertexFormat.hasTangent)
        {
            return;
        }

        std::vector<Ogre::Vector3> t1(this->vertexCount, Ogre::Vector3::ZERO);
        std::vector<Ogre::Vector3> t2(this->vertexCount, Ogre::Vector3::ZERO);
        for (size_t i = 0; i < this->indexCount; i += 3)
        {
            size_t i0 = this->indices[i], i1 = this->indices[i + 1], i2 = this->indices[i + 2];
            const Ogre::Vector3 &v0 = this->vertices[i0], &v1 = this->vertices[i1], &v2 = this->vertices[i2];
            const Ogre::Vector2 &u0 = this->uvCoordinates[i0], &u1 = this->uvCoordinates[i1], &u2 = this->uvCoordinates[i2];
            Ogre::Vector3 e1 = v1 - v0, e2 = v2 - v0;
            float du1 = u1.x - u0.x, dv1 = u1.y - u0.y, du2 = u2.x - u0.x, dv2 = u2.y - u0.y;
            float d = du1 * dv2 - du2 * dv1;
            if (std::abs(d) < 1e-6f)
            {
                d = 1e-6f;
            }
            float r = 1.0f / d;
            Ogre::Vector3 s((dv2 * e1.x - dv1 * e2.x) * r, (dv2 * e1.y - dv1 * e2.y) * r, (dv2 * e1.z - dv1 * e2.z) * r);
            Ogre::Vector3 tt((du1 * e2.x - du2 * e1.x) * r, (du1 * e2.y - du2 * e1.y) * r, (du1 * e2.z - du2 * e1.z) * r);
            t1[i0] += s;
            t1[i1] += s;
            t1[i2] += s;
            t2[i0] += tt;
            t2[i1] += tt;
            t2[i2] += tt;
        }
        for (size_t i = 0; i < this->vertexCount; ++i)
        {
            const Ogre::Vector3& n = this->normals[i];
            Ogre::Vector3 tan = (t1[i] - n * n.dotProduct(t1[i])).normalisedCopy();
            float hand = (n.crossProduct(t1[i]).dotProduct(t2[i]) < 0.0f) ? -1.0f : 1.0f;
            this->tangents[i] = {tan.x, tan.y, tan.z, hand};
        }
    }

    void MeshEditComponent::buildVertexAdjacency(void)
    {
        this->vertexNeighbors.clear();
        this->vertexNeighbors.resize(this->vertexCount);
        std::vector<std::unordered_set<size_t>> sets(this->vertexCount);
        for (size_t i = 0; i < this->indexCount; i += 3)
        {
            size_t i0 = this->indices[i], i1 = this->indices[i + 1], i2 = this->indices[i + 2];
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

    // =========================================================================
    //  Overlay
    // =========================================================================

    void MeshEditComponent::createOverlay(void)
    {
        NOWA::GraphicsModule::RenderCommand cmd = [this]()
        {
            this->overlayNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode();
            this->overlayObject = this->gameObjectPtr->getSceneManager()->createManualObject();
            this->overlayObject->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_MESH);
            this->overlayObject->setName("MeshEdit_Overlay_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()));
            this->overlayObject->setQueryFlags(0 << 0);
            this->overlayObject->setCastShadows(false);
            this->overlayNode->attachObject(this->overlayObject);
            this->overlayNode->setVisible(false);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "MeshEditComponent::createOverlay");
    }

    void MeshEditComponent::destroyOverlay(void)
    {
        const Ogre::String closureId = this->gameObjectPtr->getName() + "_MeshEditOverlay_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
        NOWA::GraphicsModule::getInstance()->removeTrackedClosure(closureId);

        NOWA::GraphicsModule::RenderCommand cmd = [this]()
        {
            if (!this->overlayNode)
            {
                return;
            }
            this->overlayNode->detachAllObjects();
            if (this->overlayObject)
            {
                this->gameObjectPtr->getSceneManager()->destroyManualObject(this->overlayObject);
                this->overlayObject = nullptr;
            }
            this->overlayNode->getParentSceneNode()->removeAndDestroyChild(this->overlayNode);
            this->overlayNode = nullptr;
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "MeshEditComponent::destroyOverlay");
    }

    void MeshEditComponent::scheduleOverlayUpdate(void)
    {
        this->overlayDirty.store(true);
    }

    // =========================================================================
    //  Picking
    // =========================================================================

    bool MeshEditComponent::raycastMesh(Ogre::Real sx, Ogre::Real sy, Ogre::Vector3& hitPos, Ogre::Vector3& hitNorm, size_t& hitTri)
    {
        Ogre::Camera* cam = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
        if (!cam)
        {
            return false;
        }
        Ogre::Ray ray = cam->getCameraToViewportRay(sx, sy);
        Ogre::Matrix4 inv = this->gameObjectPtr->getSceneNode()->_getFullTransform().inverse();
        Ogre::Matrix3 inv3;
        inv.extract3x3Matrix(inv3);
        Ogre::Ray lr(inv * ray.getOrigin(), (inv3 * ray.getDirection()).normalisedCopy());

        float best = std::numeric_limits<float>::max();
        bool hit = false;
        hitTri = SIZE_MAX;
        for (size_t i = 0; i < this->indexCount; i += 3)
        {
            auto r = Ogre::Math::intersects(lr, this->vertices[this->indices[i]], this->vertices[this->indices[i + 1]], this->vertices[this->indices[i + 2]], true, false);
            if (r.first && r.second < best)
            {
                best = r.second;
                hitPos = lr.getPoint(best);
                hitNorm = ((this->vertices[this->indices[i + 1]] - this->vertices[this->indices[i]]).crossProduct(this->vertices[this->indices[i + 2]] - this->vertices[this->indices[i]])).normalisedCopy();
                hitTri = i / 3;
                hit = true;
            }
        }
        return hit;
    }

    bool MeshEditComponent::pickVertex(Ogre::Real sx, Ogre::Real sy, size_t& outIdx)
    {
        Ogre::Camera* cam = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
        if (!cam)
        {
            return false;
        }

        Ogre::Ray ray = cam->getCameraToViewportRay(sx, sy); // same as face picking
        const Ogre::Matrix4 world = this->gameObjectPtr->getSceneNode()->_getFullTransform();

        float bestDist = std::numeric_limits<float>::max();
        size_t bi = SIZE_MAX;

        for (size_t i = 0; i < this->vertexCount; ++i)
        {
            Ogre::Vector3 worldPos = world * this->vertices[i];
            float t = (worldPos - ray.getOrigin()).dotProduct(ray.getDirection());
            if (t <= 0.0f)
            {
                continue; // behind camera
            }

            Ogre::Vector3 closest = ray.getOrigin() + ray.getDirection() * t;
            float perpDist = (worldPos - closest).length();

            // Scale tolerance with camera distance so it works at any zoom level
            float camDist = (worldPos - cam->getDerivedPosition()).length();
            float tolerance = camDist * 0.02f;

            if (perpDist < tolerance && perpDist < bestDist)
            {
                bestDist = perpDist;
                bi = i;
            }
        }
        if (bi != SIZE_MAX)
        {
            outIdx = bi;
            return true;
        }
        return false;
    }

    bool MeshEditComponent::pickEdge(Ogre::Real sx, Ogre::Real sy, EdgeKey& outEdge)
    {
        Ogre::Camera* cam = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
        if (!cam)
        {
            return false;
        }
        const Ogre::Matrix4 vp = cam->getProjectionMatrix() * cam->getViewMatrix();
        const Ogre::Matrix4 world = this->gameObjectPtr->getSceneNode()->_getFullTransform();

        std::vector<Ogre::Vector2> sp(this->vertexCount);
        std::vector<bool> valid(this->vertexCount, false);
        for (size_t i = 0; i < this->vertexCount; ++i)
        {
            Ogre::Vector4 cp = vp * (world * Ogre::Vector4(this->vertices[i], 1.0f));
            if (cp.w <= 0.0f)
            {
                continue;
            }
            sp[i] = {((cp.x / cp.w) + 1.0f) * 0.5f, 1.0f - ((cp.y / cp.w) + 1.0f) * 0.5f};
            valid[i] = true;
        }

        auto ptSeg = [](float px, float py, float ax, float ay, float bx, float by) -> float
        {
            float dx = bx - ax, dy = by - ay, len = dx * dx + dy * dy;
            if (len < 1e-8f)
            {
                return std::sqrt((px - ax) * (px - ax) + (py - ay) * (py - ay));
            }
            float t = std::max(0.0f, std::min(1.0f, ((px - ax) * dx + (py - ay) * dy) / len));
            return std::sqrt((px - ax - t * dx) * (px - ax - t * dx) + (py - ay - t * dy) * (py - ay - t * dy));
        };

        float best = 0.02f;
        size_t ba = SIZE_MAX, bb = SIZE_MAX;
        std::set<std::pair<size_t, size_t>> checked;
        for (size_t i = 0; i < this->indexCount; i += 3)
        {
            for (int e = 0; e < 3; ++e)
            {
                size_t a = this->indices[i + e], b = this->indices[i + (e + 1) % 3];
                if (!checked.insert({std::min(a, b), std::max(a, b)}).second)
                {
                    continue;
                }
                if (!valid[a] || !valid[b])
                {
                    continue;
                }
                float d = ptSeg(sx, sy, sp[a].x, sp[a].y, sp[b].x, sp[b].y);
                if (d < best)
                {
                    best = d;
                    ba = a;
                    bb = b;
                }
            }
        }
        if (ba != SIZE_MAX)
        {
            outEdge = EdgeKey(ba, bb);
            return true;
        }
        return false;
    }

    bool MeshEditComponent::pickFace(Ogre::Real sx, Ogre::Real sy, size_t& outFace)
    {
        Ogre::Vector3 hp, hn;
        size_t ht;
        if (this->raycastMesh(sx, sy, hp, hn, ht))
        {
            outFace = ht;
            return true;
        }
        return false;
    }

    // =========================================================================
    //  Grab
    // =========================================================================

    void MeshEditComponent::beginGrab(void)
    {
        this->grabSavedPositions = this->vertices;
        this->grabAccumDelta = Ogre::Vector3::ZERO;
        this->grabAxis = GrabAxis::FREE;
        this->isGrabbing = true;
        this->activeTool = ActiveTool::GRAB;

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MeshEditComponent] Grab started. X/Y/Z to constrain axis.");
    }

    void MeshEditComponent::confirmGrab(void)
    {
        this->isGrabbing = false;
        this->activeTool = ActiveTool::SELECT;
        this->grabSavedPositions.clear();
        this->recalculateNormals_internal();
        this->recalculateTangents();
        NOWA::GraphicsModule::RenderCommand cmd = [this]()
        {
            this->uploadVertexData();
        };
        NOWA::GraphicsModule::getInstance()->enqueue(std::move(cmd), "MeshEditComponent::confirmGrab");
        this->scheduleOverlayUpdate();
    }

    void MeshEditComponent::cancelGrab(void)
    {
        if (!this->grabSavedPositions.empty())
        {
            this->vertices = this->grabSavedPositions;
        }
        this->grabSavedPositions.clear();
        this->isGrabbing = false;
        this->activeTool = ActiveTool::SELECT;
        NOWA::GraphicsModule::RenderCommand cmd = [this]()
        {
            this->uploadVertexData();
        };
        NOWA::GraphicsModule::getInstance()->enqueue(std::move(cmd), "MeshEditComponent::cancelGrab");
        this->scheduleOverlayUpdate();
    }

    void MeshEditComponent::applyGrabMouseDelta(int dx, int dy)
    {
        if (!this->isGrabbing)
        {
            return;
        }
        Ogre::Camera* cam = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
        if (!cam)
        {
            return;
        }

        const float sen = 0.005f;
        Ogre::Vector3 wDelta = cam->getRight() * (dx * sen) + cam->getUp() * (-dy * sen);

        // Axis constraint — project world delta onto the chosen world axis
        // Ogre coordinate system: X=right  Y=up  Z=toward viewer (right-handed)
        // KC_X → world X,  KC_Y → world Y (up),  KC_Z → world Z (depth)
        switch (this->grabAxis)
        {
        case GrabAxis::X:
            wDelta = Ogre::Vector3::UNIT_X * wDelta.dotProduct(Ogre::Vector3::UNIT_X);
            break;
        case GrabAxis::Y:
            // Y = up in Ogre (≡ Blender Z)
            // Use full mouse movement magnitude projected onto Y
            wDelta = Ogre::Vector3::UNIT_Y * wDelta.dotProduct(Ogre::Vector3::UNIT_Y);
            // If camera is looking level the Y component of wDelta may be tiny —
            // remap mouse vertical directly to Y in that case
            if (std::abs(wDelta.y) < 1e-4f)
            {
                wDelta = Ogre::Vector3::UNIT_Y * (-dy * sen);
            }
            break;
        case GrabAxis::Z:
            wDelta = Ogre::Vector3::UNIT_Z * wDelta.dotProduct(Ogre::Vector3::UNIT_Z);
            break;
        default:
            break;
        }

        const Ogre::Matrix4 worldMat = this->gameObjectPtr->getSceneNode()->_getFullTransform();
        Ogre::Matrix3 rot3;
        worldMat.extract3x3Matrix(rot3);
        const Ogre::Vector3 localDelta = rot3.Inverse() * wDelta;

        this->grabAccumDelta += localDelta;

        std::unordered_set<size_t> toMove;
        for (size_t i : this->selectedVertices)
        {
            toMove.insert(i);
        }
        for (const EdgeKey& e : this->selectedEdges)
        {
            toMove.insert(e.a);
            toMove.insert(e.b);
        }
        for (size_t t : this->selectedFaces)
        {
            toMove.insert(this->indices[t * 3]);
            toMove.insert(this->indices[t * 3 + 1]);
            toMove.insert(this->indices[t * 3 + 2]);
        }

        for (size_t i : toMove)
        {
            this->vertices[i] = this->grabSavedPositions[i] + this->grabAccumDelta;
        }

        NOWA::GraphicsModule::RenderCommand cmd = [this]()
        {
            this->uploadVertexData();
        };
        NOWA::GraphicsModule::getInstance()->enqueue(std::move(cmd), "MeshEditComponent::grabMove");
        this->scheduleOverlayUpdate();
    }

    // =========================================================================
    //  Brush
    // =========================================================================

    Ogre::Real MeshEditComponent::calculateBrushInfluence(Ogre::Real dist, Ogre::Real radius) const
    {
        if (dist >= radius)
        {
            return 0.0f;
        }
        Ogre::Real nd = dist / radius;
        Ogre::Real inf = std::pow(1.0f - nd, this->brushFalloff->getReal());
        if (!this->brushData.empty() && this->brushImageWidth > 0)
        {
            size_t cx = this->brushImageWidth / 2;
            size_t sx2 = std::min(static_cast<size_t>(cx + nd * cx), this->brushImageWidth - 1);
            inf *= this->brushData[cx * this->brushImageWidth + sx2];
        }
        return inf * this->brushIntensity->getReal();
    }

    void MeshEditComponent::applyBrush(const Ogre::Vector3& ctr, bool invert)
    {
        Ogre::Real radius = this->brushSize->getReal();
        BrushMode mode = this->getBrushMode();
        if (invert)
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

        Ogre::Vector3 avgP = Ogre::Vector3::ZERO, avgN = Ogre::Vector3::ZERO;
        std::vector<std::pair<size_t, Ogre::Real>> aff;
        for (size_t i = 0; i < this->vertexCount; ++i)
        {
            Ogre::Real d = this->vertices[i].distance(ctr);
            if (d < radius)
            {
                Ogre::Real inf = this->calculateBrushInfluence(d, radius);
                aff.push_back({i, inf});
                avgP += this->vertices[i];
                avgN += this->normals[i];
            }
        }
        if (aff.empty())
        {
            return;
        }
        avgP /= static_cast<Ogre::Real>(aff.size());
        avgN.normalise();

        for (auto& [idx, inf] : aff)
        {
            switch (mode)
            {
            case BrushMode::PUSH:
                this->vertices[idx] += this->normals[idx] * inf;
                break;
            case BrushMode::PULL:
                this->vertices[idx] -= this->normals[idx] * inf;
                break;
            case BrushMode::SMOOTH:
                if (idx < this->vertexNeighbors.size() && !this->vertexNeighbors[idx].empty())
                {
                    Ogre::Vector3 avg = Ogre::Vector3::ZERO;
                    for (size_t nb : this->vertexNeighbors[idx])
                    {
                        avg += this->vertices[nb];
                    }
                    avg /= static_cast<Ogre::Real>(this->vertexNeighbors[idx].size());
                    this->vertices[idx] = this->vertices[idx] * (1.0f - inf) + avg * inf;
                }
                break;
            case BrushMode::FLATTEN:
            {
                Ogre::Real d2p = avgN.dotProduct(this->vertices[idx] - avgP);
                this->vertices[idx] -= avgN * d2p * inf;
            }
            break;
            case BrushMode::PINCH:
            {
                Ogre::Vector3 tc = (ctr - this->vertices[idx]).normalisedCopy();
                this->vertices[idx] += tc * inf;
            }
            break;
            case BrushMode::INFLATE:
                this->vertices[idx] += this->normals[idx] * inf;
                break;
            }
        }

        this->recalculateNormals_internal();
        this->recalculateTangents();
        NOWA::GraphicsModule::RenderCommand cmd = [this]()
        {
            this->uploadVertexData();
        };
        NOWA::GraphicsModule::getInstance()->enqueue(std::move(cmd), "MeshEditComponent::applyBrush");
        this->scheduleOverlayUpdate();
    }

    // =========================================================================
    //  Coordinates
    // =========================================================================

    Ogre::Vector3 MeshEditComponent::worldToLocal(const Ogre::Vector3& w) const
    {
        return this->gameObjectPtr->getSceneNode()->_getFullTransform().inverse() * w;
    }
    Ogre::Vector3 MeshEditComponent::localToWorld(const Ogre::Vector3& l) const
    {
        return this->gameObjectPtr->getSceneNode()->_getFullTransform() * l;
    }

    // =========================================================================
    //  Input listener + state management  (ProceduralRoadComponent pattern)
    // =========================================================================

    void MeshEditComponent::addInputListener(void)
    {
        if (!this->activated->getBool())
        {
            return;
        }
        const Ogre::String listenerName = MeshEditComponent::getStaticClassName() + "_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
        NOWA::GraphicsModule::RenderCommand cmd = [this, listenerName]()
        {
            if (auto* core = InputDeviceCore::getSingletonPtr())
            {
                core->addKeyListener(this, listenerName);
                core->addMouseListener(this, listenerName);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "MeshEditComponent::addInputListener");
    }

    void MeshEditComponent::removeInputListener(void)
    {
        const Ogre::String listenerName = MeshEditComponent::getStaticClassName() + "_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
        NOWA::GraphicsModule::RenderCommand cmd = [this, listenerName]()
        {
            if (auto* core = InputDeviceCore::getSingletonPtr())
            {
                core->removeKeyListener(listenerName);
                core->removeMouseListener(listenerName);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "MeshEditComponent::removeInputListener");
    }

    void MeshEditComponent::updateModificationState(void)
    {
        // Exactly mirrors ProceduralRoadComponent::updateModificationState
        const bool shouldBeActive = this->activated->getBool() && this->isEditorMeshModifyMode && this->isSelected && !this->isSimulating;
        if (true == shouldBeActive)
        {
            this->addInputListener();
        }
        else
        {
            this->removeInputListener();
            if (this->isGrabbing)
            {
                this->cancelGrab();
            }
        }
    }

    // =========================================================================
    //  Event handlers  (ProceduralRoadComponent pattern)
    // =========================================================================

    void MeshEditComponent::handleMeshModifyMode(NOWA::EventDataPtr eventData)
    {
        auto cast = boost::static_pointer_cast<NOWA::EventDataEditorMode>(eventData);
        this->isEditorMeshModifyMode = (cast->getManipulationMode() == EditorManager::EDITOR_MESH_MODIFY_MODE);
        this->updateModificationState();

        if (true == this->isEditorMeshModifyMode)
        {
            // Remove visual bounding box
            if (nullptr != this->gameObjectPtr)
            {
                this->gameObjectPtr->showBoundingBox(false);
            }
        }
    }

    void MeshEditComponent::handleGameObjectSelected(NOWA::EventDataPtr eventData)
    {
        auto cast = boost::static_pointer_cast<NOWA::EventDataGameObjectSelected>(eventData);

        if (cast->getGameObjectId() == this->gameObjectPtr->getId())
        {
            this->isSelected = cast->getIsSelected();
        }
        else if (cast->getIsSelected())
        {
            // Another object was selected — deselect ourselves
            this->isSelected = false;
        }

        this->updateModificationState();
    }

    // =========================================================================
    //  Static helpers
    // =========================================================================

    bool MeshEditComponent::canStaticAddComponent(GameObject* go)
    {
        Ogre::Item* item = go->getMovableObjectUnsafe<Ogre::Item>();
        return (item != nullptr && go->getComponentCount<MeshEditComponent>() == 0);
    }

    void MeshEditComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& /*gameObjectClass*/, class_<GameObjectController>& /*gameObjectControllerClass*/)
    {
        luabind::module(lua)[luabind::class_<MeshEditComponent, GameObjectComponent>("MeshEditComponent")
                .def("setEditMode", &MeshEditComponent::setEditMode)
                .def("getEditModeString", &MeshEditComponent::getEditModeString)
                .def("selectAll", &MeshEditComponent::selectAll)
                .def("deselectAll", &MeshEditComponent::deselectAll)
                .def("deleteSelected", &MeshEditComponent::deleteSelected)
                .def("mergeByDistance", &MeshEditComponent::mergeByDistance)
                .def("flipNormals", &MeshEditComponent::flipNormals)
                .def("recalculateNormals", &MeshEditComponent::recalculateNormals)
                .def("subdivideSelectedFaces", &MeshEditComponent::subdivideSelectedFaces)
                .def("extrudeSelectedFaces", &MeshEditComponent::extrudeSelectedFaces)
                .def("undo", &MeshEditComponent::undo)
                .def("canUndo", &MeshEditComponent::canUndo)
                .def("exportMesh", &MeshEditComponent::exportMesh)
                .def("applyMesh", &MeshEditComponent::applyMesh)
                .def("getVertexCount", &MeshEditComponent::getVertexCount)
                .def("getTriangleCount", &MeshEditComponent::getTriangleCount)
                .def("setBrushSize", &MeshEditComponent::setBrushSize)
                .def("getBrushSize", &MeshEditComponent::getBrushSize)
                .def("setBrushIntensity", &MeshEditComponent::setBrushIntensity)
                .def("getBrushIntensity", &MeshEditComponent::getBrushIntensity)
                .def("setBrushMode", &MeshEditComponent::setBrushMode)
                .def("getBrushModeString", &MeshEditComponent::getBrushModeString)
                .def("setOutputFileName", &MeshEditComponent::setOutputFileName)
                .def("getOutputFileName", &MeshEditComponent::getOutputFileName)];
    }

}; // namespace NOWA
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

namespace
{
    std::unordered_set<size_t> expandToCoLocated(const std::unordered_set<size_t>& base, const std::vector<int>& vertexGroup, const std::vector<std::vector<size_t>>& positionGroups)
    {
        std::unordered_set<size_t> result = base;
        for (size_t idx : base)
        {
            const int g = vertexGroup[idx];
            if (g < 0)
            {
                continue; // lone vertex — no group
            }
            for (size_t member : positionGroups[static_cast<size_t>(g)])
            {
                result.insert(member);
            }
        }
        return result;
    }
}

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    // =========================================================================
    //  Constructor
    // =========================================================================

    MeshEditComponent::MeshEditComponent() :
        MeshEditComponentBase(),
        componentName("MeshEditComponent"),
        editableItem(nullptr),
        dynamicVertexBuffer(nullptr),
        dynamicIndexBuffer(nullptr),
        meshRebuildCounter(0),
        isIndices32(false),
        vertexCount(0),
        indexCount(0),
        brushImageWidth(0),
        brushImageHeight(0),
        activeTool(ActiveTool::SELECT),
        isGrabbing(false),
        isBrushArmed(false),
        brushLastHitPos(Ogre::Vector3::ZERO),
        overlayNode(nullptr),
        overlayObject(nullptr),
        overlayClosureRemoved(false),
        isPressing(false),
        isShiftPressed(false),
        isCtrlPressed(false),
        isEditorMeshModifyMode(false),
        isSelected(false),
        isSimulating(false),
        isRectSelecting(false),
        rectDragThreshold(4.0f),
        rectPressX(0),
        rectPressY(0),

        // ── Core ─────────────────────────────────────────────────────────────
        activated(new Variant(MeshEditComponent::AttrActivated(), true, this->attributes)),
        editMode(new Variant(MeshEditComponent::AttrEditMode(), std::vector<Ogre::String>{"Object", "Vertex", "Edge", "Face"}, this->attributes)),
        showWireframe(new Variant(MeshEditComponent::AttrShowWireframe(), true, this->attributes)),
        xRayOverlay(new Variant(MeshEditComponent::AttrXRayOverlay(), true, this->attributes)),
        vertexMarkerSize(new Variant(MeshEditComponent::AttrVertexMarkerSize(), 0.04f, this->attributes)),
        outputFileName(new Variant(MeshEditComponent::AttrOutputFileName(), Ogre::String(""), this->attributes)),

        // ── Brush ────────────────────────────────────────────────────────────
        brushName(new Variant(MeshEditComponent::AttrBrushName(), this->attributes)),
        brushSize(new Variant(MeshEditComponent::AttrBrushSize(), 1.0f, this->attributes)),
        brushIntensity(new Variant(MeshEditComponent::AttrBrushIntensity(), 0.1f, this->attributes)),
        brushFalloff(new Variant(MeshEditComponent::AttrBrushFalloff(), 2.0f, this->attributes)),
        brushMode(new Variant(MeshEditComponent::AttrBrushMode(), std::vector<Ogre::String>{"Push", "Pull", "Smooth", "Flatten", "Pinch", "Inflate"}, this->attributes)),

        // ── Modeling Parameters ─────────────────────────────────────────
        proportionalRadius(new Variant(MeshEditComponent::AttrProportionalRadius(), 1.0f, this->attributes)),
        bevelAmount(new Variant(MeshEditComponent::AttrBevelAmount(), 0.05f, this->attributes)),
        loopCutFraction(new Variant(MeshEditComponent::AttrLoopCutFraction(), 0.5f, this->attributes)),

        // ── Existing Actions ─────────────────────────────────────────────────
        weldButton(new Variant(MeshEditComponent::AttrWeldVertices(), Ogre::String("Weld"), this->attributes)),
        flipNormalsButton(new Variant(MeshEditComponent::AttrFlipNormals(), Ogre::String("Flip Normals"), this->attributes)),
        recalcNormalsButton(new Variant(MeshEditComponent::AttrRecalcNormals(), Ogre::String("Recalculate"), this->attributes)),
        subdivideFacesButton(new Variant(MeshEditComponent::AttrSubdivideFaces(), Ogre::String("Subdivide Selected"), this->attributes)),
        subdivideAllButton(new Variant(MeshEditComponent::AttrSubdivideAll(), Ogre::String("Subdivide All"), this->attributes)),

        extrudeAmount(new Variant(MeshEditComponent::AttrExtrudeAmount(), 0.1f, this->attributes)),
        extrudeFacesButton(new Variant(MeshEditComponent::AttrExtrudeFaces(), Ogre::String("Extrude Selected"), this->attributes)),

        // ── Modeling Actions ────────────────────────────────────────────
        mergeSelectedButton(new Variant(MeshEditComponent::AttrMergeSelected(), Ogre::String("Merge Selected"), this->attributes)),
        dissolveButton(new Variant(MeshEditComponent::AttrDissolveSelected(), Ogre::String("Dissolve"), this->attributes)),
        fillButton(new Variant(MeshEditComponent::AttrFillSelected(), Ogre::String("Fill"), this->attributes)),
        bevelButton(new Variant(MeshEditComponent::AttrBevel(), Ogre::String("Bevel"), this->attributes)),
        loopCutButton(new Variant(MeshEditComponent::AttrLoopCut(), Ogre::String("Loop Cut"), this->attributes)),

        // ── Mirror ───────────────────────────────────────────────────────────
        mirrorAxis(new Variant(MeshEditComponent::AttrMirrorAxis(), std::vector<Ogre::String>{"+X", "-X", "+Y", "-Y", "+Z", "-Z"}, this->attributes)),
        mirrorMeshButton(new Variant(MeshEditComponent::AttrMirrorMesh(), Ogre::String("Mirror Mesh"), this->attributes)),

        // ── Apply Scale ──────────────────────────────────────────────────────────
        applyScaleButton(new Variant(MeshEditComponent::AttrApplyScale(), Ogre::String("Apply Scale"), this->attributes)),

        buildFaceButton(new Variant(MeshEditComponent::AttrBuildFace(), Ogre::String("Build Face"), this->attributes)),
        // ── Origin Alignment ─────────────────────────────────────────────────────
        originAlignment(new Variant(MeshEditComponent::AttrOriginAlignment(),
            std::vector<Ogre::String>{"Bottom Left Front", "Bottom Left Center", "Bottom Left Back", "Bottom Center Front", "Bottom Center", "Bottom Center Back", "Bottom Right Front", "Bottom Right Center", "Bottom Right Back", "Center Left Front",
                "Center Left", "Center Left Back", "Center Front", "Center", "Center Back", "Center Right Front", "Center Right", "Center Right Back", "Top Left Front", "Top Left Center", "Top Left Back", "Top Center Front", "Top Center",
                "Top Center Back", "Top Right Front", "Top Right Center", "Top Right Back"},
            this->attributes)),
        originAlignButton(new Variant(MeshEditComponent::ActionApplyOriginAlignment(), Ogre::String("Apply Origin"), this->attributes)),

        // ── Flip Direction ────────────────────────────────────────────────────────
        flipDirection(new Variant(MeshEditComponent::AttrFlipDirection(), std::vector<Ogre::String>{"X", "Y", "Z"}, this->attributes)),
        flipDirectionButton(new Variant(MeshEditComponent::ActionApplyFlipDirection(), Ogre::String("Apply Flip"), this->attributes)),
        showOrigin(new Variant(MeshEditComponent::AttrShowOrigin(), false, this->attributes)),

        // ── Final Actions ────────────────────────────────────────────────────────
        applyMeshButton(new Variant(MeshEditComponent::AttrApplyMesh(), Ogre::String("Apply Mesh"), this->attributes)),
        cancelEditButton(new Variant(MeshEditComponent::AttrCancelEdit(), Ogre::String("Cancel Edit"), this->attributes))
    {
        this->outputFileName->setDescription("Output file name (no extension). Defaults to <original>_edit. Never overwrites the original mesh.");

        this->vertexMarkerSize->setConstraints(0.005f, 1.0f);
        this->vertexMarkerSize->addUserData(GameObject::AttrActionNoUndo());

        this->proportionalRadius->setConstraints(0.001f, 100.0f);
        this->proportionalRadius->setDescription("Radius for proportional editing. Nearby vertices are affected with smooth falloff.");

        this->bevelAmount->setConstraints(0.0001f, 10.0f);
        this->bevelAmount->setDescription("Distance used for bevel operations. Controls edge width or corner rounding.");

        this->loopCutFraction->setConstraints(0.01f, 0.99f);
        this->loopCutFraction->setDescription("Position of loop cut along edges (0.5 = centered).");

        this->mergeSelectedButton->addUserData(GameObject::AttrActionExec());
        this->mergeSelectedButton->addUserData(GameObject::AttrActionExecId(), MeshEditComponent::ActionMergeSelected());
        this->mergeSelectedButton->addUserData(GameObject::AttrActionNeedRefresh());
        this->mergeSelectedButton->setDescription("Merges selected vertices into a single point (center). "
                                                  "Use to clean up geometry or close small gaps. Keyboard: M.");

        this->dissolveButton->addUserData(GameObject::AttrActionExec());
        this->dissolveButton->addUserData(GameObject::AttrActionExecId(), MeshEditComponent::ActionDissolveSelected());
        this->dissolveButton->addUserData(GameObject::AttrActionNeedRefresh());
        this->dissolveButton->setDescription("Removes selected vertices/edges while preserving surrounding faces. "
                                             "Does not create holes. Ideal for simplifying topology. Keyboard: X.");

        this->fillButton->addUserData(GameObject::AttrActionExec());
        this->fillButton->addUserData(GameObject::AttrActionExecId(), MeshEditComponent::ActionFillSelected());
        this->fillButton->addUserData(GameObject::AttrActionNeedRefresh());
        this->fillButton->setDescription("Creates a face from selected vertices or edges. "
                                         "Use to close holes in the mesh. Keyboard: F.");

        this->bevelButton->addUserData(GameObject::AttrActionExec());
        this->bevelButton->addUserData(GameObject::AttrActionExecId(), MeshEditComponent::ActionBevel());
        this->bevelButton->addUserData(GameObject::AttrActionNeedRefresh());
        this->bevelButton->setDescription("Bevels selected edges or vertices using 'Bevel Amount'. "
                                          "Creates chamfers or rounded edges. Keyboard: B.");

        this->loopCutButton->addUserData(GameObject::AttrActionExec());
        this->loopCutButton->addUserData(GameObject::AttrActionExecId(), MeshEditComponent::ActionLoopCut());
        this->loopCutButton->addUserData(GameObject::AttrActionNeedRefresh());
        this->loopCutButton->setDescription("Inserts an edge loop across the mesh at 'Loop Cut Fraction'. "
                                            "Useful for adding detail or controlling deformation. Keyboard: Ctrl+R.");

        this->weldButton->addUserData(GameObject::AttrActionExec());
        this->weldButton->addUserData(GameObject::AttrActionExecId(), MeshEditComponent::ActionWeldVertices());
        this->weldButton->addUserData(GameObject::AttrActionNeedRefresh());
        this->weldButton->setDescription("Merges vertices that are closer than 0.00001 units. Fixes z-fighting.");

        this->flipNormalsButton->addUserData(GameObject::AttrActionExec());
        this->flipNormalsButton->addUserData(GameObject::AttrActionExecId(), MeshEditComponent::ActionFlipNormals());
        this->flipNormalsButton->addUserData(GameObject::AttrActionNeedRefresh());
        this->flipNormalsButton->setDescription("Flips all mesh normals and triangle winding. Keyboard: F.");

        this->recalcNormalsButton->addUserData(GameObject::AttrActionExec());
        this->recalcNormalsButton->addUserData(GameObject::AttrActionExecId(), MeshEditComponent::ActionRecalcNormals());
        this->recalcNormalsButton->addUserData(GameObject::AttrActionNeedRefresh());
        this->recalcNormalsButton->setDescription("Recomputes smooth normals. Keyboard: Ctrl+N.");

        this->subdivideFacesButton->addUserData(GameObject::AttrActionExec());
        this->subdivideFacesButton->addUserData(GameObject::AttrActionExecId(), MeshEditComponent::ActionSubdivideFaces());
        this->subdivideFacesButton->addUserData(GameObject::AttrActionNeedRefresh());
        this->subdivideFacesButton->setDescription("Splits selected faces into 4.");

        this->subdivideAllButton->addUserData(GameObject::AttrActionExec());
        this->subdivideAllButton->addUserData(GameObject::AttrActionExecId(), MeshEditComponent::ActionSubdivideAll());
        this->subdivideAllButton->addUserData(GameObject::AttrActionNeedRefresh());
        this->subdivideAllButton->setDescription("Subdivides entire mesh.");

        this->extrudeAmount->setConstraints(0.001f, 100.0f);
        this->extrudeAmount->setDescription("Distance for extrusion.");

        this->extrudeFacesButton->addUserData(GameObject::AttrActionExec());
        this->extrudeFacesButton->addUserData(GameObject::AttrActionExecId(), MeshEditComponent::ActionExtrudeFaces());
        this->extrudeFacesButton->addUserData(GameObject::AttrActionNeedRefresh());
        this->extrudeFacesButton->setDescription("Extrudes selected faces.");

        this->mirrorAxis->setDescription("Axis for mirroring mesh.");

        this->mirrorMeshButton->addUserData(GameObject::AttrActionExec());
        this->mirrorMeshButton->addUserData(GameObject::AttrActionExecId(), MeshEditComponent::ActionMirrorMesh());
        this->mirrorMeshButton->addUserData(GameObject::AttrActionNeedRefresh());
        this->mirrorMeshButton->setDescription("Mirrors mesh and welds seam.");

        this->applyScaleButton->addUserData(GameObject::AttrActionExec());
        this->applyScaleButton->addUserData(GameObject::AttrActionExecId(), MeshEditComponent::ActionApplyScale());
        this->applyScaleButton->addUserData(GameObject::AttrActionNeedRefresh());
        this->applyScaleButton->setDescription("Applies node scale to vertices.");

        this->buildFaceButton->addUserData(GameObject::AttrActionExec());
        this->buildFaceButton->addUserData(GameObject::AttrActionExecId(), MeshEditComponent::ActionBuildFace());
        this->buildFaceButton->addUserData(GameObject::AttrActionNeedRefresh());
        this->buildFaceButton->setDescription("Creates a triangle (3 vertices selected) or quad (4 vertices -> 2 triangles) "
                                              "from the current vertex selection. Keyboard: F in Vertex mode.");

        this->originAlignment->setDescription("Moves all vertices so the chosen bounding-box point becomes the local origin (0,0,0). "
                                              "Ogre axes: X=right, Y=up, Z=toward viewer. "
                                              "Left/Right=-X/+X  Bottom/Top=-Y/+Y  Front/Back=+Z/-Z.");
        this->originAlignment->addUserData(GameObject::AttrActionNoUndo());

        this->originAlignButton->addUserData(GameObject::AttrActionExec());
        this->originAlignButton->addUserData(GameObject::AttrActionExecId(), MeshEditComponent::ActionApplyOriginAlignment());
        this->originAlignButton->addUserData(GameObject::AttrActionNeedRefresh());
        this->originAlignButton->setDescription("Commits the selected Origin Alignment preset. Pushes one undo record.");

        this->flipDirection->setDescription("Mirrors all vertices across the chosen axis and fixes normals + winding. "
                                            "X=left<->right  Y=bottom<->top  Z=front<->back.");
        this->flipDirection->addUserData(GameObject::AttrActionNoUndo());

        this->flipDirectionButton->addUserData(GameObject::AttrActionExec());
        this->flipDirectionButton->addUserData(GameObject::AttrActionExecId(), MeshEditComponent::ActionApplyFlipDirection());
        this->flipDirectionButton->addUserData(GameObject::AttrActionNeedRefresh());
        this->flipDirectionButton->setDescription("Commits the selected Flip Direction. Pushes one undo record.");

        this->showOrigin->setDescription("Draws an origin marker and direction arrow at the local mesh origin (0,0,0). "
                                         "The arrow shaft points along the scene node forward direction (+Z). "
                                         "Cyan dot = origin, cyan arrow = forward, yellow stub = up (+Y).");
        this->showOrigin->addUserData(GameObject::AttrActionNoUndo());

        this->applyMeshButton->addUserData(GameObject::AttrActionExec());
        this->applyMeshButton->addUserData(GameObject::AttrActionExecId(), MeshEditComponent::ActionApplyMesh());
        this->applyMeshButton->addUserData(GameObject::AttrActionNeedRefresh());
        this->applyMeshButton->setDescription("Exports edited mesh to 'Procedural' resource group, so that other projects also can load that mesh.");

        this->cancelEditButton->addUserData(GameObject::AttrActionExec());
        this->cancelEditButton->addUserData(GameObject::AttrActionExecId(), MeshEditComponent::ActionCancelEdit());
        this->cancelEditButton->addUserData(GameObject::AttrActionNeedRefresh());
        this->cancelEditButton->setDescription("Discards all edits.");
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
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrExtrudeAmount())
        {
            this->extrudeAmount->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.1f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrMirrorAxis())
        {
            this->mirrorAxis->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", "+X"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrOriginAlignment())
        {
            this->originAlignment->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", "Bottom Center"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrFlipDirection())
        {
            this->flipDirection->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", "X"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrShowOrigin())
        {
            this->showOrigin->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
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
        if (false == brushList.empty())
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

        // If its fresh new component, check if existing item does exist and skip
        Ogre::Item* existingItem = this->gameObjectPtr->getMovableObject<Ogre::Item>();
        if (nullptr != existingItem)
        {
            this->originalMeshName = existingItem->getMesh()->getName();

            if (this->outputFileName->getString().empty())
            {
                this->outputFileName->setValue(this->buildDefaultOutputName());
            }
        }
        else
        {
            if (this->outputFileName->getString().empty())
            {
                this->outputFileName->setValue(this->buildDefaultOutputName());
            }

            Ogre::String savedPath = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getSceneName();
            Ogre::String savedMeshName = this->outputFileName->getString() + ".mesh";

            // Prüfe ob die Datei existiert
            std::ifstream testFile(savedPath + "/" + savedMeshName);
            if (true == testFile.good())
            {
                testFile.close();
                // Lade die gespeicherte Mesh statt der Original-Mesh
                Ogre::String savedMeshName = this->outputFileName->getString() + ".mesh";

                bool reloaded = false;
                NOWA::GraphicsModule::RenderCommand loadCmd = [this, savedPath, savedMeshName, &reloaded]()
                {
                    Ogre::VaoManager* vaoManager = Ogre::Root::getSingletonPtr()->getRenderSystem()->getVaoManager();

                    Ogre::MeshPtr savedMesh;
                    try
                    {
                        savedMesh = Core::getSingletonPtr()->loadMeshFromAbsolutePath(savedPath, savedMeshName, vaoManager);
                    }
                    catch (const Ogre::Exception& e)
                    {
                        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MeshEditComponent] " + e.getDescription());
                        return;
                    }

                    if (false == savedMesh.isNull())
                    {
                        // Ersetze das Item mit der gespeicherten Mesh
                        Ogre::Item* savedItem = this->gameObjectPtr->getSceneManager()->createItem(savedMesh, this->gameObjectPtr->isDynamic() ? Ogre::SCENE_DYNAMIC : Ogre::SCENE_STATIC);

                        if (nullptr == savedItem || savedItem->getNumSubItems() == 0)
                        {
                            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                                "[MeshEditComponent] Could not load mesh: " + savedMeshName + ", because its mesh data is coruppt and there is no sub item. For game object: " + this->gameObjectPtr->getName());
                            return;
                        }

                        savedItem->setName(this->gameObjectPtr->getName());

                        this->originalMeshName = savedItem->getMesh()->getName();
                        this->editableItem = savedItem;

                        this->gameObjectPtr->init(savedItem);

                        reloaded = true;
                        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MeshEditComponent] Loaded saved mesh: " + savedMeshName);
                    }
                };
                NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(loadCmd), "MeshEditComponent::postInit::loadSaved");
            }
        }

        // ── Build editable mesh ───────────────────────────────────────────────
        if (false == this->prepareEditableMesh())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MeshEditComponent] prepareEditableMesh failed for: " + this->gameObjectPtr->getName());
            return false;
        }

        if (!this->gameObjectPtr->getDatablockNames().empty())
        {
            NOWA::GraphicsModule::RenderCommand applyCmd = [this]()
            {
                if (!this->editableItem)
                {
                    return;
                }

                const auto& dbNames = this->gameObjectPtr->getDatablockNames();
                for (size_t i = 0; i < dbNames.size() && i < this->editableItem->getNumSubItems(); ++i)
                {
                    const Ogre::String& dbName = dbNames[i];
                    if (dbName.empty() || dbName == "Missing")
                    {
                        continue;
                    }

                    Ogre::HlmsDatablock* db = Ogre::Root::getSingleton().getHlmsManager()->getDatablockNoDefault(dbName);
                    if (db)
                    {
                        this->editableItem->getSubItem(i)->setDatablock(db);
                        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MeshEditComponent] Applied datablock '" + dbName + "' to sub-item " + Ogre::StringConverter::toString(i));
                    }
                    else
                    {
                        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MeshEditComponent] Datablock not found: '" + dbName + "'");
                    }
                }
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(applyCmd), "MeshEditComponent::postInit::applyDatablocks");
        }

        // Auto-weld duplicate vertices to eliminate z-fighting from flat-shaded meshes
        // this->mergeByDistance(1e-5f);

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
        // this->createStatusOverlay();

        Ogre::Camera* camera = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
        this->rectSelector.init(this->gameObjectPtr->getSceneManager(), camera);

        // Do NOT add input listener here — updateModificationState() will do it
        // once the editor fires EDITOR_MESH_MODIFY_MODE and selects this object.

        return true;
    }

    // =========================================================================
    //  connect / disconnect
    // =========================================================================

    bool MeshEditComponent::connect(void)
    {
        GameObjectComponent::connect();

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
        GameObjectComponent::disconnect();

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

        this->writeMesh();

        this->removeInputListener();
        this->destroyOverlay();
        // this->destroyStatusOverlay();

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

        // editableItem and editableMesh stay alive — they ARE the GameObject's mesh now.
        this->editableItem = nullptr;
        this->editableMesh.reset(); // releases our shared_ptr; Ogre still owns the mesh

        this->rectSelector.destroy();
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
        MeshEditComponentPtr meshEditComponentPtr(boost::make_shared<MeshEditComponent>());
        meshEditComponentPtr->setActivated(this->activated->getBool());
        meshEditComponentPtr->setEditMode(this->getEditModeString());
        meshEditComponentPtr->setBrushSize(this->brushSize->getReal());
        meshEditComponentPtr->setBrushIntensity(this->brushIntensity->getReal());
        meshEditComponentPtr->setBrushFalloff(this->brushFalloff->getReal());
        meshEditComponentPtr->setBrushMode(this->getBrushModeString());
        meshEditComponentPtr->setOutputFileName(this->outputFileName->getString());
        clonedGameObjectPtr->addComponent(meshEditComponentPtr);
        meshEditComponentPtr->setOwner(clonedGameObjectPtr);
        GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(meshEditComponentPtr));
        return meshEditComponentPtr;
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

        // ── Keys active only while grabbing ──────────────────────────────────────
        if (this->isGrabbing)
        {
            if (evt.key == OIS::KC_X)
            {
                this->grabAxis = (this->grabAxis == GrabAxis::X) ? GrabAxis::FREE : GrabAxis::X;
                return false;
            }
            if (evt.key == OIS::KC_Y)
            {
                this->grabAxis = (this->grabAxis == GrabAxis::Z) ? GrabAxis::FREE : GrabAxis::Z;
                return false;
            }
            if (evt.key == OIS::KC_Z)
            {
                this->grabAxis = (this->grabAxis == GrabAxis::Y) ? GrabAxis::FREE : GrabAxis::Y;
                return false;
            }
            if (evt.key == OIS::KC_S)
            {
                this->isScaling = !this->isScaling;
                this->grabScaleMouseAccum = 0;
                if (this->isScaling)
                {
                    this->grabAxis = GrabAxis::FREE;
                }
                return false;
            }
        }

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

        if (evt.key == OIS::KC_RETURN && this->isGrabbing)
        {
            this->confirmGrab();
            return false;
        }

        // G : begin grab
        if (evt.key == OIS::KC_G && !this->isGrabbing)
        {
            const bool hasSel = !this->selectedVertices.empty() || !this->selectedEdges.empty() || !this->selectedFaces.empty();
            if (hasSel)
            {
                this->beginGrab();
            }
            return false;
        }

        // X : delete selected
        if (evt.key == OIS::KC_X && !this->isGrabbing)
        {
            this->deleteSelected();
            return false;
        }

        // Ctrl+A : select all / deselect all  (before plain A)
        if (evt.key == OIS::KC_A && this->isCtrlPressed && !this->isGrabbing)
        {
            const bool hasSel = !this->selectedVertices.empty() || !this->selectedEdges.empty() || !this->selectedFaces.empty();
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

        // Ctrl+B : bevel  (before plain B)
        if (evt.key == OIS::KC_B && this->isCtrlPressed && !this->isGrabbing)
        {
            this->bevel(this->bevelAmount->getReal());
            return false;
        }

        // B : toggle brush
        if (evt.key == OIS::KC_B && !this->isGrabbing)
        {
            this->isBrushArmed = !this->isBrushArmed;
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, Ogre::String("[MeshEditComponent] Brush ") + (this->isBrushArmed ? "ARMED" : "disarmed"));
            return false;
        }

        // E : extrude faces (Face mode only)
        if (evt.key == OIS::KC_E && !this->isGrabbing)
        {
            if (this->getEditMode() == EditMode::FACE)
            {
                this->extrudeSelectedFaces(this->extrudeAmount->getReal());
            }
            return false;
        }

        // Ctrl+I : subdivide all  |  I : subdivide selected
        if (evt.key == OIS::KC_I && !this->isGrabbing)
        {
            if (this->isCtrlPressed)
            {
                this->subdivideAll();
            }
            else
            {
                this->subdivideSelectedFaces();
            }
            return false;
        }

        // Ctrl+N : recalculate normals
        if (evt.key == OIS::KC_N && this->isCtrlPressed && !this->isGrabbing)
        {
            this->recalculateNormals();
            return false;
        }

        // Ctrl+R : loop cut
        if (evt.key == OIS::KC_R && this->isCtrlPressed && !this->isGrabbing)
        {
            this->loopCut(this->loopCutFraction->getReal());
            return false;
        }

        // Ctrl+D : dissolve selected
        if (evt.key == OIS::KC_D && this->isCtrlPressed && !this->isGrabbing)
        {
            this->dissolveSelected();
            return false;
        }

        // F key — priority: Ctrl+F (fill) > F+Vertex+3sel (build face) > plain F (flip normals)
        if (evt.key == OIS::KC_F && !this->isGrabbing)
        {
            if (this->isCtrlPressed)
            {
                this->fillSelected();
            }
            else if (this->getEditMode() == EditMode::VERTEX && this->selectedVertices.size() >= 3)
            {
                // Need at least 3 vertices to form a face
                this->buildFaceFromSelection();
            }
            else
            {
                this->flipNormals();
            }
            return false;
        }

        // M : merge selected to centroid
        if (evt.key == OIS::KC_M && !this->isGrabbing)
        {
            this->mergeSelected();
            return false;
        }

        // O : toggle proportional editing
        if (evt.key == OIS::KC_O && !this->isGrabbing)
        {
            this->isProportionalEditing = !this->isProportionalEditing;
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, Ogre::String("[MeshEditComponent] Proportional editing: ") + (this->isProportionalEditing ? "ON" : "OFF"));
            return false;
        }

        // L : select linked
        if (evt.key == OIS::KC_L && !this->isGrabbing)
        {
            const OIS::MouseState& ms = NOWA::InputDeviceCore::getSingletonPtr()->getMouse()->getMouseState();
            Ogre::Real sx = 0, sy = 0;
            MathHelper::getInstance()->mouseToViewPort(ms.X.abs, ms.Y.abs, sx, sy, Core::getSingletonPtr()->getOgreRenderWindow());
            this->selectLinked(sx, sy, this->isShiftPressed);
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

        if (this->isEditorMeshModifyMode)
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

        const bool add = this->isShiftPressed;
        const bool rem = this->isCtrlPressed;

        Ogre::Real sx = 0, sy = 0;
        MathHelper::getInstance()->mouseToViewPort(evt.state.X.abs, evt.state.Y.abs, sx, sy, Core::getSingletonPtr()->getOgreRenderWindow());

        this->rectPressX = evt.state.X.abs;
        this->rectPressY = evt.state.Y.abs;

        const EditMode mode = this->getEditMode();

        // ── VERTEX ────────────────────────────────────────────────────────────────
        if (mode == EditMode::VERTEX)
        {
            // Brush path
            if (this->isBrushArmed)
            {
                Ogre::Vector3 hp, hn;
                size_t ht;
                if (this->raycastMesh(sx, sy, hp, hn, ht))
                {
                    this->isPressing = true;
                    this->brushLastHitPos = hp;
                    this->brushStrokePreEditData = this->getMeshData();
                    this->applyBrush(hp, rem);
                }
                return false;
            }

            // Normal vertex picking
            size_t vIdx;
            if (this->pickVertex(sx, sy, vIdx))
            {
                const Ogre::Vector3& pickedPos = this->vertices[vIdx];
                constexpr float kEps2 = 1e-8f;

                if (rem)
                {
                    for (size_t i = 0; i < this->vertexCount; ++i)
                    {
                        if (this->vertices[i].squaredDistance(pickedPos) <= kEps2)
                        {
                            this->selectedVertices.erase(i);
                        }
                    }
                }
                else
                {
                    if (!add)
                    {
                        this->selectedVertices.clear();
                    }
                    for (size_t i = 0; i < this->vertexCount; ++i)
                    {
                        if (this->vertices[i].squaredDistance(pickedPos) <= kEps2)
                        {
                            this->selectedVertices.insert(i);
                        }
                    }
                }
                this->scheduleOverlayUpdate();
                this->isPressing = true;
            }
            else
            {
                // Nothing hit — prepare rect select
                this->isPressing = true;
            }
        }
        // ── EDGE ──────────────────────────────────────────────────────────────────
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
            this->isPressing = true;
        }
        // ── FACE ──────────────────────────────────────────────────────────────────
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
            this->isPressing = true;
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

        Ogre::Real sx = 0, sy = 0;
        MathHelper::getInstance()->mouseToViewPort(evt.state.X.abs, evt.state.Y.abs, sx, sy, Core::getSingletonPtr()->getOgreRenderWindow());

        // ── Grab drag ─────────────────────────────────────────────────────────────
        if (this->isGrabbing)
        {
            this->applyGrabMouseDelta(evt.state.X.rel, evt.state.Y.rel);
            return false;
        }

        // ── Brush stroke drag ─────────────────────────────────────────────────────
        if (this->isBrushArmed && this->isPressing && this->getEditMode() == EditMode::VERTEX)
        {
            Ogre::Vector3 hp, hn;
            size_t ht;
            if (this->raycastMesh(sx, sy, hp, hn, ht))
            {
                const Ogre::Real minDist = this->brushSize->getReal() * 0.1f;
                if (hp.distance(this->brushLastHitPos) >= minDist)
                {
                    this->brushLastHitPos = hp;
                    this->applyBrush(hp, this->isCtrlPressed);
                }
            }
            return false;
        }

        // ── Rectangle select drag ─────────────────────────────────────────────────
        // Guard: skip if Add Vertex mode is active (those clicks are not selections)
        if (this->isPressing && !this->isBrushArmed)
        {
            const float dxPx = std::abs(evt.state.X.abs - this->rectPressX);
            const float dyPx = std::abs(evt.state.Y.abs - this->rectPressY);

            if (!this->rectSelector.isDragging() && (dxPx > this->rectDragThreshold || dyPx > this->rectDragThreshold))
            {
                float sx0 = 0, sy0 = 0;
                MathHelper::getInstance()->mouseToViewPort(static_cast<int>(this->rectPressX), static_cast<int>(this->rectPressY), sx0, sy0, Core::getSingletonPtr()->getOgreRenderWindow());
                this->rectSelector.beginDrag(sx0, sy0);
            }

            if (this->rectSelector.isDragging())
            {
                this->rectSelector.updateDrag(sx, sy);
                return false;
            }
        }

        return true;
    }

    bool MeshEditComponent::mouseReleased(const OIS::MouseEvent&, OIS::MouseButtonID id)
    {
        struct VertexRectAdapter : public NOWA::IScreenRectSelectable
        {
            MeshEditComponent& comp;
            VertexRectAdapter(MeshEditComponent& c) : comp(c)
            {
            }

            size_t getElementCount() const override
            {
                return comp.vertexCount;
            }

            bool projectElement(size_t idx, float& nx, float& ny) const override
            {
                return comp.projectVertexToScreen(idx, nx, ny);
            }

            void applyRectSelection(const std::vector<size_t>& ids, bool add, bool remove) override
            {
                if (!add && !remove)
                {
                    comp.selectedVertices.clear();
                }
                for (size_t i : ids)
                {
                    if (remove)
                    {
                        comp.selectedVertices.erase(i);
                    }
                    else
                    {
                        comp.selectedVertices.insert(i);
                    }
                }
                comp.scheduleOverlayUpdate();
            }
        };

        struct EdgeRectAdapter : public NOWA::IScreenRectSelectable
        {
            MeshEditComponent& comp;
            std::vector<MeshEditComponent::EdgeKey> edgeList;

            EdgeRectAdapter(MeshEditComponent& c) : comp(c)
            {
                std::set<std::pair<size_t, size_t>> seen;
                for (size_t i = 0; i < c.indexCount; i += 3)
                {
                    for (int e = 0; e < 3; ++e)
                    {
                        size_t a = c.indices[i + e], b = c.indices[i + (e + 1) % 3];
                        if (seen.insert({std::min(a, b), std::max(a, b)}).second)
                        {
                            edgeList.push_back(MeshEditComponent::EdgeKey(a, b));
                        }
                    }
                }
            }

            size_t getElementCount() const override
            {
                return edgeList.size();
            }

            bool projectElement(size_t idx, float& nx, float& ny) const override
            {
                float ax, ay, bx, by;
                bool okA = comp.projectVertexToScreen(edgeList[idx].a, ax, ay);
                bool okB = comp.projectVertexToScreen(edgeList[idx].b, bx, by);
                if (!okA && !okB)
                {
                    return false;
                }
                if (!okA)
                {
                    nx = bx;
                    ny = by;
                    return true;
                }
                if (!okB)
                {
                    nx = ax;
                    ny = ay;
                    return true;
                }
                nx = (ax + bx) * 0.5f;
                ny = (ay + by) * 0.5f;
                return true;
            }

            void applyRectSelection(const std::vector<size_t>& ids, bool add, bool remove) override
            {
                if (!add && !remove)
                {
                    comp.selectedEdges.clear();
                }
                for (size_t i : ids)
                {
                    if (remove)
                    {
                        comp.selectedEdges.erase(edgeList[i]);
                    }
                    else
                    {
                        comp.selectedEdges.insert(edgeList[i]);
                    }
                }
                comp.scheduleOverlayUpdate();
            }
        };

        struct FaceRectAdapter : public NOWA::IScreenRectSelectable
        {
            MeshEditComponent& comp;
            FaceRectAdapter(MeshEditComponent& c) : comp(c)
            {
            }

            size_t getElementCount() const override
            {
                return comp.indexCount / 3;
            }

            bool projectElement(size_t idx, float& nx, float& ny) const override
            {
                size_t i0 = comp.indices[idx * 3], i1 = comp.indices[idx * 3 + 1], i2 = comp.indices[idx * 3 + 2];
                float ax, ay, bx, by, cx, cy;
                bool okA = comp.projectVertexToScreen(i0, ax, ay);
                bool okB = comp.projectVertexToScreen(i1, bx, by);
                bool okC = comp.projectVertexToScreen(i2, cx, cy);
                int cnt = (int)okA + (int)okB + (int)okC;
                if (cnt == 0)
                {
                    return false;
                }
                nx = ((okA ? ax : 0.f) + (okB ? bx : 0.f) + (okC ? cx : 0.f)) / cnt;
                ny = ((okA ? ay : 0.f) + (okB ? by : 0.f) + (okC ? cy : 0.f)) / cnt;
                return true;
            }

            void applyRectSelection(const std::vector<size_t>& ids, bool add, bool remove) override
            {
                if (!add && !remove)
                {
                    comp.selectedFaces.clear();
                }
                for (size_t i : ids)
                {
                    if (remove)
                    {
                        comp.selectedFaces.erase(i);
                    }
                    else
                    {
                        comp.selectedFaces.insert(i);
                    }
                }
                comp.scheduleOverlayUpdate();
            }
        };

        if (id == OIS::MB_Left)
        {
            // ── Finish brush stroke ──────────────────────────────────────────────
            if (this->isPressing && this->isBrushArmed && !this->brushStrokePreEditData.empty())
            {
                this->fireUndoEvent(this->brushStrokePreEditData);
                this->brushStrokePreEditData.clear();
            }

            // ── Finish rectangle select ──────────────────────────────────────────
            if (this->rectSelector.isDragging())
            {
                const bool add = this->isShiftPressed;
                const bool rem = this->isCtrlPressed;

                switch (this->getEditMode())
                {
                case EditMode::VERTEX:
                {
                    VertexRectAdapter adapter(*this);
                    this->rectSelector.endDrag(adapter, add, rem);
                    break;
                }
                case EditMode::EDGE:
                {
                    EdgeRectAdapter adapter(*this);
                    this->rectSelector.endDrag(adapter, add, rem);
                    break;
                }
                case EditMode::FACE:
                {
                    FaceRectAdapter adapter(*this);
                    this->rectSelector.endDrag(adapter, add, rem);
                    break;
                }
                default:
                    this->rectSelector.cancelDrag();
                    break;
                }
            }

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

        // One-time: remove any tracked closure that may have been registered by
        // a previous version of this code.  GraphicsModule silently ignores unknown ids.
        if (!this->overlayClosureRemoved)
        {
            const Ogre::String closureId = this->gameObjectPtr->getName() + "_MeshEditOverlay_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
            NOWA::GraphicsModule::getInstance()->removeTrackedClosure(closureId);
            this->overlayClosureRemoved = true;
        }
        // All overlay rebuilds are issued by scheduleOverlayUpdate().
        // Nothing else to do here every frame.
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

        if (MeshEditComponent::ActionWeldVertices() == actionId)
        {
            this->mergeByDistance(1e-5f);
            return true;
        }

        if (MeshEditComponent::ActionFlipNormals() == actionId)
        {
            this->flipNormals();
            return true;
        }

        if (MeshEditComponent::ActionRecalcNormals() == actionId)
        {
            this->recalculateNormals();
            return true;
        }

        if (MeshEditComponent::ActionSubdivideFaces() == actionId)
        {
            this->subdivideSelectedFaces();
            return true;
        }

        if (MeshEditComponent::ActionSubdivideAll() == actionId)
        {
            this->subdivideAll();
            return true;
        }

        if (MeshEditComponent::ActionExtrudeFaces() == actionId)
        {
            this->extrudeSelectedFaces(this->extrudeAmount->getReal());
            return true;
        }

        if (MeshEditComponent::ActionMirrorMesh() == actionId)
        {
            this->mirrorMesh(this->mirrorAxis->getListSelectedValue());
            return true;
        }

        if (MeshEditComponent::ActionApplyScale() == actionId)
        {
            this->applyScale();
            return true;
        }

        if (MeshEditComponent::ActionMergeSelected() == actionId)
        {
            this->mergeSelected();
            return true;
        }
        if (MeshEditComponent::ActionDissolveSelected() == actionId)
        {
            this->dissolveSelected();
            return true;
        }
        if (MeshEditComponent::ActionFillSelected() == actionId)
        {
            this->fillSelected();
            return true;
        }
        if (MeshEditComponent::ActionBevel() == actionId)
        {
            this->bevel(this->bevelAmount->getReal());
            return true;
        }
        if (MeshEditComponent::ActionLoopCut() == actionId)
        {
            this->loopCut(this->loopCutFraction->getReal());
            return true;
        }
        if (MeshEditComponent::ActionBuildFace() == actionId)
        {
            this->buildFaceFromSelection();
            return true;
        }
        if (MeshEditComponent::ActionApplyOriginAlignment() == actionId)
        {
            this->applyOriginAlignment();
            return true;
        }
        if (MeshEditComponent::ActionApplyFlipDirection() == actionId)
        {
            this->applyFlipDirection();
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
        else if (AttrExtrudeAmount() == attribute->getName())
        {
            this->setExtrudeAmount(attribute->getReal());
        }
        else if (AttrMirrorAxis() == attribute->getName())
        {
            this->setMirrorAxis(attribute->getListSelectedValue());
        }
        else if (AttrProportionalRadius() == attribute->getName())
        {
            this->setProportionalRadius(attribute->getReal());
        }
        else if (AttrBevelAmount() == attribute->getName())
        {
            this->setBevelAmount(attribute->getReal());
        }
        else if (AttrLoopCutFraction() == attribute->getName())
        {
            this->setLoopCutFraction(attribute->getReal());
        }
        else if (AttrOriginAlignment() == attribute->getName())
        {
            this->setOriginAlignment(attribute->getListSelectedValue());
        }
        else if (AttrFlipDirection() == attribute->getName())
        {
            this->setFlipDirection(attribute->getListSelectedValue());
        }
        else if (AttrShowOrigin() == attribute->getName())
        {
            this->showOrigin->setValue(attribute->getBool());
            this->scheduleOverlayUpdate();
        }
    }

    // =========================================================================
    //  writeXML
    // =========================================================================

    void MeshEditComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
    {
        this->writeMesh();

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
        add("12", AttrShowWireframe(), XMLConverter::ConvertString(doc, this->showWireframe->getBool()));
        add("12", AttrXRayOverlay(), XMLConverter::ConvertString(doc, this->xRayOverlay->getBool()));
        add("6", AttrVertexMarkerSize(), XMLConverter::ConvertString(doc, this->vertexMarkerSize->getReal()));
        add("7", AttrOutputFileName(), XMLConverter::ConvertString(doc, this->outputFileName->getString()));
        add("7", AttrBrushName(), XMLConverter::ConvertString(doc, this->brushName->getListSelectedValue()));
        add("6", AttrBrushSize(), XMLConverter::ConvertString(doc, this->brushSize->getReal()));
        add("6", AttrBrushIntensity(), XMLConverter::ConvertString(doc, this->brushIntensity->getReal()));
        add("6", AttrBrushFalloff(), XMLConverter::ConvertString(doc, this->brushFalloff->getReal()));
        add("7", AttrBrushMode(), XMLConverter::ConvertString(doc, this->brushMode->getListSelectedValue()));
        add("6", AttrExtrudeAmount(), XMLConverter::ConvertString(doc, this->extrudeAmount->getReal()));
        add("7", AttrMirrorAxis(), XMLConverter::ConvertString(doc, this->mirrorAxis->getListSelectedValue()));
        add("7", AttrOriginAlignment(), XMLConverter::ConvertString(doc, this->originAlignment->getListSelectedValue()));
        add("7", AttrFlipDirection(), XMLConverter::ConvertString(doc, this->flipDirection->getListSelectedValue()));
        add("12", AttrShowOrigin(), XMLConverter::ConvertString(doc, this->showOrigin->getBool()));
    }

    void MeshEditComponent::writeMesh()
    {
        if (this->editableItem && this->editableMesh && !this->outputFileName->getString().empty())
        {
            this->gameObjectPtr->actualizeDatablocks();

            Ogre::String fullPath = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getSceneName() + "/" + this->outputFileName->getString() + ".mesh";

            NOWA::GraphicsModule::RenderCommand cmd = [this, fullPath]()
            {
                try
                {
                    Ogre::MeshSerializer serializer(Ogre::Root::getSingletonPtr()->getRenderSystem()->getVaoManager());
                    serializer.exportMesh(this->editableMesh.get(), fullPath, Ogre::MESH_VERSION_LATEST, Ogre::Serializer::ENDIAN_NATIVE);
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MeshEditComponent] Auto-saved mesh to: " + fullPath);
                }
                catch (Ogre::Exception& e)
                {
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MeshEditComponent] Auto-save failed: " + e.getDescription());
                }
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "MeshEditComponent::writeMesh::export");
        }
    }

    // =========================================================================
    //  Output / Apply
    // =========================================================================

    Ogre::String MeshEditComponent::buildDefaultOutputName(void) const
    {
        Ogre::String base = this->originalMeshName;
        const size_t dot = base.rfind('.');
        if (dot != Ogre::String::npos)
        {
            base = base.substr(0, dot);
        }

        // Append GO id — guarantees uniqueness across all instances of the same mesh.
        // Case3.mesh on GameObject id=99887766 -> "Case3_edit_99887766"
        return base + "_edit_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
    }

    std::vector<unsigned char> MeshEditComponent::getMeshData(void) const
    {
        // Layout: [vertexCount:uint32][indexCount:uint32]
        //         [float3 * vertexCount pos][float3 * vertexCount normals]
        //         [float4 * vertexCount tangents][float2 * vertexCount uvs]
        //         [uint32 * indexCount indices]
        std::vector<unsigned char> buf;
        auto write = [&](const void* p, size_t n)
        {
            const unsigned char* b = reinterpret_cast<const unsigned char*>(p);
            buf.insert(buf.end(), b, b + n);
        };
        uint32_t vc = static_cast<uint32_t>(this->vertexCount);
        uint32_t ic = static_cast<uint32_t>(this->indexCount);
        write(&vc, 4);
        write(&ic, 4);
        write(this->vertices.data(), vc * sizeof(Ogre::Vector3));
        write(this->normals.data(), vc * sizeof(Ogre::Vector3));
        write(this->tangents.data(), vc * sizeof(Ogre::Vector4));
        write(this->uvCoordinates.data(), vc * sizeof(Ogre::Vector2));
        write(this->indices.data(), ic * sizeof(Ogre::uint32));
        return buf;
    }

    void MeshEditComponent::setMeshData(const std::vector<unsigned char>& data)
    {
        if (data.size() < 8)
        {
            return;
        }
        const unsigned char* p = data.data();
        auto read = [&](void* dst, size_t n)
        {
            std::memcpy(dst, p, n);
            p += n;
        };

        uint32_t vc = 0, ic = 0;
        read(&vc, 4);
        read(&ic, 4);
        this->vertexCount = vc;
        this->indexCount = ic;
        this->vertices.resize(vc);
        this->normals.resize(vc);
        this->tangents.resize(vc, Ogre::Vector4(1, 0, 0, 1));
        this->uvCoordinates.resize(vc);
        this->indices.resize(ic);
        read(this->vertices.data(), vc * sizeof(Ogre::Vector3));
        read(this->normals.data(), vc * sizeof(Ogre::Vector3));
        read(this->tangents.data(), vc * sizeof(Ogre::Vector4));
        read(this->uvCoordinates.data(), vc * sizeof(Ogre::Vector2));
        read(this->indices.data(), ic * sizeof(Ogre::uint32));
        this->deselectAll();
        this->buildVertexAdjacency();
        this->rebuildDynamicBuffers();
        this->scheduleOverlayUpdate();
    }

    void MeshEditComponent::fireUndoEvent(const std::vector<unsigned char>& oldData)
    {
        // Also push to internal stack as a backup / for the brush (no topology change)
        // The EditorManager stack handles the main undo; keep internal for brush continuity
        std::vector<unsigned char> newData = this->getMeshData();
        auto evt = boost::make_shared<EventDataCommandTransactionBegin>("Mesh Edit");
        AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(evt);
        auto evtMod = boost::make_shared<EventDataMeshEditModifyEnd>(oldData, newData, this->gameObjectPtr->getId());
        AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(evtMod);
        auto evtEnd = boost::make_shared<EventDataCommandTransactionEnd>();
        AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(evtEnd);
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

        boost::shared_ptr<NOWA::EventDataRefreshMeshResources> eventDataRefreshMeshResources(new NOWA::EventDataRefreshMeshResources());
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataRefreshMeshResources);

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

    // =============================================================================
    //  MeshEditComponent — Mesh Naming Fix
    //
    //  ROOT CAUSE
    //  ----------
    //  createDynamicBuffers() named the editable mesh "Case3_2_MeshEdit_2241755060"
    //  (an internal RAM-only name).  DotSceneExportModule reads item->getMesh()->getName()
    //  and writes meshFile="Case3_2_MeshEdit_2241755060" into the .scene XML.
    //
    //  On the SECOND reload:
    //    1. DotSceneImportModule tries to load "Case3_2_MeshEdit_2241755060" as a file
    //       -> file does not exist -> Ogre registers the name as a manual mesh (warning)
    //    2. postInit() calls createDynamicBuffers() which calls createManual with the
    //       same name -> ItemIdentityException -> component fails to init.
    //
    //  FIX
    //  ---
    //  Name the editable mesh after outputFileName ("Case3_edit_2241755060") — the
    //  SAME name as the exported .mesh file.  DotScene then always saves the correct
    //  file reference.  Before createManual we remove any existing entry under that
    //  name (which may be the file-loaded version from the previous session).
    //
    //  REPLACE createDynamicBuffers() and rebuildDynamicBuffers() with these.
    // =============================================================================

    bool MeshEditComponent::createDynamicBuffers(void)
    {
        Ogre::VaoManager* vm = Ogre::Root::getSingletonPtr()->getRenderSystem()->getVaoManager();
        Ogre::Item* srcItem = this->gameObjectPtr->getMovableObject<Ogre::Item>();

        // Use outputFileName as the mesh name so DotScene saves the correct file
        // reference.  Fall back to the internal name if outputFileName is not set yet.
        Ogre::String meshName = this->outputFileName->getString();
        if (meshName.empty())
        {
            meshName = this->gameObjectPtr->getName() + "_MeshEdit_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
        }

        // Remove any mesh already registered under this name.
        // On a scene reload the DotSceneImportModule may have loaded the .mesh file
        // from disk under this exact name.  We replace it with our dynamic version.
        {
            Ogre::ResourcePtr existing = Ogre::MeshManager::getSingleton().getByName(meshName, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
            if (!existing.isNull())
            {
                Ogre::MeshManager::getSingleton().remove(existing->getHandle());
            }
        }

        this->editableMesh = Ogre::MeshManager::getSingleton().createManual(meshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
        this->editableMesh->_setVaoManager(vm);

        Ogre::Vector3 minBB(std::numeric_limits<float>::max());
        Ogre::Vector3 maxBB(-std::numeric_limits<float>::max());

        for (size_t smIdx = 0; smIdx < this->subMeshInfoList.size(); ++smIdx)
        {
            SubMeshInfo& si = this->subMeshInfoList[smIdx];

            // Check whether the datablock requires tangents (normal map present)
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

            const size_t fpv = si.floatsPerVertex;
            const size_t vS = si.vertexOffset;
            const size_t vC = si.vertexCount;

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

            // BT_DYNAMIC_DEFAULT + keepAsShadow=false: we own vd, VaoManager keeps it
            si.dynamicVertexBuffer = vm->createVertexBuffer(elems, vC, Ogre::BT_DYNAMIC_DEFAULT, vd, false);

            const size_t iC = si.indexCount;
            const size_t iS = si.indexOffset;

            Ogre::uint16* id = reinterpret_cast<Ogre::uint16*>(OGRE_MALLOC_SIMD(iC * sizeof(Ogre::uint16), Ogre::MEMCATEGORY_GEOMETRY));
            for (size_t i = 0; i < iC; ++i)
            {
                id[i] = static_cast<Ogre::uint16>(this->indices[iS + i] - vS);
            }

            // BT_DEFAULT + keepAsShadow=true: Ogre owns id, do NOT free here
            si.dynamicIndexBuffer = vm->createIndexBuffer(Ogre::IndexBufferPacked::IT_16BIT, iC, Ogre::BT_DEFAULT, id, true);

            Ogre::VertexBufferPackedVec vbv;
            vbv.push_back(si.dynamicVertexBuffer);
            Ogre::VertexArrayObject* vao = vm->createVertexArrayObject(vbv, si.dynamicIndexBuffer, Ogre::OT_TRIANGLE_LIST);

            Ogre::SubMesh* subMesh = this->editableMesh->createSubMesh();
            subMesh->mVao[Ogre::VpNormal].push_back(vao);
            subMesh->mVao[Ogre::VpShadow].push_back(vao);

            // Store the datablock name so MeshSerializer writes it into the .mesh file
            if (srcItem && smIdx < srcItem->getNumSubItems())
            {
                Ogre::HlmsDatablock* db = srcItem->getSubItem(smIdx)->getDatablock();
                if (db && db->getNameStr())
                {
                    subMesh->mMaterialName = *db->getNameStr();
                }
            }
        }

        if (minBB.x > maxBB.x)
        {
            minBB = maxBB = Ogre::Vector3::ZERO;
        }
        Ogre::Aabb bounds;
        bounds.setExtents(minBB, maxBB);
        this->editableMesh->_setBounds(bounds, false);
        this->editableMesh->_setBoundingSphereRadius(bounds.getRadius());

        // editableItem is null until here — no SubItem access allowed above
        this->editableItem = this->gameObjectPtr->getSceneManager()->createItem(this->editableMesh, this->gameObjectPtr->isDynamic() ? Ogre::SCENE_DYNAMIC : Ogre::SCENE_STATIC);
        this->editableItem->setName(this->gameObjectPtr->getName() + "_MeshEdit");

        // Apply datablocks from source item
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

        this->gameObjectPtr->actualizeDatablocks();

        this->dynamicVertexBuffer = this->subMeshInfoList.empty() ? nullptr : this->subMeshInfoList[0].dynamicVertexBuffer;
        this->dynamicIndexBuffer = this->subMeshInfoList.empty() ? nullptr : this->subMeshInfoList[0].dynamicIndexBuffer;

        return true;
    }

    // =============================================================================
    bool MeshEditComponent::rebuildDynamicBuffers(void)
    {
        this->recalculateNormals_internal();
        this->recalculateTangents();

        bool success = false;
        NOWA::GraphicsModule::RenderCommand cmd = [this, &success]()
        {
            Ogre::VaoManager* vm = Ogre::Root::getSingletonPtr()->getRenderSystem()->getVaoManager();

            const bool hasTan = this->vertexFormat.hasTangent;
            const size_t fpv = hasTan ? 12u : 8u;

            // Use outputFileName as the mesh name for the same reason as
            // createDynamicBuffers — keeps DotScene in sync with the exported file.
            // A counter makes successive rebuilds within one session unique so
            // we never hit DUPLICATE_ITEM on the old name before it is removed below.
            const Ogre::String baseName = this->outputFileName->getString().empty() ? (this->gameObjectPtr->getName() + "_MeshEditTmp_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId())) : this->outputFileName->getString();

            const Ogre::String tmpName = baseName + "_tmp_" + Ogre::StringConverter::toString(this->meshRebuildCounter++);

            // Remove any stale entry under this tmp name (should not exist, but guard)
            {
                Ogre::ResourcePtr stale = Ogre::MeshManager::getSingleton().getByName(tmpName, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
                if (!stale.isNull())
                {
                    Ogre::MeshManager::getSingleton().remove(stale->getHandle());
                }
            }

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

            // BT_DYNAMIC_DEFAULT + keepAsShadow=false: we own vd, free after upload
            Ogre::VertexBufferPacked* newVB = vm->createVertexBuffer(elems, this->vertexCount, Ogre::BT_DYNAMIC_DEFAULT, vd, false);
            OGRE_FREE_SIMD(vd, Ogre::MEMCATEGORY_GEOMETRY);

            Ogre::uint32* id = reinterpret_cast<Ogre::uint32*>(OGRE_MALLOC_SIMD(this->indexCount * sizeof(Ogre::uint32), Ogre::MEMCATEGORY_GEOMETRY));
            for (size_t i = 0; i < this->indexCount; ++i)
            {
                id[i] = this->indices[i];
            }

            // BT_DEFAULT + keepAsShadow=true: Ogre owns id, do NOT free here
            Ogre::IndexBufferPacked* newIB = vm->createIndexBuffer(Ogre::IndexBufferPacked::IT_32BIT, this->indexCount, Ogre::BT_DEFAULT, id, true);

            Ogre::VertexBufferPackedVec vbv;
            vbv.push_back(newVB);
            Ogre::VertexArrayObject* newVao = vm->createVertexArrayObject(vbv, newIB, Ogre::OT_TRIANGLE_LIST);

            Ogre::SubMesh* newSM = newMesh->createSubMesh();
            newSM->mVao[Ogre::VpNormal].push_back(newVao);
            newSM->mVao[Ogre::VpShadow].push_back(newVao);

            // Carry material name so future writeMesh() exports it correctly
            if (this->editableItem && this->editableItem->getNumSubItems() > 0)
            {
                Ogre::HlmsDatablock* db = this->editableItem->getSubItem(0)->getDatablock();
                if (db && db->getNameStr())
                {
                    newSM->mMaterialName = *db->getNameStr();
                }
            }

            if (minBB.x > maxBB.x)
            {
                minBB = maxBB = Ogre::Vector3::ZERO;
            }
            Ogre::Aabb bounds;
            bounds.setExtents(minBB, maxBB);
            newMesh->_setBounds(bounds, false);
            newMesh->_setBoundingSphereRadius(bounds.getRadius());

            Ogre::Item* newItem = this->gameObjectPtr->getSceneManager()->createItem(newMesh, this->gameObjectPtr->isDynamic() ? Ogre::SCENE_DYNAMIC : Ogre::SCENE_STATIC);
            newItem->setName(this->gameObjectPtr->getName() + "_MeshEdit");

            // Carry all datablocks from the current item
            if (this->editableItem)
            {
                const size_t n = std::min(this->editableItem->getNumSubItems(), newItem->getNumSubItems());
                for (size_t s = 0; s < n; ++s)
                {
                    Ogre::HlmsDatablock* db = this->editableItem->getSubItem(s)->getDatablock();
                    if (db)
                    {
                        newItem->getSubItem(s)->setDatablock(db);
                    }
                }
            }

            // Atomically swap old item out
            Ogre::MovableObject* oldMovable = nullptr;
            this->gameObjectPtr->swapMovableObject(newItem, oldMovable);
            if (oldMovable)
            {
                this->gameObjectPtr->getSceneManager()->destroyItem(static_cast<Ogre::Item*>(oldMovable));
            }

            // Remove and release the old editable mesh
            if (this->editableMesh)
            {
                Ogre::MeshManager::getSingleton().remove(this->editableMesh);
                this->editableMesh.reset();
            }

            // Rename the tmp mesh to baseName so DotScene saves the right file reference.
            // We do this by re-registering: remove tmp, recreate under baseName.
            // Ogre does not support renaming, so we copy and re-register.
            // Simpler: just keep tmpName for now — writeMesh() exports with outputFileName
            // and postInit() will clear it on next load via the remove-before-create guard.
            this->editableMesh = newMesh;
            this->editableItem = newItem;

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
        // Pack on the CALLING thread (must be main thread, i.e. called before enqueue)
        if (this->subMeshInfoList.empty())
        {
            return;
        }

        // Collect per-submesh upload packets: buffer pointer + packed float data
        struct Packet
        {
            Ogre::VertexBufferPacked* buffer;
            std::vector<float> data;
            size_t vertexCount;
        };
        std::vector<Packet> packets;
        packets.reserve(this->subMeshInfoList.size());

        for (const SubMeshInfo& sm : this->subMeshInfoList)
        {
            if (!sm.dynamicVertexBuffer)
            {
                continue;
            }

            const size_t fpv = sm.floatsPerVertex;
            const size_t vS = sm.vertexOffset;
            const size_t vC = sm.vertexCount;

            Packet pkt;
            pkt.buffer = sm.dynamicVertexBuffer;
            pkt.vertexCount = vC;
            pkt.data.resize(vC * fpv);

            // All reads of this->vertices happen HERE on the main thread — safe
            for (size_t i = 0; i < vC; ++i)
            {
                const size_t gi = vS + i;
                size_t o = i * fpv;
                pkt.data[o++] = this->vertices[gi].x;
                pkt.data[o++] = this->vertices[gi].y;
                pkt.data[o++] = this->vertices[gi].z;
                pkt.data[o++] = this->normals[gi].x;
                pkt.data[o++] = this->normals[gi].y;
                pkt.data[o++] = this->normals[gi].z;
                if (sm.hasTangent)
                {
                    pkt.data[o++] = this->tangents[gi].x;
                    pkt.data[o++] = this->tangents[gi].y;
                    pkt.data[o++] = this->tangents[gi].z;
                    pkt.data[o++] = this->tangents[gi].w;
                }
                pkt.data[o++] = this->uvCoordinates[gi].x;
                pkt.data[o] = this->uvCoordinates[gi].y;
            }
            packets.push_back(std::move(pkt));
        }

        if (packets.empty())
        {
            return;
        }

        // Upload on render thread — main thread blocked by enqueueAndWait -> safe
        NOWA::GraphicsModule::RenderCommand cmd = [packets = std::move(packets)]() mutable
        {
            for (Packet& pkt : packets)
            {
                if (!pkt.buffer)
                {
                    continue;
                }
                float* vd = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(pkt.data.size() * sizeof(float), Ogre::MEMCATEGORY_GEOMETRY));
                std::memcpy(vd, pkt.data.data(), pkt.data.size() * sizeof(float));
                pkt.buffer->upload(vd, 0, pkt.vertexCount);
                OGRE_FREE_SIMD(vd, Ogre::MEMCATEGORY_GEOMETRY);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "MeshEditComponent::uploadVertexData");
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

        std::vector<unsigned char> oldData = this->getMeshData();

        // Step 1: remove all triangles that touch any selected vertex
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

        // Step 2: find which vertices are still referenced after face removal
        std::unordered_set<size_t> usedVerts;
        for (size_t idx : newIdx)
        {
            usedVerts.insert(idx);
        }

        // Step 3: compact vertex arrays, skipping selected AND orphaned vertices
        std::vector<int32_t> remap(this->vertexCount, -1);
        std::vector<Ogre::Vector3> nv, nn;
        std::vector<Ogre::Vector4> nt;
        std::vector<Ogre::Vector2> nu;
        size_t ni = 0;
        for (size_t i = 0; i < this->vertexCount; ++i)
        {
            if (usedVerts.count(i)) // keep only vertices still in use
            {
                remap[i] = static_cast<int32_t>(ni++);
                nv.push_back(this->vertices[i]);
                nn.push_back(this->normals[i]);
                nt.push_back(this->tangents[i]);
                nu.push_back(this->uvCoordinates[i]);
            }
        }

        // Step 4: remap indices to the compacted vertex array
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
        this->fireUndoEvent(oldData);
    }

    void MeshEditComponent::deleteSelectedEdges(void)
    {
        if (this->selectedEdges.empty())
        {
            return;
        }
        std::vector<unsigned char> oldData = this->getMeshData();

        std::vector<Ogre::uint32> newIdx;
        for (size_t i = 0; i < this->indexCount; i += 3)
        {
            EdgeKey e01(this->indices[i], this->indices[i + 1]);
            EdgeKey e12(this->indices[i + 1], this->indices[i + 2]);
            EdgeKey e20(this->indices[i + 2], this->indices[i]);

            int selCount = (int)this->selectedEdges.count(e01) + (int)this->selectedEdges.count(e12) + (int)this->selectedEdges.count(e20);

            if (selCount >= 2) // need ≥2 selected edges on a face to remove it
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
        this->fireUndoEvent(oldData);
    }

    void MeshEditComponent::deleteSelectedFaces(void)
    {
        if (this->selectedFaces.empty())
        {
            return;
        }

        std::vector<unsigned char> oldData = this->getMeshData();

        // Remember which edges bordered the deleted faces — these become the hole rim
        // We collect edges that appear exactly once across the deleted faces
        // (shared between a deleted face and a kept face = new open border)
        std::map<std::pair<size_t, size_t>, int> edgeCount;
        for (size_t t : this->selectedFaces)
        {
            size_t i0 = this->indices[t * 3], i1 = this->indices[t * 3 + 1], i2 = this->indices[t * 3 + 2];
            auto add = [&](size_t a, size_t b)
            {
                edgeCount[{std::min(a, b), std::max(a, b)}]++;
            };
            add(i0, i1);
            add(i1, i2);
            add(i2, i0);
        }

        // Build new index buffer without the deleted faces
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

        // Remove orphaned vertices
        std::unordered_set<size_t> usedVerts;
        for (size_t idx : newIdx)
        {
            usedVerts.insert(idx);
        }

        std::vector<int32_t> remap(this->vertexCount, -1);
        std::vector<Ogre::Vector3> nv, nn;
        std::vector<Ogre::Vector4> nt;
        std::vector<Ogre::Vector2> nu;
        size_t ni = 0;
        for (size_t i = 0; i < this->vertexCount; ++i)
        {
            if (usedVerts.count(i))
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

        // Auto-select the new hole rim edges so Ctrl+F fills the right hole.
        // A rim edge bordered exactly one deleted face AND still has both vertices
        // present in the surviving mesh (remap[v] != -1).
        this->selectedFaces.clear();
        this->selectedEdges.clear();
        for (auto& [edge, count] : edgeCount)
        {
            // count == 1 means this edge belonged to exactly one deleted face
            // (if count == 2, both adjacent faces were deleted — interior edge, not a rim)
            if (count == 1 && remap[edge.first] != -1 && remap[edge.second] != -1)
            {
                this->selectedEdges.insert(EdgeKey(static_cast<size_t>(remap[edge.first]), static_cast<size_t>(remap[edge.second])));
            }
        }

        this->buildVertexAdjacency();
        this->rebuildDynamicBuffers();
        this->scheduleOverlayUpdate();
        this->fireUndoEvent(oldData);
    }

    // =========================================================================
    //  Move
    // =========================================================================

    void MeshEditComponent::moveSelected(const Ogre::Vector3& localDelta)
    {
        std::unordered_set<size_t> selectedSet;
        for (size_t i : this->selectedVertices)
        {
            selectedSet.insert(i);
        }
        for (const EdgeKey& e : this->selectedEdges)
        {
            selectedSet.insert(e.a);
            selectedSet.insert(e.b);
        }
        for (size_t t : this->selectedFaces)
        {
            selectedSet.insert(this->indices[t * 3]);
            selectedSet.insert(this->indices[t * 3 + 1]);
            selectedSet.insert(this->indices[t * 3 + 2]);
        }
        if (selectedSet.empty())
        {
            return;
        }

        const std::unordered_set<size_t> toMove = expandToCoLocated(selectedSet, this->vertexGroup, this->positionGroups);

        for (size_t i : toMove)
        {
            this->vertices[i] += localDelta;
        }

        this->recalculateNormals_internal();
        this->recalculateTangents();
        this->uploadVertexData();
        this->scheduleOverlayUpdate();
    }

    // =========================================================================
    //  Extra operations
    // =========================================================================

    void MeshEditComponent::mergeByDistance_noUndo(Ogre::Real threshold)
    {
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

    void MeshEditComponent::mergeByDistance(Ogre::Real threshold)
    {
        std::vector<unsigned char> oldData = this->getMeshData();
        this->mergeByDistance_noUndo(threshold);
        this->fireUndoEvent(oldData);
    }

    // =========================================================================
    //  New: Subdivide All
    // =========================================================================

    void MeshEditComponent::subdivideAll(void)
    {
        std::vector<unsigned char> oldData = this->getMeshData();

        // Select every face, re-use subdivideSelectedFaces logic inline
        // (avoids modifying selectedFaces from outside the function contract)
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
        this->deselectAll();
        this->buildVertexAdjacency();
        this->rebuildDynamicBuffers();
        this->scheduleOverlayUpdate();

        this->fireUndoEvent(oldData);
    }

    // =========================================================================
    //  New: Mirror Mesh
    // =========================================================================

    void MeshEditComponent::mirrorMesh(const Ogre::String& axis)
    {
        std::vector<unsigned char> oldData = this->getMeshData();

        // Determine mirror normal and sign
        // axis string: "+X" / "-X" / "+Y" / "-Y" / "+Z" / "-Z"
        // The sign tells us which half to KEEP as source.
        // "+X" -> keep vertices with local X >= 0 as source, reflect across X=0 plane.
        int axisIdx = 0; // 0=X, 1=Y, 2=Z
        float keepSign = 1.0f;
        if (axis == "+X")
        {
            axisIdx = 0;
            keepSign = 1.0f;
        }
        else if (axis == "-X")
        {
            axisIdx = 0;
            keepSign = -1.0f;
        }
        else if (axis == "+Y")
        {
            axisIdx = 1;
            keepSign = 1.0f;
        }
        else if (axis == "-Y")
        {
            axisIdx = 1;
            keepSign = -1.0f;
        }
        else if (axis == "+Z")
        {
            axisIdx = 2;
            keepSign = 1.0f;
        }
        else if (axis == "-Z")
        {
            axisIdx = 2;
            keepSign = -1.0f;
        }

        // Append reflected copies of every vertex
        const size_t origVCount = this->vertexCount;
        this->vertices.reserve(origVCount * 2);
        this->normals.reserve(origVCount * 2);
        this->tangents.reserve(origVCount * 2);
        this->uvCoordinates.reserve(origVCount * 2);

        for (size_t i = 0; i < origVCount; ++i)
        {
            Ogre::Vector3 rv = this->vertices[i];
            Ogre::Vector3 rn = this->normals[i];
            Ogre::Vector4 rt = this->tangents[i];

            // Reflect position and normal across the chosen axis plane
            rv[axisIdx] = -rv[axisIdx];
            rn[axisIdx] = -rn[axisIdx];
            rt[axisIdx] = -rt[axisIdx];

            this->vertices.push_back(rv);
            this->normals.push_back(rn);
            this->tangents.push_back(rt);
            this->uvCoordinates.push_back(this->uvCoordinates[i]); // UVs mirrored in U for X-axis, keep for others
        }
        this->vertexCount = this->vertices.size();

        // Append reflected index triangles with reversed winding (to keep front-face consistent)
        const size_t origIdxCount = this->indexCount;
        this->indices.reserve(origIdxCount * 2);
        for (size_t i = 0; i < origIdxCount; i += 3)
        {
            // Reversed winding: i0, i2, i1 (mirrors the normal direction)
            this->indices.push_back(static_cast<Ogre::uint32>(this->indices[i] + origVCount));
            this->indices.push_back(static_cast<Ogre::uint32>(this->indices[i + 2] + origVCount));
            this->indices.push_back(static_cast<Ogre::uint32>(this->indices[i + 1] + origVCount));
        }
        this->indexCount = this->indices.size();

        // Weld the seam — vertices whose axis coordinate is ~0 are shared between halves
        this->mergeByDistance_noUndo(1e-4f);

        // mergeByDistance_noUndo already calls rebuildDynamicBuffers + scheduleOverlayUpdate
        this->fireUndoEvent(oldData);
    }

    // =========================================================================
    //  New: Apply Scale
    // =========================================================================

    void MeshEditComponent::applyScale(void)
    {
        Ogre::SceneNode* node = this->gameObjectPtr->getSceneNode();
        if (!node)
        {
            return;
        }

        const Ogre::Vector3 scale = node->getScale();

        // If scale is already identity (within floating-point tolerance), nothing to do
        if (scale.positionEquals(Ogre::Vector3::UNIT_SCALE, 1e-5f))
        {
            return;
        }

        std::vector<unsigned char> oldData = this->getMeshData();

        // Bake scale into vertex positions and normals
        for (auto& v : this->vertices)
        {
            v.x *= scale.x;
            v.y *= scale.y;
            v.z *= scale.z;
        }

        // Normals transform by the inverse-transpose of scale (= 1/scale for uniform, diagonal for non-uniform)
        const Ogre::Vector3 invScale(scale.x != 0.0f ? 1.0f / scale.x : 1.0f, scale.y != 0.0f ? 1.0f / scale.y : 1.0f, scale.z != 0.0f ? 1.0f / scale.z : 1.0f);

        for (auto& n : this->normals)
        {
            n.x *= invScale.x;
            n.y *= invScale.y;
            n.z *= invScale.z;
            n.normalise();
        }

        // Reset the scene node scale to identity
        NOWA::GraphicsModule::RenderCommand cmd = [node]()
        {
            node->setScale(Ogre::Vector3::UNIT_SCALE);
        };
        NOWA::GraphicsModule::getInstance()->enqueue(std::move(cmd), "MeshEditComponent::applyScale_resetNode");

        this->recalculateTangents();
        this->buildVertexAdjacency();
        this->rebuildDynamicBuffers();
        this->scheduleOverlayUpdate();

        this->fireUndoEvent(oldData);
    }

    // =========================================================================
    //  Extrude amount + Mirror axis accessors
    // =========================================================================

    void MeshEditComponent::setExtrudeAmount(Ogre::Real amount)
    {
        this->extrudeAmount->setValue(amount);
    }

    Ogre::Real MeshEditComponent::getExtrudeAmount(void) const
    {
        return this->extrudeAmount->getReal();
    }

    void MeshEditComponent::setMirrorAxis(const Ogre::String& axis)
    {
        this->mirrorAxis->setListSelectedValue(axis);
    }

    Ogre::String MeshEditComponent::getMirrorAxis(void) const
    {
        return this->mirrorAxis->getListSelectedValue();
    }

    void MeshEditComponent::addVertexAtPosition(const Ogre::Vector3& localPos)
    {
        std::vector<unsigned char> oldData = this->getMeshData();

        // Append the new vertex; normal points up by default (user can recalc later)
        this->vertices.push_back(localPos);
        this->normals.push_back(Ogre::Vector3::UNIT_Y);
        this->tangents.push_back(Ogre::Vector4(1.0f, 0.0f, 0.0f, 1.0f));
        this->uvCoordinates.push_back(Ogre::Vector2(0.5f, 0.5f));

        const size_t newIdx = this->vertexCount;
        ++this->vertexCount;

        // Select the new vertex exclusively so the user sees where it landed
        // and can immediately select more for buildFaceFromSelection()
        this->selectedVertices.clear();
        this->selectedVertices.insert(newIdx);

        // No new faces yet — topology rebuild needed only when buildFace is called.
        // We do a lightweight upload here (vertex count grew -> must rebuild buffers).
        this->buildVertexAdjacency();
        this->rebuildDynamicBuffers();
        this->scheduleOverlayUpdate();

        this->fireUndoEvent(oldData);

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MeshEditComponent] Added vertex #" + Ogre::StringConverter::toString(newIdx) + " at " + Ogre::StringConverter::toString(localPos));
    }

    // =============================================================================
    //  buildFaceFromSelection  —  REPLACE
    //  3 vertices   -> 1 triangle
    //  4 vertices   -> 2 triangles (shorter diagonal split)
    //  5+ vertices  -> fan triangulation from first vertex
    // =============================================================================
    bool MeshEditComponent::buildFaceFromSelection(void)
    {
        if (this->selectedVertices.size() < 3)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MeshEditComponent] buildFaceFromSelection: need at least 3 vertices, got " + Ogre::StringConverter::toString(static_cast<int>(this->selectedVertices.size())));
            return false;
        }

        std::vector<unsigned char> oldData = this->getMeshData();

        // Collect selected indices in a stable order
        std::vector<size_t> sel(this->selectedVertices.begin(), this->selectedVertices.end());

        // Determine winding: face normal should point roughly toward camera
        Ogre::Vector3 desiredNormal = Ogre::Vector3::UNIT_Y;
        {
            Ogre::Camera* cam = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
            if (cam)
            {
                Ogre::Vector3 localCen = Ogre::Vector3::ZERO;
                for (size_t i : sel)
                {
                    localCen += this->vertices[i];
                }
                localCen /= static_cast<float>(sel.size());

                const Ogre::Matrix4 worldMat = this->gameObjectPtr->getSceneNode()->_getFullTransform();
                const Ogre::Vector3 wCen = worldMat * localCen;

                Ogre::Matrix3 rot3;
                worldMat.extract3x3Matrix(rot3);
                desiredNormal = rot3.Inverse() * (cam->getDerivedPosition() - wCen);
                desiredNormal.normalise();
            }
        }

        // Add one triangle with correct winding toward camera
        auto addTriangle = [&](size_t a, size_t b, size_t c)
        {
            const Ogre::Vector3 e1 = this->vertices[b] - this->vertices[a];
            const Ogre::Vector3 e2 = this->vertices[c] - this->vertices[a];
            Ogre::Vector3 n = e1.crossProduct(e2);
            if (n.squaredLength() < 1e-10f)
            {
                return; // degenerate, skip
            }
            n.normalise();
            if (n.dotProduct(desiredNormal) < 0.0f)
            {
                std::swap(b, c); // flip winding
            }
            this->indices.push_back(static_cast<Ogre::uint32>(a));
            this->indices.push_back(static_cast<Ogre::uint32>(b));
            this->indices.push_back(static_cast<Ogre::uint32>(c));
            this->indexCount += 3;
        };

        if (sel.size() == 3)
        {
            addTriangle(sel[0], sel[1], sel[2]);
        }
        else if (sel.size() == 4)
        {
            // Split on the shorter diagonal for a more regular quad
            const float diagA = this->vertices[sel[0]].distance(this->vertices[sel[2]]);
            const float diagB = this->vertices[sel[1]].distance(this->vertices[sel[3]]);
            if (diagA <= diagB)
            {
                addTriangle(sel[0], sel[1], sel[2]);
                addTriangle(sel[0], sel[2], sel[3]);
            }
            else
            {
                addTriangle(sel[0], sel[1], sel[3]);
                addTriangle(sel[1], sel[2], sel[3]);
            }
        }
        else
        {
            // 5+ vertices: fan triangulation from sel[0]
            for (size_t i = 1; i + 1 < sel.size(); ++i)
            {
                addTriangle(sel[0], sel[i], sel[i + 1]);
            }
        }

        this->buildVertexAdjacency();
        this->rebuildDynamicBuffers();
        this->scheduleOverlayUpdate();
        this->fireUndoEvent(oldData);

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MeshEditComponent] Built face from " + Ogre::StringConverter::toString(static_cast<int>(sel.size())) + " vertices.");
        return true;
    }

    void MeshEditComponent::setOriginAlignment(const Ogre::String& preset)
    {
        this->originAlignment->setListSelectedValue(preset);
    }

    Ogre::String MeshEditComponent::getOriginAlignment(void) const
    {
        return this->originAlignment->getListSelectedValue();
    }

    void MeshEditComponent::setFlipDirection(const Ogre::String& axis)
    {
        this->flipDirection->setListSelectedValue(axis);
    }

    Ogre::String MeshEditComponent::getFlipDirection(void) const
    {
        return this->flipDirection->getListSelectedValue();
    }

    void MeshEditComponent::flipNormals(void)
    {
        std::vector<unsigned char> oldData = this->getMeshData();

        for (auto& n : this->normals)
        {
            n = -n;
        }
        for (size_t i = 0; i < this->indexCount; i += 3)
        {
            std::swap(this->indices[i + 1], this->indices[i + 2]);
        }

        // rebuildDynamicBuffers is enqueueAndWait — safe to read vertices inside
        this->rebuildDynamicBuffers();

        this->fireUndoEvent(oldData);
    }

    void MeshEditComponent::recalculateNormals(void)
    {
        std::vector<unsigned char> oldData = this->getMeshData();

        this->recalculateNormals_internal();

        // rebuildDynamicBuffers calls recalculateNormals_internal again internally
        // (no-op double-compute since result is deterministic from geometry).
        this->rebuildDynamicBuffers();

        this->fireUndoEvent(oldData);
    }

    void MeshEditComponent::subdivideSelectedFaces(void)
    {
        if (this->selectedFaces.empty())
        {
            return;
        }

        std::vector<unsigned char> oldData = this->getMeshData();

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
        this->fireUndoEvent(oldData);
    }

    void MeshEditComponent::extrudeSelectedFaces(Ogre::Real amount)
    {
        if (this->selectedFaces.empty())
        {
            return;
        }

        std::vector<unsigned char> oldData = this->getMeshData();

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
        this->fireUndoEvent(oldData);
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

        // ── Build co-location groups so sculpting never leaves gaps ──────────────
        this->buildPositionGroups();
    }

    void MeshEditComponent::buildPositionGroups(void)
    {
        this->positionGroups.clear();
        this->vertexGroup.assign(this->vertexCount, -1);

        constexpr float kEps2 = 1e-8f; // same epsilon used elsewhere (0.01 mm^2)

        for (size_t i = 0; i < this->vertexCount; ++i)
        {
            if (this->vertexGroup[i] >= 0)
            {
                continue; // already assigned
            }

            // Start a new group with vertex i
            const int g = static_cast<int>(this->positionGroups.size());
            this->positionGroups.push_back({i});
            this->vertexGroup[i] = g;

            // Find all later vertices at the same position
            const Ogre::Vector3& pi = this->vertices[i];
            for (size_t j = i + 1; j < this->vertexCount; ++j)
            {
                if (this->vertexGroup[j] >= 0)
                {
                    continue;
                }
                if (this->vertices[j].squaredDistance(pi) <= kEps2)
                {
                    this->positionGroups[g].push_back(j);
                    this->vertexGroup[j] = g;
                }
            }
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
        if (!this->overlayObject || !this->gameObjectPtr)
        {
            return;
        }

        const EditMode mode = this->getEditMode();

        // ── Object mode: just clear the ManualObject ─────────────────────────────
        if (mode == EditMode::OBJECT)
        {
            NOWA::GraphicsModule::RenderCommand cmd = [this]()
            {
                if (this->overlayObject)
                {
                    this->overlayObject->clear();
                }
            };
            NOWA::GraphicsModule::getInstance()->enqueue(std::move(cmd), "MeshEditComponent::overlay_clear");
            return;
        }

        // ── Get camera pos on main thread (1-frame stale is fine for 4mm push) ───
        Ogre::Vector3 camPos = Ogre::Vector3::ZERO;
        {
            Ogre::Camera* cam = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
            if (cam)
            {
                camPos = cam->getDerivedPosition();
            }
        }
        const float pushDist = 0.004f;

        auto push = [&](const Ogre::Vector3& wp) -> Ogre::Vector3
        {
            const Ogre::Vector3 dir = camPos - wp;
            const float len = dir.length();
            if (len < 1e-6f)
            {
                return wp;
            }
            return wp + dir * (pushDist / len);
        };

        // ── World transform — safe to read on main thread ─────────────────────────
        const Ogre::Matrix4 worldMat = this->gameObjectPtr->getSceneNode()->_getFullTransform();
        auto lw = [&](const Ogre::Vector3& v) -> Ogre::Vector3
        {
            return worldMat * v;
        };

        const float s = this->vertexMarkerSize->getReal();
        const bool doWire = this->showWireframe->getBool() || (mode == EditMode::EDGE);

        // We pass three separate sections to the render command.
        // Section 0 — wireframe/edge lines
        // Section 1 — vertex crosses
        // Section 2 — face centre squares
        // Each section is a flat LINE_LIST: pairs of (pos,color) sequential verts.

        std::vector<OverlayVertex> wireVerts;
        std::vector<OverlayVertex> vertVerts;
        std::vector<OverlayVertex> faceVerts;

        // ── Section 0: Wireframe / edge lines ─────────────────────────────────────
        if (doWire)
        {
            const Ogre::ColourValue cWire(0.35f, 0.35f, 0.35f, 1.0f);
            const Ogre::ColourValue cSel(1.0f, 0.75f, 0.0f, 1.0f);

            std::set<std::pair<size_t, size_t>> drawn;
            for (size_t i = 0; i < this->indexCount; i += 3)
            {
                for (int e = 0; e < 3; ++e)
                {
                    const size_t a = this->indices[i + e];
                    const size_t b = this->indices[i + (e + 1) % 3];
                    if (!drawn.insert({std::min(a, b), std::max(a, b)}).second)
                    {
                        continue;
                    }

                    const bool sel = (mode == EditMode::EDGE) && this->selectedEdges.count(EdgeKey(a, b));
                    const Ogre::ColourValue& c = sel ? cSel : cWire;

                    wireVerts.push_back({push(lw(this->vertices[a])), c});
                    wireVerts.push_back({push(lw(this->vertices[b])), c});
                }
            }
        }

        // ── Section 1: Vertex crosses ─────────────────────────────────────────────
        if (mode == EditMode::VERTEX)
        {
            const Ogre::ColourValue cV(1.0f, 1.0f, 1.0f, 1.0f);
            const Ogre::ColourValue cVS(1.0f, 0.75f, 0.0f, 1.0f);
            constexpr float kEps2 = 1e-8f;

            for (size_t i = 0; i < this->vertexCount; ++i)
            {
                bool sel = (this->selectedVertices.count(i) > 0);
                if (!sel)
                {
                    const Ogre::Vector3& myPos = this->vertices[i];
                    for (size_t si : this->selectedVertices)
                    {
                        if (this->vertices[si].squaredDistance(myPos) <= kEps2)
                        {
                            sel = true;
                            break;
                        }
                    }
                }
                const Ogre::ColourValue& c = sel ? cVS : cV;
                const Ogre::Vector3 wp = lw(this->vertices[i]);

                // Six arm endpoints of the cross (+X/-X, +Y/-Y, +Z/-Z)
                vertVerts.push_back({push(wp + Ogre::Vector3(-s, 0, 0)), c});
                vertVerts.push_back({push(wp + Ogre::Vector3(s, 0, 0)), c});

                vertVerts.push_back({push(wp + Ogre::Vector3(0, -s, 0)), c});
                vertVerts.push_back({push(wp + Ogre::Vector3(0, s, 0)), c});

                vertVerts.push_back({push(wp + Ogre::Vector3(0, 0, -s)), c});
                vertVerts.push_back({push(wp + Ogre::Vector3(0, 0, s)), c});
            }
        }

        // ── Section 2: Face centre squares ────────────────────────────────────────
        if (mode == EditMode::FACE)
        {
            const float fs = s * 1.5f;
            const Ogre::ColourValue cF(0.5f, 0.5f, 1.0f, 1.0f);
            const Ogre::ColourValue cFS(1.0f, 0.5f, 0.0f, 1.0f);

            Ogre::Matrix3 worldRot3;
            worldMat.extract3x3Matrix(worldRot3);

            const size_t triCount = this->indexCount / 3;
            for (size_t t = 0; t < triCount; ++t)
            {
                const Ogre::Vector3& lv0 = this->vertices[this->indices[t * 3]];
                const Ogre::Vector3& lv1 = this->vertices[this->indices[t * 3 + 1]];
                const Ogre::Vector3& lv2 = this->vertices[this->indices[t * 3 + 2]];

                const Ogre::Vector3 localNorm = (lv1 - lv0).crossProduct(lv2 - lv0).normalisedCopy();
                const Ogre::Vector3 wNorm = (worldRot3 * localNorm).normalisedCopy();

                const Ogre::Vector3 ref = (std::abs(wNorm.dotProduct(Ogre::Vector3::UNIT_Y)) < 0.99f) ? Ogre::Vector3::UNIT_Y : Ogre::Vector3::UNIT_X;
                const Ogre::Vector3 wTan = wNorm.crossProduct(ref).normalisedCopy();
                const Ogre::Vector3 wBit = wNorm.crossProduct(wTan).normalisedCopy();

                const Ogre::Vector3 wCen = lw((lv0 + lv1 + lv2) / 3.0f);
                const bool sel = (this->selectedFaces.count(t) > 0);
                const Ogre::ColourValue& c = sel ? cFS : cF;

                const Ogre::Vector3 c00 = push(wCen + (-wTan - wBit) * fs);
                const Ogre::Vector3 c10 = push(wCen + (wTan - wBit) * fs);
                const Ogre::Vector3 c11 = push(wCen + (wTan + wBit) * fs);
                const Ogre::Vector3 c01 = push(wCen + (-wTan + wBit) * fs);

                // Four edges of the square (8 vertices = 4 line segments)
                faceVerts.push_back({c00, c});
                faceVerts.push_back({c10, c});
                faceVerts.push_back({c10, c});
                faceVerts.push_back({c11, c});
                faceVerts.push_back({c11, c});
                faceVerts.push_back({c01, c});
                faceVerts.push_back({c01, c});
                faceVerts.push_back({c00, c});
            }
        }

        // ── Section 3: Origin + direction arrow ──────────────────────────────────────
        // Arrow length = 30% of the largest AABB extent, min 0.15 to stay visible.
        // Shaft:      local origin -> +Z (forward), colour cyan
        // Up stub:    local origin -> +Y,            colour yellow  (short, 40% of shaft)
        // Arrowhead:  four lines at the tip of the shaft, fanned back ~25°
        std::vector<OverlayVertex> originVerts;

        if (this->showOrigin->getBool() && this->vertexCount > 0)
        {
            // Compute AABB extent for adaptive arrow length
            Ogre::Vector3 minV(std::numeric_limits<float>::max());
            Ogre::Vector3 maxV(-std::numeric_limits<float>::max());
            for (const auto& v : this->vertices)
            {
                minV.makeFloor(v);
                maxV.makeCeil(v);
            }
            const Ogre::Vector3 ext = maxV - minV;
            const float aLen = std::max(0.15f, std::max({ext.x, ext.y, ext.z}) * 0.3f);
            const float aHead = aLen * 0.25f; // arrowhead length
            const float uLen = aLen * 0.4f;   // up stub length

            // Local axis vectors transformed to world space
            Ogre::Matrix3 worldRot3;
            worldMat.extract3x3Matrix(worldRot3);

            const Ogre::Vector3 wOrigin = lw(Ogre::Vector3::ZERO); // local (0,0,0) in world
            const Ogre::Vector3 wForward = (worldRot3 * Ogre::Vector3::UNIT_Z).normalisedCopy();
            const Ogre::Vector3 wUp = (worldRot3 * Ogre::Vector3::UNIT_Y).normalisedCopy();
            const Ogre::Vector3 wRight = (worldRot3 * Ogre::Vector3::UNIT_X).normalisedCopy();

            const Ogre::Vector3 wTip = wOrigin + wForward * aLen;

            // Reference vector for arrowhead fan (perpendicular to forward)
            // Use world up unless forward is nearly parallel to it
            const Ogre::Vector3 ref1 = (std::abs(wForward.dotProduct(wUp)) < 0.95f) ? wUp : wRight;
            const Ogre::Vector3 ref2 = wForward.crossProduct(ref1).normalisedCopy();
            const Ogre::Vector3 ref3 = ref1.normalisedCopy();

            const Ogre::ColourValue cCyan(0.0f, 0.9f, 1.0f, 1.0f);   // shaft / arrowhead
            const Ogre::ColourValue cYellow(1.0f, 0.9f, 0.0f, 1.0f); // up stub
            const Ogre::ColourValue cRed(1.0f, 0.2f, 0.2f, 1.0f);    // origin dot

            // Small diamond marker at the origin (two crossed diagonals)
            const float ds = aLen * 0.06f;
            originVerts.push_back({push(wOrigin - wRight * ds), cRed});
            originVerts.push_back({push(wOrigin + wRight * ds), cRed});
            originVerts.push_back({push(wOrigin - wUp * ds), cRed});
            originVerts.push_back({push(wOrigin + wUp * ds), cRed});
            originVerts.push_back({push(wOrigin - wForward * ds), cRed});
            originVerts.push_back({push(wOrigin + wForward * ds), cRed});

            // Main shaft: origin -> tip
            originVerts.push_back({push(wOrigin), cCyan});
            originVerts.push_back({push(wTip), cCyan});

            // Arrowhead: four lines from tip back along ref1/ref2 diagonals
            originVerts.push_back({push(wTip), cCyan});
            originVerts.push_back({push(wTip - wForward * aHead + ref2 * aHead * 0.5f), cCyan});
            originVerts.push_back({push(wTip), cCyan});
            originVerts.push_back({push(wTip - wForward * aHead - ref2 * aHead * 0.5f), cCyan});
            originVerts.push_back({push(wTip), cCyan});
            originVerts.push_back({push(wTip - wForward * aHead + ref3 * aHead * 0.5f), cCyan});
            originVerts.push_back({push(wTip), cCyan});
            originVerts.push_back({push(wTip - wForward * aHead - ref3 * aHead * 0.5f), cCyan});

            // Short up stub: origin -> +Y  (shows orientation / up direction)
            originVerts.push_back({push(wOrigin), cYellow});
            originVerts.push_back({push(wOrigin + wUp * uLen), cYellow});
        }

        // ── Enqueue render command — captures geometry by MOVE, never reads 'this->vertices' ──
        NOWA::GraphicsModule::RenderCommand cmd = [this, wireV = std::move(wireVerts), vertV = std::move(vertVerts), faceV = std::move(faceVerts),
                                                      origV = std::move(originVerts)]()
        {
            if (!this->overlayObject)
            {
                return;
            }
            this->overlayObject->clear();

            auto buildSection = [this](const std::vector<OverlayVertex>& verts)
            {
                if (verts.empty())
                {
                    return;
                }
                this->overlayObject->begin("WhiteNoLightingBackground", Ogre::OT_LINE_LIST);
                uint32_t idx = 0;
                for (const OverlayVertex& v : verts)
                {
                    this->overlayObject->position(v.pos);
                    this->overlayObject->colour(v.color);
                    this->overlayObject->index(idx++);
                }
                this->overlayObject->end();
            };

            buildSection(wireV);
            buildSection(vertV);
            buildSection(faceV);
            buildSection(origV);
        };

        NOWA::GraphicsModule::getInstance()->enqueue(std::move(cmd), "MeshEditComponent::overlayRebuild");
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

        Ogre::Window* win = Core::getSingletonPtr()->getOgreRenderWindow();
        const float W = static_cast<float>(win->getWidth());
        const float H = static_cast<float>(win->getHeight());

        // Mouse in pixel space
        const float mx = sx * W;
        const float my = sy * H;

        const Ogre::Matrix4 vp = cam->getProjectionMatrix() * cam->getViewMatrix();
        const Ogre::Matrix4 world = this->gameObjectPtr->getSceneNode()->_getFullTransform();

        // ── Project all vertices to pixel space ──────────────────────────────────
        struct ScreenVert
        {
            float x, y;
            bool valid;
        };
        std::vector<ScreenVert> sp(this->vertexCount);

        for (size_t i = 0; i < this->vertexCount; ++i)
        {
            Ogre::Vector4 cp = vp * (world * Ogre::Vector4(this->vertices[i], 1.0f));
            if (cp.w <= 1e-4f)
            {
                sp[i] = {0.f, 0.f, false};
                continue;
            }
            // NDC -> [0,1] -> pixels
            const float ndcX = cp.x / cp.w;
            const float ndcY = cp.y / cp.w;
            sp[i] = {((ndcX + 1.0f) * 0.5f) * W, (1.0f - (ndcY + 1.0f) * 0.5f) * H, true};
        }

        // ── Point-to-segment distance in pixel space (aspect-ratio correct) ──────
        //    If only one endpoint is valid (partially clipped), we treat the valid
        //    endpoint as a degenerate zero-length segment — i.e. we return the
        //    distance from the cursor to that single visible point.
        auto ptSeg = [](float px, float py, float ax, float ay, bool aOk, float bx, float by, bool bOk) -> float
        {
            if (!aOk && !bOk)
            {
                return std::numeric_limits<float>::max();
            }

            if (!aOk) // only B visible — distance to B
            {
                return std::hypot(px - bx, py - by);
            }
            if (!bOk) // only A visible — distance to A
            {
                return std::hypot(px - ax, py - ay);
            }

            // Both valid — standard point-to-segment
            const float dx = bx - ax, dy = by - ay;
            const float len2 = dx * dx + dy * dy;
            if (len2 < 1.0f) // edge shorter than 1 px — treat as point
            {
                return std::hypot(px - ax, py - ay);
            }
            const float t = std::max(0.0f, std::min(1.0f, ((px - ax) * dx + (py - ay) * dy) / len2));
            return std::hypot(px - ax - t * dx, py - ay - t * dy);
        };

        // ── Picking threshold: 8 pixels (scale-independent, works on any DPI) ────
        // Picking threshold — 12 px works well at typical editing zoom levels.
        // Boundary edges (belong to only one triangle) get a 4 px bonus since
        // they are visually prominent and users click them often.
        float best = 12.0f;
        size_t ba = SIZE_MAX, bb = SIZE_MAX;

        // Count how many triangles each canonical edge belongs to (for boundary bonus)
        std::map<std::pair<size_t, size_t>, int> edgeTriCount;
        for (size_t i = 0; i < this->indexCount; i += 3)
        {
            for (int e = 0; e < 3; ++e)
            {
                size_t a = this->indices[i + e];
                size_t b = this->indices[i + (e + 1) % 3];
                edgeTriCount[{std::min(a, b), std::max(a, b)}]++;
            }
        }

        std::set<std::pair<size_t, size_t>> checked;
        for (size_t i = 0; i < this->indexCount; i += 3)
        {
            for (int e = 0; e < 3; ++e)
            {
                const size_t a = this->indices[i + e];
                const size_t b = this->indices[i + (e + 1) % 3];

                if (!checked.insert({std::min(a, b), std::max(a, b)}).second)
                {
                    continue;
                }
                if (!sp[a].valid && !sp[b].valid)
                {
                    continue;
                }

                const float d = ptSeg(mx, my, sp[a].x, sp[a].y, sp[a].valid, sp[b].x, sp[b].y, sp[b].valid);

                // Boundary edges get extra tolerance
                const bool isBoundary = edgeTriCount[{std::min(a, b), std::max(a, b)}] == 1;
                const float effectiveThreshold = best + (isBoundary ? 4.0f : 0.0f);

                if (d < effectiveThreshold)
                {
                    best = d; // update best so we still take the closest one
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
        // Full mesh snapshot for undo (taken BEFORE any movement)
        this->grabPreEditData = this->getMeshData();
        this->grabSavedPositions = this->vertices;
        this->grabAccumDelta = Ogre::Vector3::ZERO;
        this->grabAxis = GrabAxis::FREE;
        this->isGrabbing = true;
        this->isScaling = false;
        this->grabScaleMouseAccum = 0;
        this->activeTool = ActiveTool::GRAB;

        // Pre-compute centroid of the selection for scale pivot
        this->grabCentroid = Ogre::Vector3::ZERO;
        std::unordered_set<size_t> sel;
        for (size_t i : this->selectedVertices)
        {
            sel.insert(i);
        }
        for (const EdgeKey& e : this->selectedEdges)
        {
            sel.insert(e.a);
            sel.insert(e.b);
        }
        for (size_t t : this->selectedFaces)
        {
            sel.insert(this->indices[t * 3]);
            sel.insert(this->indices[t * 3 + 1]);
            sel.insert(this->indices[t * 3 + 2]);
        }
        if (!sel.empty())
        {
            for (size_t i : sel)
            {
                this->grabCentroid += this->vertices[i];
            }
            this->grabCentroid /= static_cast<float>(sel.size());
        }

        // this->updateStatusOverlay();
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MeshEditComponent] Grab started. G+X/Y/Z=axis, G+S=scale, Enter/LMB=confirm, Esc/RMB=cancel.");
    }

    void MeshEditComponent::confirmGrab(void)
    {
        this->isGrabbing = false;
        this->isScaling = false;
        this->activeTool = ActiveTool::SELECT;

        this->recalculateNormals_internal();
        this->recalculateTangents();

        // rebuildDynamicBuffers is enqueueAndWait — reads this->vertices safely
        // (main thread is blocked while the render command runs).
        // It also updates the AABB so culling is correct after large deformations.
        this->rebuildDynamicBuffers();

        this->fireUndoEvent(this->grabPreEditData);
        this->grabPreEditData.clear();
        this->grabSavedPositions.clear();
        // this->updateStatusOverlay();
    }

    void MeshEditComponent::cancelGrab(void)
    {
        if (!this->grabSavedPositions.empty())
        {
            this->vertices = this->grabSavedPositions;
        }

        this->grabSavedPositions.clear();
        this->grabPreEditData.clear();
        this->isGrabbing = false;
        this->isScaling = false;
        this->activeTool = ActiveTool::SELECT;

        this->recalculateNormals_internal();
        this->recalculateTangents();

        // Full rebuild restores the AABB to the original shape as well
        this->rebuildDynamicBuffers();

        this->scheduleOverlayUpdate();
        // this->updateStatusOverlay();
    }

    void MeshEditComponent::applyGrabMouseDelta(int dx, int dy)
    {
        if (!this->isGrabbing)
        {
            return;
        }

        if (this->isScaling)
        {
            this->applyGrabScale(dx);
            return;
        }

        Ogre::Camera* cam = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
        if (!cam)
        {
            return;
        }

        const float sen = 0.005f;
        Ogre::Vector3 wDelta = cam->getRight() * (dx * sen) + cam->getUp() * (-dy * sen);

        switch (this->grabAxis)
        {
        case GrabAxis::X:
            wDelta = Ogre::Vector3::UNIT_X * wDelta.dotProduct(Ogre::Vector3::UNIT_X);
            break;
        case GrabAxis::Y:
            wDelta = Ogre::Vector3::UNIT_Y * wDelta.dotProduct(Ogre::Vector3::UNIT_Y);
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

        // ── Collect directly-selected vertices ───────────────────────────────────
        std::unordered_set<size_t> selectedSet;
        for (size_t i : this->selectedVertices)
        {
            selectedSet.insert(i);
        }
        for (const EdgeKey& e : this->selectedEdges)
        {
            selectedSet.insert(e.a);
            selectedSet.insert(e.b);
        }
        for (size_t t : this->selectedFaces)
        {
            selectedSet.insert(this->indices[t * 3]);
            selectedSet.insert(this->indices[t * 3 + 1]);
            selectedSet.insert(this->indices[t * 3 + 2]);
        }

        // ── Expand to co-located group members — prevents grab gaps ──────────────
        const std::unordered_set<size_t> toMove = expandToCoLocated(selectedSet, this->vertexGroup, this->positionGroups);

        for (size_t i : toMove)
        {
            this->vertices[i] = this->grabSavedPositions[i] + this->grabAccumDelta;
        }

        // ── Proportional editing ──────────────────────────────────────────────────
        if (this->isProportionalEditing)
        {
            const float propRadius = this->proportionalRadius->getReal();
            const float falloffExp = this->brushFalloff->getReal();
            for (size_t i = 0; i < this->vertexCount; ++i)
            {
                if (toMove.count(i))
                {
                    continue;
                }
                float minDist = std::numeric_limits<float>::max();
                for (size_t s : toMove)
                {
                    float d = this->grabSavedPositions[i].distance(this->grabSavedPositions[s]);
                    if (d < minDist)
                    {
                        minDist = d;
                    }
                }
                if (minDist >= propRadius)
                {
                    continue;
                }
                const float nd = minDist / propRadius;
                const float falloff = std::pow(1.0f - nd, falloffExp);
                this->vertices[i] = this->grabSavedPositions[i] + this->grabAccumDelta * falloff;
            }
        }

        this->recalculateNormals_internal();
        this->recalculateTangents();
        this->uploadVertexData();
        this->scheduleOverlayUpdate();
        // this->updateStatusOverlay();
    }

    void MeshEditComponent::applyGrabScale(int dx)
    {
        const float sen = 0.005f;
        this->grabScaleMouseAccum += dx;
        const float scaleFactor = std::max(0.01f, 1.0f + this->grabScaleMouseAccum * sen);

        // Collect + expand selection
        std::unordered_set<size_t> selectedSet;
        for (size_t i : this->selectedVertices)
        {
            selectedSet.insert(i);
        }
        for (const EdgeKey& e : this->selectedEdges)
        {
            selectedSet.insert(e.a);
            selectedSet.insert(e.b);
        }
        for (size_t t : this->selectedFaces)
        {
            selectedSet.insert(this->indices[t * 3]);
            selectedSet.insert(this->indices[t * 3 + 1]);
            selectedSet.insert(this->indices[t * 3 + 2]);
        }

        const std::unordered_set<size_t> toScale = expandToCoLocated(selectedSet, this->vertexGroup, this->positionGroups);

        for (size_t i : toScale)
        {
            this->vertices[i] = this->grabCentroid + (this->grabSavedPositions[i] - this->grabCentroid) * scaleFactor;
        }

        this->recalculateNormals_internal();
        this->recalculateTangents();
        this->uploadVertexData();
        this->scheduleOverlayUpdate();
        // this->updateStatusOverlay();
    }

    void MeshEditComponent::mergeSelected(void)
    {
        std::unordered_set<size_t> toMerge;
        switch (this->getEditMode())
        {
        case EditMode::VERTEX:
            for (size_t i : this->selectedVertices)
            {
                toMerge.insert(i);
            }
            break;
        case EditMode::EDGE:
            for (const EdgeKey& e : this->selectedEdges)
            {
                toMerge.insert(e.a);
                toMerge.insert(e.b);
            }
            break;
        case EditMode::FACE:
            for (size_t t : this->selectedFaces)
            {
                toMerge.insert(this->indices[t * 3]);
                toMerge.insert(this->indices[t * 3 + 1]);
                toMerge.insert(this->indices[t * 3 + 2]);
            }
            break;
        default:
            return;
        }
        if (toMerge.size() < 2)
        {
            return;
        }

        std::vector<unsigned char> oldData = this->getMeshData();

        // Compute centroid
        Ogre::Vector3 centroid = Ogre::Vector3::ZERO;
        for (size_t i : toMerge)
        {
            centroid += this->vertices[i];
        }
        centroid /= static_cast<float>(toMerge.size());

        // Move all selected to centroid
        for (size_t i : toMerge)
        {
            this->vertices[i] = centroid;
        }

        // Weld the coincident vertices
        this->mergeByDistance_noUndo(1e-5f); // already calls rebuildDynamicBuffers + scheduleOverlayUpdate

        this->fireUndoEvent(oldData);
    }

    void MeshEditComponent::dissolveSelected(void)
    {
        switch (this->getEditMode())
        {
        case EditMode::VERTEX:
            this->dissolveSelectedVertices();
            break;
        case EditMode::EDGE:
            this->dissolveSelectedEdges();
            break;
        default:
            break;
        }
    }

    void MeshEditComponent::dissolveSelectedVertices(void)
    {
        if (this->selectedVertices.empty())
        {
            return;
        }

        std::vector<unsigned char> oldData = this->getMeshData();

        // Process each selected vertex independently (simple, single-vertex at a time)
        std::vector<Ogre::uint32> masterIdx(this->indices.begin(), this->indices.end());

        for (size_t sv : this->selectedVertices)
        {
            // Find all triangles in masterIdx that contain sv
            std::vector<size_t> starTris;
            const size_t iCount = masterIdx.size();
            for (size_t i = 0; i < iCount; i += 3)
            {
                if (masterIdx[i] == sv || masterIdx[i + 1] == sv || masterIdx[i + 2] == sv)
                {
                    starTris.push_back(i / 3);
                }
            }
            if (starTris.empty())
            {
                continue;
            }

            // Count edge occurrences within the star
            std::map<std::pair<size_t, size_t>, int> edgeCnt;
            for (size_t t : starTris)
            {
                size_t a = masterIdx[t * 3], b = masterIdx[t * 3 + 1], c = masterIdx[t * 3 + 2];
                auto add = [&](size_t x, size_t y)
                {
                    edgeCnt[{std::min(x, y), std::max(x, y)}]++;
                };
                add(a, b);
                add(b, c);
                add(c, a);
            }

            // Boundary edges: shared by exactly one star triangle AND do NOT contain sv
            std::vector<std::pair<size_t, size_t>> boundaryEdges;
            for (auto& [e, cnt] : edgeCnt)
            {
                if (cnt == 1 && e.first != sv && e.second != sv)
                {
                    boundaryEdges.push_back(e);
                }
            }
            if (boundaryEdges.size() < 2)
            {
                continue;
            }

            // Walk boundary edges into an ordered loop
            std::map<size_t, std::vector<size_t>> adj;
            for (auto& [a, b] : boundaryEdges)
            {
                adj[a].push_back(b);
                adj[b].push_back(a);
            }

            std::vector<size_t> loop;
            {
                size_t start = boundaryEdges[0].first;
                loop.push_back(start);
                size_t prev = SIZE_MAX, cur = start;
                for (size_t step = 0; step < boundaryEdges.size(); ++step)
                {
                    size_t next = SIZE_MAX;
                    for (size_t n : adj[cur])
                    {
                        if (n != prev)
                        {
                            next = n;
                            break;
                        }
                    }
                    if (next == SIZE_MAX || next == start)
                    {
                        break;
                    }
                    loop.push_back(next);
                    prev = cur;
                    cur = next;
                }
            }
            if (loop.size() < 3)
            {
                continue;
            }

            // Remove star triangles from masterIdx
            std::set<size_t> toRemove(starTris.begin(), starTris.end());
            std::vector<Ogre::uint32> newIdx;
            newIdx.reserve(masterIdx.size());
            for (size_t i = 0; i < masterIdx.size(); i += 3)
            {
                if (!toRemove.count(i / 3))
                {
                    newIdx.push_back(masterIdx[i]);
                    newIdx.push_back(masterIdx[i + 1]);
                    newIdx.push_back(masterIdx[i + 2]);
                }
            }

            // Fan-triangulate boundary loop
            for (size_t i = 1; i + 1 < loop.size(); ++i)
            {
                newIdx.push_back(static_cast<Ogre::uint32>(loop[0]));
                newIdx.push_back(static_cast<Ogre::uint32>(loop[i]));
                newIdx.push_back(static_cast<Ogre::uint32>(loop[i + 1]));
            }

            masterIdx = std::move(newIdx);
        }

        this->indices = std::move(masterIdx);
        this->indexCount = this->indices.size();
        this->selectedVertices.clear();
        this->buildVertexAdjacency();
        this->rebuildDynamicBuffers();
        this->scheduleOverlayUpdate();
        this->fireUndoEvent(oldData);
    }

    void MeshEditComponent::dissolveSelectedEdges(void)
    {
        if (this->selectedEdges.empty())
        {
            return;
        }

        std::vector<unsigned char> oldData = this->getMeshData();

        std::set<size_t> triToRemove;
        std::vector<std::array<size_t, 3>> trisToAdd;
        std::set<size_t> processedTris;

        for (const EdgeKey& ek : this->selectedEdges)
        {
            const size_t ea = ek.a, eb = ek.b;

            // Find up to two triangles sharing this edge
            std::vector<size_t> shared;
            for (size_t i = 0; i < this->indexCount; i += 3)
            {
                bool hasA = (this->indices[i] == ea || this->indices[i + 1] == ea || this->indices[i + 2] == ea);
                bool hasB = (this->indices[i] == eb || this->indices[i + 1] == eb || this->indices[i + 2] == eb);
                if (hasA && hasB)
                {
                    shared.push_back(i / 3);
                }
            }
            if (shared.size() != 2)
            {
                continue; // skip boundary or degenerate edges
            }

            size_t t1 = shared[0], t2 = shared[1];
            if (processedTris.count(t1) || processedTris.count(t2))
            {
                continue;
            }
            processedTris.insert(t1);
            processedTris.insert(t2);

            auto getOther = [&](size_t t) -> size_t
            {
                for (int k = 0; k < 3; ++k)
                {
                    size_t v = this->indices[t * 3 + k];
                    if (v != ea && v != eb)
                    {
                        return v;
                    }
                }
                return SIZE_MAX;
            };

            size_t c1 = getOther(t1);
            size_t c2 = getOther(t2);
            if (c1 == SIZE_MAX || c2 == SIZE_MAX)
            {
                continue;
            }

            triToRemove.insert(t1);
            triToRemove.insert(t2);

            // Quad (ea, c1, eb, c2) with the original edge removed -> two new tris
            trisToAdd.push_back({ea, c1, c2});
            trisToAdd.push_back({c1, eb, c2});
        }

        std::vector<Ogre::uint32> newIdx;
        newIdx.reserve(this->indexCount);
        for (size_t i = 0; i < this->indexCount; i += 3)
        {
            if (!triToRemove.count(i / 3))
            {
                newIdx.push_back(this->indices[i]);
                newIdx.push_back(this->indices[i + 1]);
                newIdx.push_back(this->indices[i + 2]);
            }
        }
        for (auto& tri : trisToAdd)
        {
            newIdx.push_back(static_cast<Ogre::uint32>(tri[0]));
            newIdx.push_back(static_cast<Ogre::uint32>(tri[1]));
            newIdx.push_back(static_cast<Ogre::uint32>(tri[2]));
        }

        this->indices = std::move(newIdx);
        this->indexCount = this->indices.size();
        this->selectedEdges.clear();
        this->buildVertexAdjacency();
        this->rebuildDynamicBuffers();
        this->scheduleOverlayUpdate();
        this->fireUndoEvent(oldData);
    }

    bool MeshEditComponent::findBorderLoop(std::vector<size_t>& outLoop)
    {
        // Build edge set from selected edges (edge mode) or all open border edges
        std::set<std::pair<size_t, size_t>> edgeSet;

        if (this->getEditMode() == EditMode::EDGE && !this->selectedEdges.empty())
        {
            for (const EdgeKey& e : this->selectedEdges)
            {
                edgeSet.insert({std::min(e.a, e.b), std::max(e.a, e.b)});
            }
        }
        else
        {
            // Auto-detect open border edges (appear in exactly one triangle)
            std::map<std::pair<size_t, size_t>, int> edgeCnt;
            for (size_t i = 0; i < this->indexCount; i += 3)
            {
                auto add = [&](size_t a, size_t b)
                {
                    edgeCnt[{std::min(a, b), std::max(a, b)}]++;
                };
                add(this->indices[i], this->indices[i + 1]);
                add(this->indices[i + 1], this->indices[i + 2]);
                add(this->indices[i + 2], this->indices[i]);
            }
            for (auto& [e, cnt] : edgeCnt)
            {
                if (cnt == 1)
                {
                    edgeSet.insert(e);
                }
            }
        }
        if (edgeSet.empty())
        {
            return false;
        }

        // Build vertex adjacency for the edge set
        std::map<size_t, std::vector<size_t>> adj;
        for (auto& [a, b] : edgeSet)
        {
            adj[a].push_back(b);
            adj[b].push_back(a);
        }

        // Walk a single loop (may not cover all edges if there are multiple loops — takes the first)
        outLoop.clear();
        size_t start = edgeSet.begin()->first;
        outLoop.push_back(start);
        size_t prev = SIZE_MAX, cur = start;
        for (size_t step = 0; step < edgeSet.size(); ++step)
        {
            size_t next = SIZE_MAX;
            for (size_t n : adj[cur])
            {
                if (n != prev)
                {
                    next = n;
                    break;
                }
            }
            if (next == SIZE_MAX || next == start)
            {
                break;
            }
            outLoop.push_back(next);
            prev = cur;
            cur = next;
        }
        return outLoop.size() >= 3;
    }

    void MeshEditComponent::fillSelected(void)
    {
        std::vector<unsigned char> oldData = this->getMeshData();

        std::vector<size_t> loop;
        if (!this->findBorderLoop(loop) || loop.size() < 3)
        {
            return;
        }

        // Fan triangulate from loop[0]
        for (size_t i = 1; i + 1 < loop.size(); ++i)
        {
            this->indices.push_back(static_cast<Ogre::uint32>(loop[0]));
            this->indices.push_back(static_cast<Ogre::uint32>(loop[i]));
            this->indices.push_back(static_cast<Ogre::uint32>(loop[i + 1]));
        }
        this->indexCount = this->indices.size();

        this->buildVertexAdjacency();
        this->rebuildDynamicBuffers();
        this->scheduleOverlayUpdate();
        this->fireUndoEvent(oldData);
    }

    void MeshEditComponent::bevel(Ogre::Real amount)
    {
        if (this->selectedEdges.empty())
        {
            return;
        }

        std::vector<unsigned char> oldData = this->getMeshData();

        std::vector<Ogre::Vector3> nv = this->vertices, nn = this->normals;
        std::vector<Ogre::Vector4> nt = this->tangents;
        std::vector<Ogre::Vector2> nu = this->uvCoordinates;

        std::set<size_t> triToRemove;
        std::vector<std::array<size_t, 3>> newTris;
        std::set<size_t> processedEdgeTris;

        // For each face-edge pair, stores the bevel vertex idx for (orig_vertex, tri_idx)
        std::map<std::pair<size_t, size_t>, size_t> bevelVertMap;

        auto addBevelVert = [&](size_t origV, size_t triIdx, const Ogre::Vector3& pos) -> size_t
        {
            auto key = std::make_pair(origV, triIdx);
            auto it = bevelVertMap.find(key);
            if (it != bevelVertMap.end())
            {
                return it->second;
            }
            size_t idx = nv.size();
            nv.push_back(pos);
            nn.push_back(this->normals[origV]);
            nt.push_back(this->tangents[origV]);
            nu.push_back(this->uvCoordinates[origV]);
            bevelVertMap[key] = idx;
            return idx;
        };

        for (const EdgeKey& ek : this->selectedEdges)
        {
            const size_t ea = ek.a, eb = ek.b;

            // Find the two adjacent triangles
            struct FaceInfo
            {
                size_t triIdx;
                size_t vc;
            };
            std::vector<FaceInfo> adj;
            for (size_t i = 0; i < this->indexCount; i += 3)
            {
                bool hasA = (this->indices[i] == ea || this->indices[i + 1] == ea || this->indices[i + 2] == ea);
                bool hasB = (this->indices[i] == eb || this->indices[i + 1] == eb || this->indices[i + 2] == eb);
                if (!hasA || !hasB)
                {
                    continue;
                }
                size_t vc = SIZE_MAX;
                for (int k = 0; k < 3; ++k)
                {
                    size_t v = this->indices[i + k];
                    if (v != ea && v != eb)
                    {
                        vc = v;
                        break;
                    }
                }
                if (vc != SIZE_MAX)
                {
                    adj.push_back({i / 3, vc});
                }
            }
            if (adj.size() != 2)
            {
                continue;
            }

            // For each adjacent face, create two bevel vertices (one at ea, one at eb)
            // offset TOWARD the opposite vertex (vc) by amount
            size_t bvA[2], bvB[2]; // bvA[fi] = bevel vertex at ea for face fi, etc.
            for (size_t fi = 0; fi < 2; ++fi)
            {
                size_t triIdx = adj[fi].triIdx;
                size_t vc = adj[fi].vc;

                Ogre::Vector3 dirAtoC = (this->vertices[vc] - this->vertices[ea]).normalisedCopy();
                Ogre::Vector3 dirBtoC = (this->vertices[vc] - this->vertices[eb]).normalisedCopy();

                bvA[fi] = addBevelVert(ea, triIdx, this->vertices[ea] + dirAtoC * amount);
                bvB[fi] = addBevelVert(eb, triIdx, this->vertices[eb] + dirBtoC * amount);

                triToRemove.insert(triIdx);

                // Replacement triangle for this face (shrunk inward)
                newTris.push_back({bvA[fi], bvB[fi], vc});
            }

            // Bevel cap quad (bvA[0], bvB[0], bvB[1], bvA[1]) -> 2 tris
            newTris.push_back({bvA[0], bvB[0], bvB[1]});
            newTris.push_back({bvA[0], bvB[1], bvA[1]});

            // Small cap triangles at ea and eb connecting the two bevel vertices to the original
            newTris.push_back({ea, bvA[0], bvA[1]});
            newTris.push_back({eb, bvB[1], bvB[0]});
        }

        // Rebuild index buffer
        std::vector<Ogre::uint32> newIdx;
        newIdx.reserve(this->indexCount + newTris.size() * 3);
        for (size_t i = 0; i < this->indexCount; i += 3)
        {
            if (!triToRemove.count(i / 3))
            {
                newIdx.push_back(this->indices[i]);
                newIdx.push_back(this->indices[i + 1]);
                newIdx.push_back(this->indices[i + 2]);
            }
        }
        for (auto& tri : newTris)
        {
            newIdx.push_back(static_cast<Ogre::uint32>(tri[0]));
            newIdx.push_back(static_cast<Ogre::uint32>(tri[1]));
            newIdx.push_back(static_cast<Ogre::uint32>(tri[2]));
        }

        this->vertices = std::move(nv);
        this->normals = std::move(nn);
        this->tangents = std::move(nt);
        this->uvCoordinates = std::move(nu);
        this->indices = std::move(newIdx);
        this->vertexCount = this->vertices.size();
        this->indexCount = this->indices.size();
        this->selectedEdges.clear();
        this->buildVertexAdjacency();
        this->rebuildDynamicBuffers();
        this->scheduleOverlayUpdate();
        this->fireUndoEvent(oldData);
    }

    void MeshEditComponent::loopCut(Ogre::Real fraction)
    {
        if (this->selectedEdges.empty())
        {
            return;
        }

        const EdgeKey& guide = *this->selectedEdges.begin();
        const Ogre::Vector3& va = this->vertices[guide.a];
        const Ogre::Vector3& vb = this->vertices[guide.b];
        const Ogre::Vector3 edgeDir = (vb - va).normalisedCopy();
        const Ogre::Vector3 planePt = va + (vb - va) * fraction;

        std::vector<unsigned char> oldData = this->getMeshData();

        std::vector<Ogre::Vector3> nv = this->vertices, nn = this->normals;
        std::vector<Ogre::Vector4> nt = this->tangents;
        std::vector<Ogre::Vector2> nu = this->uvCoordinates;

        // Map canonical edge (min,max) -> new cut vertex index
        std::map<std::pair<size_t, size_t>, size_t> cutMap;

        auto getCutVertex = [&](size_t a, size_t b) -> size_t
        {
            auto key = std::make_pair(std::min(a, b), std::max(a, b));
            auto it = cutMap.find(key);
            if (it != cutMap.end())
            {
                return it->second;
            }

            float da = edgeDir.dotProduct(this->vertices[a] - planePt);
            float db = edgeDir.dotProduct(this->vertices[b] - planePt);
            if ((da < 0.f) == (db < 0.f))
            {
                return SIZE_MAX; // same side — no cut
            }

            float t = da / (da - db);
            size_t newI = nv.size();
            nv.push_back(this->vertices[a] + (this->vertices[b] - this->vertices[a]) * t);
            nn.push_back(((this->normals[a] * (1.f - t)) + (this->normals[b] * t)).normalisedCopy());
            nt.push_back(this->tangents[a] * (1.f - t) + this->tangents[b] * t);
            nu.push_back(this->uvCoordinates[a] * (1.f - t) + this->uvCoordinates[b] * t);
            cutMap[key] = newI;
            return newI;
        };

        std::vector<Ogre::uint32> newIdx;
        newIdx.reserve(this->indexCount * 2);

        auto push3 = [&](size_t x, size_t y, size_t z)
        {
            newIdx.push_back(static_cast<Ogre::uint32>(x));
            newIdx.push_back(static_cast<Ogre::uint32>(y));
            newIdx.push_back(static_cast<Ogre::uint32>(z));
        };

        for (size_t i = 0; i < this->indexCount; i += 3)
        {
            const size_t ia = this->indices[i], ib = this->indices[i + 1], ic = this->indices[i + 2];

            const float da = edgeDir.dotProduct(this->vertices[ia] - planePt);
            const float db = edgeDir.dotProduct(this->vertices[ib] - planePt);
            const float dc = edgeDir.dotProduct(this->vertices[ic] - planePt);

            const bool ab_cut = ((da < 0.f) != (db < 0.f));
            const bool bc_cut = ((db < 0.f) != (dc < 0.f));
            const bool ca_cut = ((dc < 0.f) != (da < 0.f));
            const int cuts = (int)ab_cut + (int)bc_cut + (int)ca_cut;

            if (cuts == 0)
            {
                push3(ia, ib, ic);
            }
            else if (cuts == 1)
            {
                // One edge cut -> 2 triangles
                if (ab_cut)
                {
                    size_t m = getCutVertex(ia, ib);
                    if (m == SIZE_MAX)
                    {
                        push3(ia, ib, ic);
                        continue;
                    }
                    push3(ia, m, ic);
                    push3(m, ib, ic);
                }
                else if (bc_cut)
                {
                    size_t m = getCutVertex(ib, ic);
                    if (m == SIZE_MAX)
                    {
                        push3(ia, ib, ic);
                        continue;
                    }
                    push3(ia, ib, m);
                    push3(ia, m, ic);
                }
                else // ca_cut
                {
                    size_t m = getCutVertex(ic, ia);
                    if (m == SIZE_MAX)
                    {
                        push3(ia, ib, ic);
                        continue;
                    }
                    push3(ia, ib, m);
                    push3(ib, ic, m);
                }
            }
            else if (cuts == 2)
            {
                // Two edges cut -> 3 triangles; the isolated vertex is on one side
                if (ab_cut && bc_cut)
                {
                    // ib is isolated
                    size_t mab = getCutVertex(ia, ib), mbc = getCutVertex(ib, ic);
                    if (mab == SIZE_MAX || mbc == SIZE_MAX)
                    {
                        push3(ia, ib, ic);
                        continue;
                    }
                    push3(mab, ib, mbc);
                    push3(ia, mab, mbc);
                    push3(ia, mbc, ic);
                }
                else if (bc_cut && ca_cut)
                {
                    // ic is isolated
                    size_t mbc = getCutVertex(ib, ic), mca = getCutVertex(ic, ia);
                    if (mbc == SIZE_MAX || mca == SIZE_MAX)
                    {
                        push3(ia, ib, ic);
                        continue;
                    }
                    push3(mbc, ic, mca);
                    push3(ia, ib, mbc);
                    push3(ia, mbc, mca);
                }
                else // ab_cut && ca_cut
                {
                    // ia is isolated
                    size_t mab = getCutVertex(ia, ib), mca = getCutVertex(ic, ia);
                    if (mab == SIZE_MAX || mca == SIZE_MAX)
                    {
                        push3(ia, ib, ic);
                        continue;
                    }
                    push3(ia, mab, mca);
                    push3(mab, ib, ic);
                    push3(mca, mab, ic);
                }
            }
            else
            {
                // All 3 edges cut (degenerate — keep original)
                push3(ia, ib, ic);
            }
        }

        this->vertices = std::move(nv);
        this->normals = std::move(nn);
        this->tangents = std::move(nt);
        this->uvCoordinates = std::move(nu);
        this->indices = std::move(newIdx);
        this->vertexCount = this->vertices.size();
        this->indexCount = this->indices.size();
        this->selectedEdges.clear();
        this->buildVertexAdjacency();
        this->rebuildDynamicBuffers();
        this->scheduleOverlayUpdate();
        this->fireUndoEvent(oldData);
    }

    Ogre::Matrix4 MeshEditComponent::getScreenMatrix() const
    {
        Ogre::Camera* cam = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
        return (cam ? cam->getProjectionMatrix() * cam->getViewMatrix() : Ogre::Matrix4::IDENTITY) * this->gameObjectPtr->getSceneNode()->_getFullTransform();
    }

    bool MeshEditComponent::projectVertexToScreen(size_t idx, float& nx, float& ny) const
    {
        Ogre::Camera* cam = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
        if (!cam)
        {
            return false;
        }

        const Ogre::Matrix4 vp = cam->getProjectionMatrix() * cam->getViewMatrix();
        const Ogre::Matrix4 world = this->gameObjectPtr->getSceneNode()->_getFullTransform();

        Ogre::Vector4 cp = vp * (world * Ogre::Vector4(this->vertices[idx], 1.0f));
        if (cp.w <= 1e-4f)
        {
            return false;
        }

        nx = (cp.x / cp.w + 1.0f) * 0.5f;
        ny = (1.0f - (cp.y / cp.w + 1.0f) * 0.5f);
        return true;
    }

    void MeshEditComponent::selectLinked(Ogre::Real sx, Ogre::Real sy, bool add)
    {
        const EditMode mode = this->getEditMode();

        // Collision-free edge encoding: upper 32 bits = min vertex, lower 32 = max vertex
        // Works for any mesh with < 4 billion vertices
        auto encodeEdge = [](size_t a, size_t b) -> uint64_t
        {
            if (a > b)
            {
                std::swap(a, b);
            }
            return (static_cast<uint64_t>(a) << 32) | static_cast<uint64_t>(b);
        };

        if (mode == EditMode::VERTEX)
        {
            size_t startIdx;
            if (!this->pickVertex(sx, sy, startIdx))
            {
                return;
            }

            std::vector<size_t> linked = floodSelect(startIdx,
                [this](size_t idx) -> std::vector<size_t>
                {
                    if (idx < this->vertexNeighbors.size())
                    {
                        return this->vertexNeighbors[idx];
                    }
                    return {};
                });

            if (!add)
            {
                this->selectedVertices.clear();
            }
            for (size_t v : linked)
            {
                this->selectedVertices.insert(v);
            }
            this->scheduleOverlayUpdate();
        }
        else if (mode == EditMode::EDGE)
        {
            EdgeKey startEdge(0, 0);
            if (!this->pickEdge(sx, sy, startEdge))
            {
                return;
            }

            // Build deduplicated edge list
            std::vector<EdgeKey> allEdges;
            {
                std::set<std::pair<size_t, size_t>> seen;
                for (size_t i = 0; i < this->indexCount; i += 3)
                {
                    for (int e = 0; e < 3; ++e)
                    {
                        size_t a = this->indices[i + e];
                        size_t b = this->indices[i + (e + 1) % 3];
                        if (seen.insert({std::min(a, b), std::max(a, b)}).second)
                        {
                            allEdges.push_back(EdgeKey(a, b));
                        }
                    }
                }
            }

            // vertex -> list of edge indices in allEdges
            std::unordered_map<size_t, std::vector<size_t>> vertToEdges;
            for (size_t i = 0; i < allEdges.size(); ++i)
            {
                vertToEdges[allEdges[i].a].push_back(i);
                vertToEdges[allEdges[i].b].push_back(i);
            }

            size_t startI = SIZE_MAX;
            for (size_t i = 0; i < allEdges.size(); ++i)
            {
                if (allEdges[i] == startEdge)
                {
                    startI = i;
                    break;
                }
            }
            if (startI == SIZE_MAX)
            {
                return;
            }

            std::vector<size_t> linked = floodSelect(startI,
                [&allEdges, &vertToEdges](size_t edgeI) -> std::vector<size_t>
                {
                    std::vector<size_t> nb;
                    size_t va = allEdges[edgeI].a;
                    size_t vb = allEdges[edgeI].b;
                    for (size_t v : std::array<size_t, 2>{va, vb}) // <- std::array statt initializer_list
                    {
                        auto it = vertToEdges.find(v);
                        if (it != vertToEdges.end())
                        {
                            for (size_t ei : it->second)
                            {
                                if (ei != edgeI)
                                {
                                    nb.push_back(ei);
                                }
                            }
                        }
                    }
                    return nb;
                });

            if (!add)
            {
                this->selectedEdges.clear();
            }
            for (size_t ei : linked)
            {
                this->selectedEdges.insert(allEdges[ei]);
            }
            this->scheduleOverlayUpdate();
        }
        else if (mode == EditMode::FACE)
        {
            size_t startFace;
            if (!this->pickFace(sx, sy, startFace))
            {
                return;
            }

            const size_t triCount = this->indexCount / 3;

            // edge key -> faces containing that edge
            std::unordered_map<uint64_t, std::vector<size_t>> edgeToFaces;
            for (size_t t = 0; t < triCount; ++t)
            {
                size_t i0 = this->indices[t * 3];
                size_t i1 = this->indices[t * 3 + 1];
                size_t i2 = this->indices[t * 3 + 2];
                edgeToFaces[encodeEdge(i0, i1)].push_back(t);
                edgeToFaces[encodeEdge(i1, i2)].push_back(t);
                edgeToFaces[encodeEdge(i2, i0)].push_back(t);
            }

            std::vector<size_t> linked = floodSelect(startFace, [this, &edgeToFaces, &encodeEdge](size_t fIdx) -> std::vector<size_t>
                {
                    std::vector<size_t> nb;
                    size_t i0 = this->indices[fIdx * 3];
                    size_t i1 = this->indices[fIdx * 3 + 1];
                    size_t i2 = this->indices[fIdx * 3 + 2];

                    std::array<uint64_t, 3> keys = {encodeEdge(i0, i1), encodeEdge(i1, i2), encodeEdge(i2, i0)};
                    for (uint64_t eKey : keys)
                    {
                        auto it = edgeToFaces.find(eKey);
                        if (it != edgeToFaces.end())
                        {
                            for (size_t f : it->second)
                            {
                                if (f != fIdx)
                                {
                                    nb.push_back(f);
                                }
                            }
                        }
                    }
                    return nb;
                });

            if (!add)
            {
                this->selectedFaces.clear();
            }
            for (size_t f : linked)
            {
                this->selectedFaces.insert(f);
            }
            this->scheduleOverlayUpdate();
        }
    }

#if 0
    void MeshEditComponent::createStatusOverlay(void)
    {
        NOWA::GraphicsModule::RenderCommand cmd = [this]()
        {
            this->statusWidget = MyGUI::Gui::getInstance().createWidget<MyGUI::TextBox>("TextBox", MyGUI::IntCoord(10, 10, 500, 26), MyGUI::Align::Default, "Overlapped");
            this->statusWidget->setTextColour(MyGUI::Colour(1.0f, 0.9f, 0.2f, 1.0f));
            this->statusWidget->setFontHeight(16);
            this->statusWidget->setVisible(false);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "MeshEditComponent::createStatusOverlay");
    }

    void MeshEditComponent::destroyStatusOverlay(void)
    {
        NOWA::GraphicsModule::RenderCommand cmd = [this]()
        {
            if (this->statusWidget)
            {
                MyGUI::Gui::getInstance().destroyWidget(this->statusWidget);
                this->statusWidget = nullptr;
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "MeshEditComponent::destroyStatusOverlay");
    }

    void MeshEditComponent::updateStatusOverlay(void)
    {
        if (!this->statusWidget) return;
 
        Ogre::String text;
        const bool active = this->isEditorMeshModifyMode && this->isSelected && this->activated->getBool();
 
        if (this->isGrabbing)
        {
            text = "[GRAB]";
            if (this->isScaling)
            {
                text += "  |  SCALE  (mouse left/right)";
            }
            else
            {
                switch (this->grabAxis)
                {
                case GrabAxis::X: text += "  |  AXIS: X"; break;
                case GrabAxis::Y: text += "  |  AXIS: Y (up)"; break;
                case GrabAxis::Z: text += "  |  AXIS: Z (depth)"; break;
                default:          text += "  |  FREE"; break;
                }
            }
            text += "  |  delta: " + Ogre::StringConverter::toString(this->grabAccumDelta, 2);
            if (this->isProportionalEditing) text += "  [O: Proportional]";
        }
        else
        {
            text = "  Mode: " + this->getEditModeString();
            if (this->isBrushArmed)         text += "  [B: Brush]";
            if (this->isProportionalEditing) text += "  [O: Proportional]";
 
            size_t selCount = 0;
            switch (this->getEditMode())
            {
            case EditMode::VERTEX: selCount = this->selectedVertices.size(); break;
            case EditMode::EDGE:   selCount = this->selectedEdges.size();    break;
            case EditMode::FACE:   selCount = this->selectedFaces.size();    break;
            default: break;
            }
            if (selCount > 0)
                text += "  |  Sel: " + Ogre::StringConverter::toString(static_cast<int>(selCount));
        }
 
        NOWA::GraphicsModule::RenderCommand cmd = [this, text, active]()
        {
            if (this->statusWidget)
            {
                this->statusWidget->setCaption(text);
                this->statusWidget->setVisible(active);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueue(std::move(cmd), "MeshEditComponent::updateStatus");
    }
#endif





    // =========================================================================
    //  Brush
    // =========================================================================

    Ogre::Real MeshEditComponent::calculateBrushInfluence(Ogre::Real dist, Ogre::Real radius) const
    {
        if (dist >= radius)
        {
            return 0.0f;
        }

        const Ogre::Real nd = dist / radius;
        Ogre::Real inf = std::pow(1.0f - nd, this->brushFalloff->getReal());

        if (!this->brushData.empty() && this->brushImageWidth > 0 && this->brushImageHeight > 0)
        {
            const size_t centerX = this->brushImageWidth / 2;
            const size_t centerY = this->brushImageHeight / 2;

            size_t sampleX = static_cast<size_t>(centerX + nd * centerX);
            size_t sampleY = centerY;

            sampleX = std::min(sampleX, this->brushImageWidth - 1);
            sampleY = std::min(sampleY, this->brushImageHeight - 1);

            const size_t brushIndex = sampleY * this->brushImageWidth + sampleX;
            if (brushIndex < this->brushData.size())
            {
                inf *= this->brushData[brushIndex];
            }
        }
        return inf * this->brushIntensity->getReal();
    }

    void MeshEditComponent::applyBrush(const Ogre::Vector3& ctr, bool invert)
    {
        Ogre::Real brushRadius = this->brushSize->getReal();
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

        // ── Pass 1: find directly affected vertices + compute averages ────────────
        Ogre::Vector3 avgP = Ogre::Vector3::ZERO;
        Ogre::Vector3 avgN = Ogre::Vector3::ZERO;
        std::vector<std::pair<size_t, Ogre::Real>> aff; // (index, influence)

        for (size_t i = 0; i < this->vertexCount; ++i)
        {
            const Ogre::Real d = this->vertices[i].distance(ctr);
            if (d < brushRadius)
            {
                const Ogre::Real inf = this->calculateBrushInfluence(d, brushRadius);
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

        // ── Pass 2: apply displacement ────────────────────────────────────────────
        // For each directly affected vertex, compute its delta THEN apply the same
        // delta to every co-located group member — closes all UV/hard-edge gaps.
        for (const auto& [idx, inf] : aff)
        {
            Ogre::Vector3 delta = Ogre::Vector3::ZERO;

            switch (mode)
            {
            case BrushMode::PUSH:
                delta = this->normals[idx] * inf;
                break;
            case BrushMode::PULL:
                delta = -this->normals[idx] * inf;
                break;
            case BrushMode::SMOOTH:
                if (idx < this->vertexNeighbors.size() && !this->vertexNeighbors[idx].empty())
                {
                    Ogre::Vector3 nb = Ogre::Vector3::ZERO;
                    for (size_t n : this->vertexNeighbors[idx])
                    {
                        nb += this->vertices[n];
                    }
                    nb /= static_cast<Ogre::Real>(this->vertexNeighbors[idx].size());
                    delta = (nb - this->vertices[idx]) * inf;
                }
                break;
            case BrushMode::FLATTEN:
            {
                const Ogre::Real dp = avgN.dotProduct(this->vertices[idx] - avgP);
                delta = -avgN * dp * inf;
            }
            break;
            case BrushMode::PINCH:
                delta = (ctr - this->vertices[idx]).normalisedCopy() * inf;
                break;
            case BrushMode::INFLATE:
                delta = avgN * inf; // area-averaged normal = balloon
                break;
            }

            if (delta.squaredLength() < 1e-14f)
            {
                continue;
            }

            // Apply delta to the canonical vertex and ALL co-located siblings
            this->vertices[idx] += delta;

            const int g = this->vertexGroup[idx];
            if (g >= 0)
            {
                for (size_t sibling : this->positionGroups[static_cast<size_t>(g)])
                {
                    if (sibling != idx)
                    {
                        this->vertices[sibling] += delta;
                    }
                }
            }
        }

        this->recalculateNormals_internal();
        this->recalculateTangents();
        this->uploadVertexData();
        this->scheduleOverlayUpdate();
    }

    void MeshEditComponent::setProportionalRadius(Ogre::Real radius)
    {
        this->proportionalRadius->setValue(radius);
    }

    Ogre::Real MeshEditComponent::getProportionalRadius(void) const
    {
        return this->proportionalRadius->getReal();
    }

    bool MeshEditComponent::getIsProportionalEditing(void) const
    {
        return this->isProportionalEditing;
    }

    void MeshEditComponent::setBevelAmount(Ogre::Real amount)
    {
        this->bevelAmount->setValue(amount);
    }

    Ogre::Real MeshEditComponent::getBevelAmount(void) const
    {
        return this->bevelAmount->getReal();
    }

    void MeshEditComponent::setLoopCutFraction(Ogre::Real fraction)
    {
        this->loopCutFraction->setValue(Ogre::Math::Clamp(fraction, 0.01f, 0.99f));
    }

    Ogre::Real MeshEditComponent::getLoopCutFraction(void) const
    {
        return this->loopCutFraction->getReal();
    }

    // =============================================================================
    //  applyOriginAlignment
    //
    //  Computes the AABB of all mesh vertices and offsets every vertex so that the
    //  chosen bounding-box corner / edge centre / face centre becomes (0,0,0).
    //
    //  Ogre coordinate convention:
    //    X = right (+) / left (-)
    //    Y = up    (+) / down (-)   ->  Top = +Y, Bottom = -Y
    //    Z = toward viewer (+) / away from viewer (-)  ->  Front = +Z, Back = -Z
    //
    //  The SceneNode position is NOT changed.  The mesh-space origin moves; the
    //  visual result is identical until the user repositions the node.
    // =============================================================================
    void MeshEditComponent::applyOriginAlignment(void)
    {
        if (this->vertexCount == 0)
        {
            return;
        }

        std::vector<unsigned char> oldData = this->getMeshData();

        // Compute AABB in local mesh space
        Ogre::Vector3 minV(std::numeric_limits<float>::max());
        Ogre::Vector3 maxV(-std::numeric_limits<float>::max());
        for (const auto& v : this->vertices)
        {
            minV.makeFloor(v);
            maxV.makeCeil(v);
        }
        const Ogre::Vector3 center = (minV + maxV) * 0.5f;

        // Decode the preset string into a pivot point.
        // Format keywords: Bottom/Center/Top for Y, Left/Center/Right for X,
        //                  Front/Center/Back for Z.
        const Ogre::String preset = this->originAlignment->getListSelectedValue();

        // ── Y (vertical) ─────────────────────────────────────────────────────────
        float pivotY = center.y;
        if (preset.find("Bottom") != Ogre::String::npos)
        {
            pivotY = minV.y;
        }
        else if (preset.find("Top") != Ogre::String::npos)
        {
            pivotY = maxV.y;
        }

        // ── X (horizontal) ───────────────────────────────────────────────────────
        float pivotX = center.x;
        if (preset.find("Left") != Ogre::String::npos)
        {
            pivotX = minV.x;
        }
        else if (preset.find("Right") != Ogre::String::npos)
        {
            pivotX = maxV.x;
        }

        // ── Z (depth) ─────────────────────────────────────────────────────────────
        // Front = +Z (toward viewer), Back = -Z (away from viewer)
        float pivotZ = center.z;
        if (preset.find("Front") != Ogre::String::npos)
        {
            pivotZ = maxV.z;
        }
        else if (preset.find("Back") != Ogre::String::npos)
        {
            pivotZ = minV.z;
        }

        const Ogre::Vector3 pivot(pivotX, pivotY, pivotZ);

        // Translate all vertices so that pivot becomes the local origin
        for (auto& v : this->vertices)
        {
            v -= pivot;
        }

        // Normals are direction vectors — they are NOT affected by translation
        // (only by rotation/scale). No normal update needed.

        this->buildVertexAdjacency();
        this->rebuildDynamicBuffers();
        this->scheduleOverlayUpdate();
        this->fireUndoEvent(oldData);

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MeshEditComponent] Origin aligned to '" + preset + "'" + " pivot=" + Ogre::StringConverter::toString(pivot));
    }

    // =============================================================================
    //  applyFlipDirection
    //
    //  Mirrors all vertex positions across the chosen axis (negates that component).
    //  Also mirrors normals (same negation) and reverses triangle winding so that
    //  faces remain front-facing after the flip.
    //
    //  This is a true geometric mirror — equivalent to Blender's
    //  Object > Apply > Scale (-1, 1, 1) for the X axis etc.
    // =============================================================================
    void MeshEditComponent::applyFlipDirection(void)
    {
        if (this->vertexCount == 0)
        {
            return;
        }

        std::vector<unsigned char> oldData = this->getMeshData();

        const Ogre::String axis = this->flipDirection->getListSelectedValue();

        // Choose which component to negate
        enum class Axis
        {
            X,
            Y,
            Z
        } flipAxis = Axis::X;
        if (axis == "Y")
        {
            flipAxis = Axis::Y;
        }
        else if (axis == "Z")
        {
            flipAxis = Axis::Z;
        }

        // Mirror vertex positions and normals across the chosen axis
        for (size_t i = 0; i < this->vertexCount; ++i)
        {
            switch (flipAxis)
            {
            case Axis::X:
                this->vertices[i].x = -this->vertices[i].x;
                this->normals[i].x = -this->normals[i].x;
                // Tangent handedness (w) must also be flipped to stay consistent
                this->tangents[i].x = -this->tangents[i].x;
                break;
            case Axis::Y:
                this->vertices[i].y = -this->vertices[i].y;
                this->normals[i].y = -this->normals[i].y;
                this->tangents[i].y = -this->tangents[i].y;
                break;
            case Axis::Z:
                this->vertices[i].z = -this->vertices[i].z;
                this->normals[i].z = -this->normals[i].z;
                this->tangents[i].z = -this->tangents[i].z;
                break;
            }
        }

        // A mirror operation changes handedness — reverse triangle winding to
        // keep faces front-facing (same as flipNormals does for winding).
        for (size_t i = 0; i < this->indexCount; i += 3)
        {
            std::swap(this->indices[i + 1], this->indices[i + 2]);
        }

        // Recalculate smooth normals from the mirrored geometry so hard edges
        // along the mirror plane remain correct.
        this->recalculateNormals_internal();
        this->recalculateTangents();

        this->buildVertexAdjacency();
        this->rebuildDynamicBuffers();
        this->scheduleOverlayUpdate();
        this->fireUndoEvent(oldData);

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MeshEditComponent] Flipped across " + axis + " axis.");
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
        luabind::module(lua)
        [
            luabind::class_<MeshEditComponent, GameObjectComponent>("MeshEditComponent")
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
            .def("getOutputFileName", &MeshEditComponent::getOutputFileName)
            .def("mergeSelected",        &MeshEditComponent::mergeSelected)
            .def("dissolveSelected",     &MeshEditComponent::dissolveSelected)
            .def("fillSelected",         &MeshEditComponent::fillSelected)
            .def("bevel",                &MeshEditComponent::bevel)
            .def("loopCut",              &MeshEditComponent::loopCut)
            .def("setProportionalRadius",&MeshEditComponent::setProportionalRadius)
            .def("getProportionalRadius",&MeshEditComponent::getProportionalRadius)
            .def("setBevelAmount",       &MeshEditComponent::setBevelAmount)
            .def("getBevelAmount",       &MeshEditComponent::getBevelAmount)
            .def("setLoopCutFraction",   &MeshEditComponent::setLoopCutFraction)
            .def("getLoopCutFraction",   &MeshEditComponent::getLoopCutFraction)
        ];
    }

}; // namespace NOWA
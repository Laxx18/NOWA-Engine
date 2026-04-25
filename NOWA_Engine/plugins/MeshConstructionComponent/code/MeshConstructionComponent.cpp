/*
Copyright (c) 2026 Lukas Kalinowski

GPL v3
*/

#include "NOWAPrecompiled.h"
#include "MeshConstructionComponent.h"
#include "gameobject/GameObjectFactory.h"
#include "utilities/MovableText.h"
#include "main/AppStateManager.h"
#include "main/EventManager.h"
#include "modules/LuaScriptApi.h"
#include "utilities/MathHelper.h"
#include "utilities/XMLConverter.h"

#include "OgreAbiUtils.h"
#include "OgreBitwise.h"
#include "OgreHlmsPbsDatablock.h"
#include "OgreItem.h"
#include "OgreManualObject2.h"
#include "OgreMesh2.h"
#include "OgreRoot.h"
#include "OgreSubMesh2.h"
#include "Vao/OgreAsyncTicket.h"
#include "Vao/OgreVaoManager.h"
#include "Vao/OgreVertexArrayObject.h"

#include <algorithm>

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    // =========================================================================
    //  Constructor
    // =========================================================================

    MeshConstructionComponent::MeshConstructionComponent() :
        GameObjectComponent(),
        componentName("MeshConstructionComponent"),
        meshMinY(0.0f),
        meshMaxY(1.0f),
        meshXZRadius(1.0f),
        vertexCount(0),
        indexCount(0),
        constructionItem(nullptr),
        barLineNode(nullptr),
        barObject(nullptr),
        barCouldDraw(false),
        textNode(nullptr),
        percentageText(nullptr),
        glowLineNode(nullptr),
        glowObject(nullptr),
        glowCouldDraw(false),
        elapsed(0.0f),
        constructionProgress(0.0f),
        isConstructionFinished(false),

        activated(new Variant(MeshConstructionComponent::AttrActivated(), true, this->attributes)),
        constructionTime(new Variant(MeshConstructionComponent::AttrConstructionTime(), 5.0f, this->attributes)),
        showProgressBar(new Variant(MeshConstructionComponent::AttrShowProgressBar(), true, this->attributes)),
        showPercentageText(new Variant(MeshConstructionComponent::AttrShowPercentageText(), true, this->attributes))
    {
        this->constructionTime->setConstraints(0.1f, 3600.0f);
        this->constructionTime->setDescription("Total time in seconds for the mesh to fully materialise.");
        this->showProgressBar->setDescription("Show a camera-facing progress bar above the object.");
        this->showPercentageText->setDescription("Show a percentage text label above the progress bar.");
    }

    MeshConstructionComponent::~MeshConstructionComponent()
    {
    }

    // =========================================================================
    //  Ogre::Plugin
    // =========================================================================

    void MeshConstructionComponent::install(const Ogre::NameValuePairList* /*options*/)
    {
        GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<MeshConstructionComponent>(MeshConstructionComponent::getStaticClassId(), MeshConstructionComponent::getStaticClassName());
    }

    const Ogre::String& MeshConstructionComponent::getName() const
    {
        return this->componentName;
    }

    void MeshConstructionComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
    {
        outAbiCookie = Ogre::generateAbiCookie();
    }

    // =========================================================================
    //  Trivial accessors
    // =========================================================================

    Ogre::String MeshConstructionComponent::getClassName(void) const
    {
        return "MeshConstructionComponent";
    }
    Ogre::String MeshConstructionComponent::getParentClassName(void) const
    {
        return "GameObjectComponent";
    }
    bool MeshConstructionComponent::isActivated(void) const
    {
        return this->activated->getBool();
    }
    Ogre::Real MeshConstructionComponent::getConstructionProgress(void) const
    {
        return this->constructionProgress;
    }
    bool MeshConstructionComponent::getIsConstructionFinished(void) const
    {
        return this->isConstructionFinished;
    }

    void MeshConstructionComponent::setConstructionTime(Ogre::Real s)
    {
        this->constructionTime->setValue(std::max(0.1f, s));
    }
    Ogre::Real MeshConstructionComponent::getConstructionTime(void) const
    {
        return this->constructionTime->getReal();
    }
    void MeshConstructionComponent::setShowProgressBar(bool v)
    {
        this->showProgressBar->setValue(v);
    }
    bool MeshConstructionComponent::getShowProgressBar(void) const
    {
        return this->showProgressBar->getBool();
    }
    void MeshConstructionComponent::setShowPercentageText(bool v)
    {
        this->showPercentageText->setValue(v);
    }
    bool MeshConstructionComponent::getShowPercentageText(void) const
    {
        return this->showPercentageText->getBool();
    }

    void MeshConstructionComponent::reactOnConstructionDone(luabind::object fn)
    {
        this->closureFunction = fn;
    }

    Ogre::String MeshConstructionComponent::trackedClosureId(void) const
    {
        return this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
    }

    // =========================================================================
    //  init
    // =========================================================================

    bool MeshConstructionComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        GameObjectComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrActivated())
        {
            this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrConstructionTime())
        {
            this->constructionTime->setValue(XMLConverter::getAttribReal(propertyElement, "data", 5.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrShowProgressBar())
        {
            this->showProgressBar->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrShowPercentageText())
        {
            this->showPercentageText->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
            propertyElement = propertyElement->next_sibling("property");
        }
        return true;
    }

    // =========================================================================
    //  postInit  — editor-safe, read-only (no resource creation, no swap)
    // =========================================================================

    bool MeshConstructionComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MeshConstructionComponent] postInit: " + this->gameObjectPtr->getName());

        Ogre::Item* item = this->gameObjectPtr->getMovableObject<Ogre::Item>();
        if (nullptr == item)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MeshConstructionComponent] No Ogre::Item on: " + this->gameObjectPtr->getName());
            return false;
        }

        this->originalMeshName = item->getMesh()->getName();

        // Rough bounds from stored AABB (refined from actual vertices in connect()).
        const Ogre::Aabb& bounds = item->getMesh()->getAabb();
        this->meshMinY = bounds.getMinimum().y;
        this->meshMaxY = bounds.getMaximum().y;
        if (this->meshMinY >= this->meshMaxY)
        {
            this->meshMinY = 0.0f;
            this->meshMaxY = 1.0f;
        }

        return true;
    }

    // =========================================================================
    //  connect  — simulation starts
    // =========================================================================

    bool MeshConstructionComponent::connect(void)
    {
        GameObjectComponent::connect();

        if (false == this->activated->getBool())
        {
            return true;
        }

        bool success = false;
        NOWA::GraphicsModule::RenderCommand buildCmd = [this, &success]()
        {
            if (false == this->extractMeshDataAndCreateBuffers())
            {
                return;
            }

            Ogre::MovableObject* old = nullptr;
            this->gameObjectPtr->swapMovableObject(this->constructionItem, old);
            if (nullptr != old)
            {
                this->gameObjectPtr->getSceneManager()->destroyItem(static_cast<Ogre::Item*>(old));
            }

            this->createOverlays();
            success = true;
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(buildCmd), "MeshConstructionComponent::connect");

        if (false == success)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MeshConstructionComponent] connect() failed for: " + this->gameObjectPtr->getName());
            return false;
        }

        this->elapsed = 0.0f;
        this->constructionProgress = 0.0f;
        this->isConstructionFinished = false;
        this->barCouldDraw = false;
        this->glowCouldDraw = false;

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MeshConstructionComponent] Started for: " + this->gameObjectPtr->getName());
        return true;
    }

    // =========================================================================
    //  disconnect  — simulation ends, restore original Item
    // =========================================================================

    bool MeshConstructionComponent::disconnect(void)
    {
        GameObjectComponent::disconnect();

        NOWA::GraphicsModule::getInstance()->removeTrackedClosure(this->trackedClosureId());

        ENQUEUE_RENDER_COMMAND("MeshConstructionComponent::disconnect", {
            this->destroyOverlays();

            if (nullptr != this->constructionItem)
            {
                Ogre::Item* restored = nullptr;
                try
                {
                    restored = this->gameObjectPtr->getSceneManager()->createItem(this->originalMeshName, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME, this->gameObjectPtr->isDynamic() ? Ogre::SCENE_DYNAMIC : Ogre::SCENE_STATIC);
                }
                catch (const Ogre::Exception& e)
                {
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MeshConstructionComponent] Restore failed: " + e.getDescription());
                }

                Ogre::MovableObject* old = nullptr;
                this->gameObjectPtr->swapMovableObject(restored, old);
                if (nullptr != old)
                {
                    this->gameObjectPtr->getSceneManager()->destroyItem(static_cast<Ogre::Item*>(old));
                }
                this->constructionItem = nullptr;
                this->destroyConstructionResources();
            }
        });

        this->vertices.clear();
        this->normals.clear();
        this->tangents.clear();
        this->uvCoordinates.clear();
        this->indices.clear();
        this->subMeshInfoList.clear();
        this->vertexCount = 0;
        this->indexCount = 0;
        this->elapsed = 0.0f;
        this->constructionProgress = 0.0f;
        this->isConstructionFinished = false;
        this->barCouldDraw = false;
        this->glowCouldDraw = false;

        return true;
    }

    // =========================================================================
    //  onRemoveComponent
    // =========================================================================

    void MeshConstructionComponent::onRemoveComponent(void)
    {
        GameObjectComponent::onRemoveComponent();

        NOWA::GraphicsModule::getInstance()->removeTrackedClosure(this->trackedClosureId());

        ENQUEUE_RENDER_COMMAND("MeshConstructionComponent::onRemoveComponent", {
            this->destroyOverlays();
            if (nullptr != this->constructionItem)
            {
                Ogre::Item* restored = nullptr;
                try
                {
                    restored = this->gameObjectPtr->getSceneManager()->createItem(this->originalMeshName, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME, this->gameObjectPtr->isDynamic() ? Ogre::SCENE_DYNAMIC : Ogre::SCENE_STATIC);
                }
                catch (...)
                {
                }

                Ogre::MovableObject* old = nullptr;
                this->gameObjectPtr->swapMovableObject(restored, old);
                if (nullptr != old)
                {
                    this->gameObjectPtr->getSceneManager()->destroyItem(static_cast<Ogre::Item*>(old));
                }
                this->constructionItem = nullptr;
                this->destroyConstructionResources();
            }
        });
    }

    // =========================================================================
    //  clone
    // =========================================================================

    GameObjectCompPtr MeshConstructionComponent::clone(GameObjectPtr cloned)
    {
        MeshConstructionComponentPtr comp(boost::make_shared<MeshConstructionComponent>());
        comp->setActivated(this->activated->getBool());
        comp->setConstructionTime(this->constructionTime->getReal());
        comp->setShowProgressBar(this->showProgressBar->getBool());
        comp->setShowPercentageText(this->showPercentageText->getBool());
        cloned->addComponent(comp);
        comp->setOwner(cloned);
        GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(comp));
        return comp;
    }

    // =========================================================================
    //  setActivated
    // =========================================================================

    void MeshConstructionComponent::setActivated(bool activated)
    {
        this->activated->setValue(activated);

        if (false == activated && this->bConnected)
        {
            NOWA::GraphicsModule::getInstance()->removeTrackedClosure(this->trackedClosureId());

            ENQUEUE_RENDER_COMMAND("MeshConstructionComponent::setActivated::hide", {
                if (nullptr != this->barLineNode)
                {
                    this->barLineNode->setVisible(false);
                }
                if (nullptr != this->glowLineNode)
                {
                    this->glowLineNode->setVisible(false);
                }
                if (nullptr != this->textNode)
                {
                    this->textNode->setVisible(false);
                }
            });
        }
    }

    // =========================================================================
    //  update  — logic thread
    //
    //  1. Advance timer, compute cutoffY (local mesh space).
    //  2. Build index packets on logic thread (binary search + degenerate triangles).
    //  3. updateTrackedClosure to upload + redraw all overlays on render thread.
    //
    //  Drawing follows ValueBarComponent::drawValueBar() EXACTLY:
    //    p  = world position of bar/ring center (precomputed with scale)
    //    o  = Quaternion::IDENTITY  (strip object rotation for camera-facing)
    //    so = MathHelper::lookAt(objPos - cameraPos)  (camera-facing)
    //    sp = Vector3::ZERO  (offset already baked into p)
    //    vertex = p + (o * (so * (sp + localOffset)))
    // =========================================================================

    void MeshConstructionComponent::update(Ogre::Real dt, bool notSimulating)
    {
        if (true == notSimulating || false == this->activated->getBool())
        {
            return;
        }
        if (this->subMeshInfoList.empty())
        {
            return;
        }

        // ── 1. Timer ──────────────────────────────────────────────────────────────
        this->elapsed += dt;
        this->constructionProgress = Ogre::Math::Clamp(this->elapsed / this->constructionTime->getReal(), 0.0f, 1.0f);

        // cutoffY in LOCAL mesh space.
        // At progress == 1.0 add epsilon so upper_bound includes the very top triangles.
        float cutoffY = static_cast<float>(this->meshMinY + this->constructionProgress * (this->meshMaxY - this->meshMinY));
        if (this->constructionProgress >= 1.0f)
        {
            cutoffY = this->meshMaxY + 1e-3f;
        }

        // ── 2. Build index packets on logic thread ────────────────────────────────
        struct IndexPacket
        {
            Ogre::IndexBufferPacked* buffer;
            std::vector<Ogre::uint16> data16;
            std::vector<Ogre::uint32> data32;
            size_t elementCount;
            bool is16;
        };

        std::vector<IndexPacket> packets;
        packets.reserve(this->subMeshInfoList.size());

        for (const SubMeshInfo& sm : this->subMeshInfoList)
        {
            if (nullptr == sm.indexBuffer || sm.sortedTriIndices.empty())
            {
                continue;
            }

            const auto it = std::upper_bound(sm.sortedCentroidY.begin(), sm.sortedCentroidY.end(), cutoffY);
            const size_t visibleTris = static_cast<size_t>(std::distance(sm.sortedCentroidY.begin(), it));
            const size_t triCount = sm.indexCount / 3;
            const bool use16 = (sm.vertexCount <= 65535u);

            IndexPacket pkt;
            pkt.buffer = sm.indexBuffer;
            pkt.elementCount = sm.indexCount;
            pkt.is16 = use16;

            if (use16)
            {
                pkt.data16.assign(sm.indexCount, 0u);
            }
            else
            {
                pkt.data32.assign(sm.indexCount, 0u);
            }

            for (size_t t = 0; t < visibleTris && t < triCount; ++t)
            {
                const size_t origTri = sm.sortedTriIndices[t];
                const size_t srcBase = sm.indexOffset + origTri * 3;
                const size_t dstBase = origTri * 3;

                for (int k = 0; k < 3; ++k)
                {
                    const size_t localIdx = this->indices[srcBase + k] - sm.vertexOffset;
                    if (use16)
                    {
                        pkt.data16[dstBase + k] = static_cast<Ogre::uint16>(localIdx);
                    }
                    else
                    {
                        pkt.data32[dstBase + k] = static_cast<Ogre::uint32>(localIdx);
                    }
                }
            }
            packets.push_back(std::move(pkt));
        }

        // ── 3. Capture all data the render closure needs ──────────────────────────
        const Ogre::Real capturedProgress = this->constructionProgress;
        const float capturedCutoffY = cutoffY; // local mesh space
        const bool doBar = this->showProgressBar->getBool();
        const bool doText = this->showPercentageText->getBool();
        const int pct = static_cast<int>(capturedProgress * 100.0f);
        const Ogre::String pctStr = Ogre::StringConverter::toString(pct) + " %";
        const float capturedMeshMinY = this->meshMinY;
        const float capturedMeshMaxY = this->meshMaxY;
        const float capturedXZRadius = this->meshXZRadius;

        // ── 4. updateTrackedClosure ───────────────────────────────────────────────
        auto renderClosure = [this, packets = std::move(packets), capturedProgress, capturedCutoffY, doBar, doText, pctStr, capturedMeshMinY, capturedMeshMaxY, capturedXZRadius](Ogre::Real /*renderDt*/) mutable
        {
            // ── a) Upload index buffers ────────────────────────────────────────────
            for (IndexPacket& pkt : packets)
            {
                if (nullptr == pkt.buffer)
                {
                    continue;
                }
                if (pkt.is16)
                {
                    Ogre::uint16* buf = reinterpret_cast<Ogre::uint16*>(OGRE_MALLOC_SIMD(pkt.elementCount * sizeof(Ogre::uint16), Ogre::MEMCATEGORY_GEOMETRY));
                    std::memcpy(buf, pkt.data16.data(), pkt.elementCount * sizeof(Ogre::uint16));
                    pkt.buffer->upload(buf, 0, pkt.elementCount);
                    OGRE_FREE_SIMD(buf, Ogre::MEMCATEGORY_GEOMETRY);
                }
                else
                {
                    Ogre::uint32* buf = reinterpret_cast<Ogre::uint32*>(OGRE_MALLOC_SIMD(pkt.elementCount * sizeof(Ogre::uint32), Ogre::MEMCATEGORY_GEOMETRY));
                    std::memcpy(buf, pkt.data32.data(), pkt.elementCount * sizeof(Ogre::uint32));
                    pkt.buffer->upload(buf, 0, pkt.elementCount);
                    OGRE_FREE_SIMD(buf, Ogre::MEMCATEGORY_GEOMETRY);
                }
            }

            // ── Grab render-thread-safe transforms ────────────────────────────────
            // Accessing _getDerivedPosition/Scale/Orientation on the render thread
            // is correct — that is exactly what ValueBarComponent::drawValueBar does
            // (it reads getPosition()/getOrientation() inside updateTrackedClosure).

            Ogre::Camera* cam = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
            if (nullptr == cam)
            {
                return;
            }

            const Ogre::Vector3 worldPos = this->gameObjectPtr->getSceneNode()->_getDerivedPosition();
            const Ogre::Vector3 scale = this->gameObjectPtr->getSceneNode()->_getDerivedScale();

            // Camera-facing quaternion — same technique as ValueBarComponent when
            // an orientation target (the camera) is given:
            //   so = MathHelper::lookAt(objectPos - targetPos)
            //   o  = Quaternion::IDENTITY  (strip object rotation)
            const Ogre::Vector3 direction = worldPos - cam->getDerivedPosition();
            const Ogre::Quaternion so = MathHelper::getInstance()->lookAt(direction);
            const Ogre::Quaternion o = Ogre::Quaternion::IDENTITY;

            // ── b) Progress bar ───────────────────────────────────────────────────
            //
            // Following ValueBarComponent::drawValueBar EXACTLY:
            //   - lineNode at root (no transform) → world-space vertex positions
            //   - Material: WhiteNoLightingBackground
            //   - vertex = p + (o * (so * (sp + localOffset)))
            //   - sp = ZERO (offset already baked into p)
            //   - couldDraw guard against empty ManualObject crashes

            if (nullptr != this->barObject && doBar)
            {
                this->barCouldDraw = false;

                // p = bar centre in world space.
                // Y = world top of mesh + fixed world-space gap (NOT scaled — we always
                // want the bar to be readable regardless of object scale).
                const Ogre::Real barGap = 0.35f;
                const Ogre::Real barWorldY = worldPos.y + capturedMeshMaxY * scale.y + barGap;
                const Ogre::Vector3 p(worldPos.x, barWorldY, worldPos.z);
                const Ogre::Vector3 sp = Ogre::Vector3::ZERO;

                // Bar dimensions in world space (fixed, not scaled).
                const Ogre::Real w = 0.5f;   // half-width
                const Ogre::Real h = 0.07f;  // height
                const Ogre::Real bs = 0.01f; // border

                // Fill width (left-aligned: from -w to -w + fillW).
                const Ogre::Real fillW = (w * 2.0f) * static_cast<Ogre::Real>(capturedProgress);
                const Ogre::Real fillRight = -w + fillW;

                const Ogre::ColourValue outerCol(0.15f, 0.15f, 0.15f, 1.0f);
                const Ogre::ColourValue fillTopCol(0.15f, 0.9f, 0.15f, 1.0f);
                const Ogre::ColourValue fillBotCol(0.10f, 0.65f, 0.10f, 1.0f);

                if (this->barObject->getNumSections() > 0)
                {
                    if (true == this->barCouldDraw)
                    {
                        this->barObject->beginUpdate(0);
                    }
                }
                else
                {
                    this->barObject->clear();
                    this->barObject->begin("WhiteNoLightingBackground", Ogre::OT_TRIANGLE_LIST);
                }

                unsigned long idx = 0;

                // Outer background rectangle (two quads front-face)
                // Upper face
                this->barObject->position(p + (o * (so * (sp + Ogre::Vector3(-w - bs, h + bs, 0.0f)))));
                this->barObject->colour(outerCol);
                this->barObject->normal(0, 0, 1);
                this->barObject->textureCoord(0, 0);
                this->barObject->position(p + (o * (so * (sp + Ogre::Vector3(-w - bs, h + bs, 0.0f)))));
                this->barObject->colour(outerCol);
                this->barObject->normal(0, 0, 1);
                this->barObject->textureCoord(0, 1);
                this->barObject->position(p + (o * (so * (sp + Ogre::Vector3(w + bs, h + bs, 0.0f)))));
                this->barObject->colour(outerCol);
                this->barObject->normal(0, 0, 1);
                this->barObject->textureCoord(1, 1);
                this->barObject->position(p + (o * (so * (sp + Ogre::Vector3(w + bs, h + bs, 0.0f)))));
                this->barObject->colour(outerCol);
                this->barObject->normal(0, 0, 1);
                this->barObject->textureCoord(1, 0);
                this->barObject->quad(idx + 0, idx + 1, idx + 2, idx + 3);
                idx += 4;
                // Lower face
                this->barObject->position(p + (o * (so * (sp + Ogre::Vector3(-w - bs, h + bs, 0.0f)))));
                this->barObject->colour(outerCol);
                this->barObject->normal(0, 0, 1);
                this->barObject->textureCoord(0, 0);
                this->barObject->position(p + (o * (so * (sp + Ogre::Vector3(-w - bs, -bs, 0.0f)))));
                this->barObject->colour(outerCol);
                this->barObject->normal(0, 0, 1);
                this->barObject->textureCoord(0, 1);
                this->barObject->position(p + (o * (so * (sp + Ogre::Vector3(w + bs, -bs, 0.0f)))));
                this->barObject->colour(outerCol);
                this->barObject->normal(0, 0, 1);
                this->barObject->textureCoord(1, 1);
                this->barObject->position(p + (o * (so * (sp + Ogre::Vector3(w + bs, h + bs, 0.0f)))));
                this->barObject->colour(outerCol);
                this->barObject->normal(0, 0, 1);
                this->barObject->textureCoord(1, 0);
                this->barObject->quad(idx + 0, idx + 1, idx + 2, idx + 3);
                idx += 4;

                // Fill rectangle (green, width = progress)
                if (capturedProgress > 0.001f)
                {
                    this->barObject->position(p + (o * (so * (sp + Ogre::Vector3(-w, h, 0.0f)))));
                    this->barObject->colour(fillTopCol);
                    this->barObject->normal(0, 0, 1);
                    this->barObject->textureCoord(0, 0);
                    this->barObject->position(p + (o * (so * (sp + Ogre::Vector3(-w, h, 0.0f)))));
                    this->barObject->colour(fillTopCol);
                    this->barObject->normal(0, 0, 1);
                    this->barObject->textureCoord(0, 1);
                    this->barObject->position(p + (o * (so * (sp + Ogre::Vector3(fillRight, h, 0.0f)))));
                    this->barObject->colour(fillTopCol);
                    this->barObject->normal(0, 0, 1);
                    this->barObject->textureCoord(1, 1);
                    this->barObject->position(p + (o * (so * (sp + Ogre::Vector3(fillRight, h, 0.0f)))));
                    this->barObject->colour(fillTopCol);
                    this->barObject->normal(0, 0, 1);
                    this->barObject->textureCoord(1, 0);
                    this->barObject->quad(idx + 0, idx + 1, idx + 2, idx + 3);
                    idx += 4;

                    this->barObject->position(p + (o * (so * (sp + Ogre::Vector3(-w, h, 0.0f)))));
                    this->barObject->colour(fillTopCol);
                    this->barObject->normal(0, 0, 1);
                    this->barObject->textureCoord(0, 0);
                    this->barObject->position(p + (o * (so * (sp + Ogre::Vector3(-w, 0.f, 0.0f)))));
                    this->barObject->colour(fillBotCol);
                    this->barObject->normal(0, 0, 1);
                    this->barObject->textureCoord(0, 1);
                    this->barObject->position(p + (o * (so * (sp + Ogre::Vector3(fillRight, 0.f, 0.0f)))));
                    this->barObject->colour(fillBotCol);
                    this->barObject->normal(0, 0, 1);
                    this->barObject->textureCoord(1, 1);
                    this->barObject->position(p + (o * (so * (sp + Ogre::Vector3(fillRight, h, 0.0f)))));
                    this->barObject->colour(fillTopCol);
                    this->barObject->normal(0, 0, 1);
                    this->barObject->textureCoord(1, 0);
                    this->barObject->quad(idx + 0, idx + 1, idx + 2, idx + 3);
                    idx += 4;
                }

                this->barCouldDraw = true;

                if (true == this->barCouldDraw)
                {
                    this->barObject->index(0);
                    this->barObject->end();
                }
                else
                {
                    this->barObject->clear();
                }

                this->barLineNode->setVisible(capturedProgress < 1.0f);
            }

            // ── c) Percentage text ────────────────────────────────────────────────
            //
            // textNode is a child of the game-object's scene node.
            // addTrackedNode() (called in createOverlays) handles camera-facing.
            // We just update the caption and position here.

            if (nullptr != this->percentageText && doText)
            {
                this->percentageText->setCaption(pctStr);
                this->percentageText->forceUpdate();

                const Ogre::Real barGap = 0.35f;
                const Ogre::Real textOffset = 0.15f; // above bar
                const Ogre::Real textWorldY = worldPos.y + capturedMeshMaxY * scale.y + barGap + textOffset;

                // Note: use updateNodePosition / updateNodeOrientation through the
                // GraphicsModule to be safe on the render thread (same as GameObjectTitleComponent).
                NOWA::GraphicsModule::getInstance()->updateNodeOrientation(this->textNode, so);
                NOWA::GraphicsModule::getInstance()->updateNodePosition(this->textNode, Ogre::Vector3(worldPos.x, textWorldY, worldPos.z));

                this->textNode->setVisible(capturedProgress < 1.0f);
            }
            else if (nullptr != this->textNode)
            {
                this->textNode->setVisible(false);
            }

            // ── d) Glow ring at construction front ────────────────────────────────
            //
            // Flat horizontal annulus in the world XZ plane at world Y = cutoffWorldY.
            // Radius accounts for game object scale.
            // Uses same material and couldDraw pattern.

            if (nullptr != this->glowObject && capturedProgress < 1.0f)
            {
                this->glowCouldDraw = false;

                // World Y of the construction front.
                const Ogre::Real cutoffWorldY = worldPos.y + capturedCutoffY * scale.y;
                const Ogre::Vector3 ringCenter(worldPos.x, cutoffWorldY, worldPos.z);

                // Scale ring radius by the larger of x/z scale factors.
                const Ogre::Real scaleFactor = std::max(scale.x, scale.z);
                const Ogre::Real outerR = capturedXZRadius * scaleFactor * 1.15f;
                const Ogre::Real innerR = capturedXZRadius * scaleFactor * 0.85f;
                const int segs = 24;

                const Ogre::ColourValue glowOuter(1.0f, 0.55f, 0.0f, 1.0f);
                const Ogre::ColourValue glowInner(1.0f, 0.95f, 0.2f, 1.0f);

                if (this->glowObject->getNumSections() > 0)
                {
                    if (true == this->glowCouldDraw)
                    {
                        this->glowObject->beginUpdate(0);
                    }
                }
                else
                {
                    this->glowObject->clear();
                    this->glowObject->begin("WhiteNoLightingBackground", Ogre::OT_TRIANGLE_LIST);
                }

                unsigned long gIdx = 0;
                for (int i = 0; i < segs; ++i)
                {
                    const float a0 = static_cast<float>(i) / segs * Ogre::Math::TWO_PI;
                    const float a1 = static_cast<float>(i + 1) / segs * Ogre::Math::TWO_PI;

                    const Ogre::Vector3 o0 = ringCenter + Ogre::Vector3(outerR * Ogre::Math::Cos(a0), 0.0f, outerR * Ogre::Math::Sin(a0));
                    const Ogre::Vector3 o1 = ringCenter + Ogre::Vector3(outerR * Ogre::Math::Cos(a1), 0.0f, outerR * Ogre::Math::Sin(a1));
                    const Ogre::Vector3 i0 = ringCenter + Ogre::Vector3(innerR * Ogre::Math::Cos(a0), 0.0f, innerR * Ogre::Math::Sin(a0));
                    const Ogre::Vector3 i1 = ringCenter + Ogre::Vector3(innerR * Ogre::Math::Cos(a1), 0.0f, innerR * Ogre::Math::Sin(a1));

                    this->glowObject->position(o0);
                    this->glowObject->colour(glowOuter);
                    this->glowObject->normal(0, 1, 0);
                    this->glowObject->textureCoord(0, 0);
                    this->glowObject->position(o1);
                    this->glowObject->colour(glowOuter);
                    this->glowObject->normal(0, 1, 0);
                    this->glowObject->textureCoord(1, 0);
                    this->glowObject->position(i1);
                    this->glowObject->colour(glowInner);
                    this->glowObject->normal(0, 1, 0);
                    this->glowObject->textureCoord(1, 1);
                    this->glowObject->position(i0);
                    this->glowObject->colour(glowInner);
                    this->glowObject->normal(0, 1, 0);
                    this->glowObject->textureCoord(0, 1);
                    this->glowObject->quad(gIdx + 0, gIdx + 1, gIdx + 2, gIdx + 3);
                    gIdx += 4;
                }

                this->glowCouldDraw = true;

                if (true == this->glowCouldDraw)
                {
                    this->glowObject->index(0);
                    this->glowObject->end();
                }
                else
                {
                    this->glowObject->clear();
                }

                this->glowLineNode->setVisible(true);
            }
            else if (nullptr != this->glowLineNode)
            {
                this->glowLineNode->setVisible(false);
            }
        };

        NOWA::GraphicsModule::getInstance()->updateTrackedClosure(this->trackedClosureId(), renderClosure, false);

        // ── 5. Completion ─────────────────────────────────────────────────────────
        if (this->constructionProgress >= 1.0f && false == this->isConstructionFinished)
        {
            this->isConstructionFinished = true;
            this->setActivated(false);

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MeshConstructionComponent] Finished: " + this->gameObjectPtr->getName());

            if (this->closureFunction.is_valid())
            {
                NOWA::AppStateManager::LogicCommand logicCmd = [this]()
                {
                    try
                    {
                        luabind::call_function<void>(this->closureFunction);
                    }
                    catch (luabind::error& error)
                    {
                        luabind::object errMsg(luabind::from_stack(error.state(), -1));
                        std::stringstream ss;
                        ss << errMsg;
                        Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[MeshConstructionComponent] Error in reactOnConstructionDone: " + Ogre::String(error.what()) + " details: " + ss.str());
                    }
                };
                NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCmd));
            }
        }
    }

    // =========================================================================
    //  actualizeValue / writeXML
    // =========================================================================

    void MeshConstructionComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);
        if (AttrActivated() == attribute->getName())
        {
            this->setActivated(attribute->getBool());
        }
        else if (AttrConstructionTime() == attribute->getName())
        {
            this->setConstructionTime(attribute->getReal());
        }
        else if (AttrShowProgressBar() == attribute->getName())
        {
            this->setShowProgressBar(attribute->getBool());
        }
        else if (AttrShowPercentageText() == attribute->getName())
        {
            this->setShowPercentageText(attribute->getBool());
        }
    }

    void MeshConstructionComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
    {
        GameObjectComponent::writeXML(propertiesXML, doc);

        auto add = [&](const char* type, const Ogre::String& nm, const Ogre::String& val)
        {
            xml_node<>* n = doc.allocate_node(node_element, "property");
            n->append_attribute(doc.allocate_attribute("type", type));
            n->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, nm)));
            n->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, val)));
            propertiesXML->append_node(n);
        };

        add("12", AttrActivated(), XMLConverter::ConvertString(doc, this->activated->getBool()));
        add("6", AttrConstructionTime(), XMLConverter::ConvertString(doc, this->constructionTime->getReal()));
        add("12", AttrShowProgressBar(), XMLConverter::ConvertString(doc, this->showProgressBar->getBool()));
        add("12", AttrShowPercentageText(), XMLConverter::ConvertString(doc, this->showPercentageText->getBool()));
    }

    // =========================================================================
    //  extractMeshDataAndCreateBuffers  —  render thread
    // =========================================================================

    bool MeshConstructionComponent::extractMeshDataAndCreateBuffers(void)
    {
        Ogre::Item* srcItem = this->gameObjectPtr->getMovableObject<Ogre::Item>();
        if (nullptr == srcItem)
        {
            return false;
        }

        Ogre::VaoManager* vm = Ogre::Root::getSingletonPtr()->getRenderSystem()->getVaoManager();
        Ogre::MeshPtr mesh = srcItem->getMesh();
        const Ogre::uint16 numSM = mesh->getNumSubMeshes();

        this->vertices.clear();
        this->normals.clear();
        this->tangents.clear();
        this->uvCoordinates.clear();
        this->indices.clear();
        this->subMeshInfoList.clear();
        this->vertexCount = 0;
        this->indexCount = 0;
        this->meshXZRadius = 0.0f;

        // Pass 1: count
        size_t totalV = 0, totalI = 0;
        for (Ogre::uint16 s = 0; s < numSM; ++s)
        {
            Ogre::SubMesh* sm = mesh->getSubMesh(s);
            if (sm->mVao[Ogre::VpNormal].empty())
            {
                continue;
            }
            Ogre::VertexArrayObject* vao = sm->mVao[Ogre::VpNormal][0];
            if (vao->getVertexBuffers().empty())
            {
                continue;
            }
            totalV += vao->getVertexBuffers()[0]->getNumElements();
            totalI += vao->getIndexBuffer() ? vao->getIndexBuffer()->getNumElements() : 0u;
        }
        if (0 == totalV)
        {
            return false;
        }

        this->vertices.resize(totalV);
        this->normals.resize(totalV, Ogre::Vector3::UNIT_Y);
        this->tangents.resize(totalV, Ogre::Vector4(1, 0, 0, 1));
        this->uvCoordinates.resize(totalV, Ogre::Vector2::ZERO);
        this->indices.reserve(totalI);

        size_t gvOff = 0, giOff = 0;

        // Pass 2: read
        for (Ogre::uint16 s = 0; s < numSM; ++s)
        {
            Ogre::SubMesh* sm = mesh->getSubMesh(s);
            if (sm->mVao[Ogre::VpNormal].empty())
            {
                continue;
            }
            Ogre::VertexArrayObject* vao = sm->mVao[Ogre::VpNormal][0];
            if (vao->getVertexBuffers().empty())
            {
                continue;
            }

            const size_t smVC = vao->getVertexBuffers()[0]->getNumElements();
            const size_t smIC = vao->getIndexBuffer() ? vao->getIndexBuffer()->getNumElements() : 0u;
            bool hasTan = false, hasQTan = false;

            for (Ogre::VertexBufferPacked* vbp : vao->getVertexBuffers())
            {
                const Ogre::VertexElement2Vec& elems = vbp->getVertexElements();
                for (const Ogre::VertexElement2& e : elems)
                {
                    if (e.mSemantic == Ogre::VES_TANGENT)
                    {
                        hasTan = true;
                    }
                    if (e.mSemantic == Ogre::VES_NORMAL && e.mType == Ogre::VET_SHORT4_SNORM)
                    {
                        hasQTan = true;
                    }
                }

                size_t stride = 0;
                for (const Ogre::VertexElement2& e : elems)
                {
                    stride += Ogre::v1::VertexElement::getTypeSize(e.mType);
                }

                Ogre::AsyncTicketPtr ticket = vbp->readRequest(0, vbp->getNumElements());
                const unsigned char* raw = reinterpret_cast<const unsigned char*>(ticket->map());

                for (size_t vi = 0; vi < smVC; ++vi)
                {
                    const unsigned char* vptr = raw + vi * stride;
                    size_t bo = 0;
                    const size_t gi = gvOff + vi;

                    for (const Ogre::VertexElement2& e : elems)
                    {
                        const unsigned char* ep = vptr + bo;
                        switch (e.mSemantic)
                        {
                        case Ogre::VES_POSITION:
                        {
                            const float* f = reinterpret_cast<const float*>(ep);
                            this->vertices[gi] = Ogre::Vector3(f[0], f[1], f[2]);
                            break;
                        }
                        case Ogre::VES_NORMAL:
                            if (e.mType == Ogre::VET_FLOAT3)
                            {
                                const float* f = reinterpret_cast<const float*>(ep);
                                this->normals[gi] = Ogre::Vector3(f[0], f[1], f[2]);
                            }
                            else if (e.mType == Ogre::VET_SHORT4_SNORM)
                            {
                                const short* sh = reinterpret_cast<const short*>(ep);
                                const float sc = 1.0f / 32767.0f;
                                Ogre::Quaternion q(sh[3] * sc, sh[0] * sc, sh[1] * sc, sh[2] * sc);
                                q.normalise();
                                this->normals[gi] = q * Ogre::Vector3::UNIT_Z;
                            }
                            break;
                        case Ogre::VES_TANGENT:
                            if (e.mType == Ogre::VET_FLOAT4)
                            {
                                const float* f = reinterpret_cast<const float*>(ep);
                                this->tangents[gi] = Ogre::Vector4(f[0], f[1], f[2], f[3]);
                            }
                            break;
                        case Ogre::VES_TEXTURE_COORDINATES:
                            if (e.mType == Ogre::VET_FLOAT2)
                            {
                                const float* f = reinterpret_cast<const float*>(ep);
                                this->uvCoordinates[gi] = Ogre::Vector2(f[0], f[1]);
                            }
                            else if (e.mType == Ogre::VET_HALF2)
                            {
                                const Ogre::uint16* h = reinterpret_cast<const Ogre::uint16*>(ep);
                                this->uvCoordinates[gi] = Ogre::Vector2(Ogre::Bitwise::halfToFloat(h[0]), Ogre::Bitwise::halfToFloat(h[1]));
                            }
                            break;
                        default:
                            break;
                        }
                        bo += Ogre::v1::VertexElement::getTypeSize(e.mType);
                    }
                }
                ticket->unmap();
            }

            // Index buffer
            if (nullptr != vao->getIndexBuffer())
            {
                Ogre::IndexBufferPacked* ibp = vao->getIndexBuffer();
                Ogre::AsyncTicketPtr idxT = ibp->readRequest(0, ibp->getNumElements());
                const void* idxRaw = idxT->map();
                const bool is32 = (ibp->getIndexType() == Ogre::IndexBufferPacked::IT_32BIT);
                for (size_t ii = 0; ii < smIC; ++ii)
                {
                    Ogre::uint32 idx = is32 ? reinterpret_cast<const Ogre::uint32*>(idxRaw)[ii] : static_cast<Ogre::uint32>(reinterpret_cast<const Ogre::uint16*>(idxRaw)[ii]);
                    this->indices.push_back(static_cast<Ogre::uint32>(gvOff + idx));
                }
                idxT->unmap();
            }

            SubMeshInfo si;
            si.vertexOffset = gvOff;
            si.vertexCount = smVC;
            si.indexOffset = giOff;
            si.indexCount = smIC;
            si.hasTangent = hasTan || hasQTan;
            si.floatsPerVertex = si.hasTangent ? 12u : 8u;
            this->subMeshInfoList.push_back(si);

            gvOff += smVC;
            giOff += smIC;
        }

        this->vertexCount = this->vertices.size();
        this->indexCount = this->indices.size();

        // ── Recompute Y bounds + XZ radius from ACTUAL vertex data ────────────────
        // This is critical — getBounds() in postInit can differ slightly, causing
        // the top triangles to never become visible at 100%.
        {
            float minY = std::numeric_limits<float>::max();
            float maxY = -std::numeric_limits<float>::max();
            for (const auto& v : this->vertices)
            {
                if (v.y < minY)
                {
                    minY = v.y;
                }
                if (v.y > maxY)
                {
                    maxY = v.y;
                }
                const float r = Ogre::Vector2(v.x, v.z).length();
                if (r > this->meshXZRadius)
                {
                    this->meshXZRadius = r;
                }
            }
            this->meshMinY = minY;
            this->meshMaxY = maxY;
            if (this->meshXZRadius < 0.01f)
            {
                this->meshXZRadius = 0.5f;
            }
        }

        // ── Sort triangles by centroid Y per sub-mesh (one-time O(n log n)) ────────
        for (SubMeshInfo& si : this->subMeshInfoList)
        {
            const size_t triCount = si.indexCount / 3;
            si.sortedTriIndices.resize(triCount);
            si.sortedCentroidY.resize(triCount);

            for (size_t t = 0; t < triCount; ++t)
            {
                const size_t i0 = this->indices[si.indexOffset + t * 3];
                const size_t i1 = this->indices[si.indexOffset + t * 3 + 1];
                const size_t i2 = this->indices[si.indexOffset + t * 3 + 2];
                si.sortedCentroidY[t] = (this->vertices[i0].y + this->vertices[i1].y + this->vertices[i2].y) / 3.0f;
                si.sortedTriIndices[t] = t;
            }

            std::sort(si.sortedTriIndices.begin(), si.sortedTriIndices.end(),
                [&si](size_t a, size_t b)
                {
                    return si.sortedCentroidY[a] < si.sortedCentroidY[b];
                });

            std::vector<float> sortedY(triCount);
            for (size_t i = 0; i < triCount; ++i)
            {
                sortedY[i] = si.sortedCentroidY[si.sortedTriIndices[i]];
            }
            si.sortedCentroidY = std::move(sortedY);
        }

        // ── Build GPU mesh ────────────────────────────────────────────────────────
        const Ogre::String meshName = this->gameObjectPtr->getName() + "_Construction_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());

        {
            Ogre::ResourcePtr ex = Ogre::MeshManager::getSingleton().getByName(meshName, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
            if (false == ex.isNull())
            {
                Ogre::MeshManager::getSingleton().remove(ex->getHandle());
            }
        }

        this->constructionMesh = Ogre::MeshManager::getSingleton().createManual(meshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
        this->constructionMesh->_setVaoManager(vm);

        Ogre::Vector3 minBB(std::numeric_limits<float>::max());
        Ogre::Vector3 maxBB(-std::numeric_limits<float>::max());

        for (size_t smIdx = 0; smIdx < this->subMeshInfoList.size(); ++smIdx)
        {
            SubMeshInfo& si = this->subMeshInfoList[smIdx];
            const size_t fpv = si.floatsPerVertex;
            const size_t vS = si.vertexOffset;
            const size_t vC = si.vertexCount;

            // Vertex buffer — original geometry, never modified.
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
                if (si.hasTangent)
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

            Ogre::VertexElement2Vec elems;
            elems.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_POSITION));
            elems.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_NORMAL));
            if (si.hasTangent)
            {
                elems.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT4, Ogre::VES_TANGENT));
            }
            elems.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES));

            si.vertexBuffer = vm->createVertexBuffer(elems, vC, Ogre::BT_DYNAMIC_DEFAULT, vd, false);

            // Index buffer — starts all-degenerate; update() fills visible tris.
            const size_t iC = si.indexCount;
            const bool use16 = (vC <= 65535u);
            if (use16)
            {
                Ogre::uint16* id = reinterpret_cast<Ogre::uint16*>(OGRE_MALLOC_SIMD(iC * sizeof(Ogre::uint16), Ogre::MEMCATEGORY_GEOMETRY));
                std::memset(id, 0, iC * sizeof(Ogre::uint16));
                si.indexBuffer = vm->createIndexBuffer(Ogre::IndexBufferPacked::IT_16BIT, iC, Ogre::BT_DYNAMIC_DEFAULT, id, false);
            }
            else
            {
                Ogre::uint32* id = reinterpret_cast<Ogre::uint32*>(OGRE_MALLOC_SIMD(iC * sizeof(Ogre::uint32), Ogre::MEMCATEGORY_GEOMETRY));
                std::memset(id, 0, iC * sizeof(Ogre::uint32));
                si.indexBuffer = vm->createIndexBuffer(Ogre::IndexBufferPacked::IT_32BIT, iC, Ogre::BT_DYNAMIC_DEFAULT, id, false);
            }

            Ogre::VertexBufferPackedVec vbv;
            vbv.push_back(si.vertexBuffer);
            Ogre::VertexArrayObject* vao = vm->createVertexArrayObject(vbv, si.indexBuffer, Ogre::OT_TRIANGLE_LIST);

            Ogre::SubMesh* subMesh = this->constructionMesh->createSubMesh();
            subMesh->mVao[Ogre::VpNormal].push_back(vao);
            subMesh->mVao[Ogre::VpShadow].push_back(vao);

            if (smIdx < srcItem->getNumSubItems())
            {
                Ogre::HlmsDatablock* db = srcItem->getSubItem(smIdx)->getDatablock();
                if (nullptr != db && nullptr != db->getNameStr())
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
        this->constructionMesh->_setBounds(bounds, false);
        this->constructionMesh->_setBoundingSphereRadius(bounds.getRadius());

        this->constructionItem = this->gameObjectPtr->getSceneManager()->createItem(this->constructionMesh, this->gameObjectPtr->isDynamic() ? Ogre::SCENE_DYNAMIC : Ogre::SCENE_STATIC);
        this->constructionItem->setName(this->gameObjectPtr->getName() + "_Construction");

        for (size_t i = 0; i < this->subMeshInfoList.size(); ++i)
        {
            if (i < srcItem->getNumSubItems() && i < this->constructionItem->getNumSubItems())
            {
                Ogre::HlmsDatablock* db = srcItem->getSubItem(i)->getDatablock();
                if (nullptr != db)
                {
                    this->constructionItem->getSubItem(i)->setDatablock(db);
                }
            }
        }

        return true;
    }

    // =========================================================================
    //  destroyConstructionResources  —  render thread
    // =========================================================================

    void MeshConstructionComponent::destroyConstructionResources(void)
    {
        for (SubMeshInfo& sm : this->subMeshInfoList)
        {
            if (sm.vertexBuffer && sm.vertexBuffer->getMappingState() != Ogre::MS_UNMAPPED)
            {
                sm.vertexBuffer->unmap(Ogre::UO_UNMAP_ALL);
            }
            if (sm.indexBuffer && sm.indexBuffer->getMappingState() != Ogre::MS_UNMAPPED)
            {
                sm.indexBuffer->unmap(Ogre::UO_UNMAP_ALL);
            }
            sm.vertexBuffer = nullptr;
            sm.indexBuffer = nullptr;
        }
        this->constructionItem = nullptr; // already swapped out
        if (false == this->constructionMesh.isNull())
        {
            Ogre::MeshManager::getSingleton().remove(this->constructionMesh);
            this->constructionMesh.reset();
        }
    }

    // =========================================================================
    //  createOverlays  —  render thread
    //
    //  Progress bar + glow ring: child of root scene node (no transform).
    //  Vertices are supplied in world space inside the tracked closure.
    //
    //  Percentage text: child of game-object's scene node.
    //  addTrackedNode() → render thread auto-orients it toward camera.
    //  (Same as GameObjectTitleComponent::postInit)
    // =========================================================================

    void MeshConstructionComponent::createOverlays(void)
    {
        Ogre::SceneManager* sm = this->gameObjectPtr->getSceneManager();
        Ogre::SceneNode* root = sm->getRootSceneNode();
        const Ogre::SceneMemoryMgrTypes memType = this->gameObjectPtr->isDynamic() ? Ogre::SCENE_DYNAMIC : Ogre::SCENE_STATIC;

        // ── Progress bar ──────────────────────────────────────────────────────────
        if (this->showProgressBar->getBool())
        {
            this->barLineNode = root->createChildSceneNode(memType);

            this->barObject = sm->createManualObject();
            this->barObject->setName("MeshConstrBar_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()) + "_" + Ogre::StringConverter::toString(this->index));
            this->barObject->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_MESH);
            this->barObject->setQueryFlags(0 << 0);
            this->barObject->setCastShadows(false);
            this->barLineNode->attachObject(this->barObject);
            this->barCouldDraw = false;
        }

        // ── Glow ring ─────────────────────────────────────────────────────────────
        this->glowLineNode = root->createChildSceneNode(memType);
        this->glowObject = sm->createManualObject();
        this->glowObject->setName("MeshConstrGlow_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()) + "_" + Ogre::StringConverter::toString(this->index));
        this->glowObject->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_MESH);
        this->glowObject->setQueryFlags(0 << 0);
        this->glowObject->setCastShadows(false);
        this->glowLineNode->attachObject(this->glowObject);
        this->glowCouldDraw = false;

        // ── Percentage text ───────────────────────────────────────────────────────
        // Follows GameObjectTitleComponent::postInit exactly.
        if (this->showPercentageText->getBool())
        {
            Ogre::NameValuePairList params;
            params["name"] = Ogre::StringConverter::toString(this->gameObjectPtr->getId()) + "_ConstrText";
            params["Caption"] = "0 %";
            params["fontName"] = "BlueHighway";

            this->percentageText = new MovableText(Ogre::Id::generateNewId<Ogre::MovableObject>(), &sm->_getEntityMemoryManager(Ogre::SCENE_DYNAMIC), sm, &params);

            this->percentageText->setCaption("0 %");
            this->percentageText->setCharacterHeight(0.25f);
            this->percentageText->showOnTop(true);
            this->percentageText->setColor(Ogre::ColourValue(1.0f, 1.0f, 0.2f, 1.0f));
            this->percentageText->setTextAlignment(MovableText::H_CENTER, MovableText::V_CENTER);
            this->percentageText->setQueryFlags(0 << 0);

            // Child of game-object scene node — inherits world position.
            this->textNode = this->gameObjectPtr->getSceneNode()->createChildSceneNode(memType);
            this->textNode->attachObject(this->percentageText);

            // Registers node for automatic camera-facing on render thread —
            // same call as GameObjectTitleComponent::postInit.
            NOWA::GraphicsModule::getInstance()->addTrackedNode(this->textNode);
            this->textNode->setVisible(false);
        }
    }

    // =========================================================================
    //  destroyOverlays  —  render thread
    // =========================================================================

    void MeshConstructionComponent::destroyOverlays(void)
    {
        Ogre::SceneManager* sm = this->gameObjectPtr->getSceneManager();

        if (nullptr != this->barObject)
        {
            this->barLineNode->detachAllObjects();
            sm->destroyManualObject(this->barObject);
            this->barObject = nullptr;
        }
        if (nullptr != this->barLineNode)
        {
            this->barLineNode->getParentSceneNode()->removeAndDestroyChild(this->barLineNode);
            this->barLineNode = nullptr;
        }

        if (nullptr != this->glowObject)
        {
            this->glowLineNode->detachAllObjects();
            sm->destroyManualObject(this->glowObject);
            this->glowObject = nullptr;
        }
        if (nullptr != this->glowLineNode)
        {
            this->glowLineNode->getParentSceneNode()->removeAndDestroyChild(this->glowLineNode);
            this->glowLineNode = nullptr;
        }

        if (nullptr != this->percentageText)
        {
            NOWA::GraphicsModule::getInstance()->removeTrackedNode(this->textNode);
            this->textNode->detachAllObjects();
            delete this->percentageText;
            this->percentageText = nullptr;
        }
        if (nullptr != this->textNode)
        {
            this->gameObjectPtr->getSceneNode()->removeAndDestroyChild(this->textNode);
            this->textNode = nullptr;
        }

        this->barCouldDraw = false;
        this->glowCouldDraw = false;
    }

    // =========================================================================
    //  canStaticAddComponent
    // =========================================================================

    bool MeshConstructionComponent::canStaticAddComponent(GameObject* go)
    {
        return go->getMovableObjectUnsafe<Ogre::Item>() != nullptr && go->getComponentCount<MeshConstructionComponent>() == 0;
    }

    // =========================================================================
    //  Lua API
    // =========================================================================

    MeshConstructionComponent* getMeshConstructionComponent(GameObject* go)
    {
        return NOWA::makeStrongPtr(go->getComponent<MeshConstructionComponent>()).get();
    }

    MeshConstructionComponent* getMeshConstructionComponentFromName(GameObject* go, const Ogre::String& name)
    {
        return NOWA::makeStrongPtr(go->getComponentFromName<MeshConstructionComponent>(name)).get();
    }

    void MeshConstructionComponent::createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass)
    {
        luabind::module(lua)[luabind::class_<MeshConstructionComponent, GameObjectComponent>("MeshConstructionComponent")
                .def("setActivated", &MeshConstructionComponent::setActivated)
                .def("isActivated", &MeshConstructionComponent::isActivated)
                .def("setConstructionTime", &MeshConstructionComponent::setConstructionTime)
                .def("getConstructionTime", &MeshConstructionComponent::getConstructionTime)
                .def("setShowProgressBar", &MeshConstructionComponent::setShowProgressBar)
                .def("getShowProgressBar", &MeshConstructionComponent::getShowProgressBar)
                .def("setShowPercentageText", &MeshConstructionComponent::setShowPercentageText)
                .def("getShowPercentageText", &MeshConstructionComponent::getShowPercentageText)
                .def("getConstructionProgress", &MeshConstructionComponent::getConstructionProgress)
                .def("getIsConstructionFinished", &MeshConstructionComponent::getIsConstructionFinished)
                .def("reactOnConstructionDone", &MeshConstructionComponent::reactOnConstructionDone)];

        LuaScriptApi::getInstance()->addClassToCollection("MeshConstructionComponent", "class inherits GameObjectComponent", MeshConstructionComponent::getStaticInfoText());
        LuaScriptApi::getInstance()->addClassToCollection("MeshConstructionComponent", "void reactOnConstructionDone(func closureFunction)", "Lua callback fired when construction completes. Signature: function()");
        LuaScriptApi::getInstance()->addClassToCollection("MeshConstructionComponent", "float getConstructionProgress()", "Returns current progress [0..1].");

        gameObjectClass.def("getMeshConstructionComponent", (MeshConstructionComponent * (*)(GameObject*)) & getMeshConstructionComponent);
        gameObjectClass.def("getMeshConstructionComponentFromName", &getMeshConstructionComponentFromName);

        gameObjectControllerClass.def("castMeshConstructionComponent", &GameObjectController::cast<MeshConstructionComponent>);

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "MeshConstructionComponent getMeshConstructionComponent()", "Gets the MeshConstructionComponent.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "MeshConstructionComponent castMeshConstructionComponent(MeshConstructionComponent other)", "Casts for Lua auto-completion.");
    }

} // namespace NOWA
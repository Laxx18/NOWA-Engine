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
        meshAxisMin(0.0f),
        meshAxisMax(1.0f),
        vertexCount(0),
        indexCount(0),
        constructionItem(nullptr),
        barLineNode(nullptr),
        barObject(nullptr),
        barCouldDraw(false),
        textLineNode(nullptr),
        percentageText(nullptr),
        elapsed(0.0f),
        constructionProgress(0.0f),
        isConstructionFinished(false),

        activated(new Variant(MeshConstructionComponent::AttrActivated(), true, this->attributes)),
        constructionTime(new Variant(MeshConstructionComponent::AttrConstructionTime(), 5.0f, this->attributes)),
        showProgressBar(new Variant(MeshConstructionComponent::AttrShowProgressBar(), true, this->attributes)),
        showPercentageText(new Variant(MeshConstructionComponent::AttrShowPercentageText(), true, this->attributes)),
        invert(new Variant(MeshConstructionComponent::AttrInvert(), false, this->attributes)),
        constructionAxis(new Variant(MeshConstructionComponent::AttrConstructionAxis(), std::vector<Ogre::String>{"X", "Y", "Z"}, this->attributes))
    {
        this->constructionTime->setConstraints(0.1f, 3600.0f);
        this->constructionTime->setDescription("Total time in seconds for the mesh to fully materialise.");
        this->showProgressBar->setDescription("Show a camera-facing progress bar above the object.");
        this->showPercentageText->setDescription("Show a percentage text label above the progress bar.");
        this->invert->setDescription("If true the mesh deconstructs top-to-bottom (demolition mode) "
                                     "instead of building bottom-to-top.");
        this->constructionAxis->setListSelectedValue("Y");
        this->constructionAxis->setDescription("Construction axis. X = left to right, Y = bottom to top, "
                                               "Z = front to back. Combine with Invert to reverse direction.");
    }

    MeshConstructionComponent::~MeshConstructionComponent()
    {
    }

    // =========================================================================
    //  Ogre::Plugin
    // =========================================================================

    void MeshConstructionComponent::install(const Ogre::NameValuePairList*)
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
    void MeshConstructionComponent::setInvert(bool v)
    {
        this->invert->setValue(v);
    }
    bool MeshConstructionComponent::getInvert(void) const
    {
        return this->invert->getBool();
    }
    void MeshConstructionComponent::setConstructionAxis(const Ogre::String& axis)
    {
        this->constructionAxis->setListSelectedValue(axis);
    }
    Ogre::String MeshConstructionComponent::getConstructionAxis(void) const
    {
        return this->constructionAxis->getListSelectedValue();
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
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrInvert())
        {
            this->invert->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrConstructionAxis())
        {
            this->constructionAxis->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", "Y"));
            propertyElement = propertyElement->next_sibling("property");
        }
        return true;
    }

    // =========================================================================
    //  postInit  — editor-safe, zero resource creation
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
        const Ogre::Aabb& b = item->getMesh()->getAabb();
        this->meshMinY = b.getMinimum().y;
        this->meshMaxY = b.getMaximum().y;
        if (this->meshMinY >= this->meshMaxY)
        {
            this->meshMinY = 0.0f;
            this->meshMaxY = 1.0f;
        }
        this->meshAxisMin = this->meshMinY;
        this->meshAxisMax = this->meshMaxY;
        return true;
    }

    // =========================================================================
    //  restoreOriginalItemAndCleanup  — always called via enqueueAndWait
    //  so the caller blocks until the render thread has finished cleaning up.
    //  Safe to call even if construction was never started.
    // =========================================================================

    void MeshConstructionComponent::restoreOriginalItemAndCleanup(void)
    {
        NOWA::GraphicsModule::RenderCommand cmd = [this]()
        {
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
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "MeshConstructionComponent::restoreOriginalItemAndCleanup");
    }

    // =========================================================================
    //  connect
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
        // barCouldDraw starts false (set in constructor, reset in destroyOverlays)

        // Inverted mode: seed GPU with all-real indices so the complete mesh
        // is visible immediately and deconstructs from there.
        if (true == this->invert->getBool())
        {
            this->uploadFinalIndicesBlocking(true);
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MeshConstructionComponent] Started for: " + this->gameObjectPtr->getName());
        return true;
    }

    // =========================================================================
    //  disconnect  — enqueueAndWait guarantees render cleanup before return
    // =========================================================================

    bool MeshConstructionComponent::disconnect(void)
    {
        GameObjectComponent::disconnect();

        // Stop render work FIRST, then block until resources are freed.
        NOWA::GraphicsModule::getInstance()->removeTrackedClosure(this->trackedClosureId());
        this->restoreOriginalItemAndCleanup();

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

        return true;
    }

    // =========================================================================
    //  onRemoveComponent  — must also block until render cleanup is done
    // =========================================================================

    void MeshConstructionComponent::onRemoveComponent(void)
    {
        GameObjectComponent::onRemoveComponent();

        // Stop render work, then block until resources are freed.
        // enqueueAndWait prevents the dangling-this crash.
        NOWA::GraphicsModule::getInstance()->removeTrackedClosure(this->trackedClosureId());
        this->restoreOriginalItemAndCleanup();
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
        comp->setInvert(this->invert->getBool());
        comp->setConstructionAxis(this->constructionAxis->getListSelectedValue());
        cloned->addComponent(comp);
        comp->setOwner(cloned);
        GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(comp));
        return comp;
    }

    // =========================================================================
    //  setActivated
    // =========================================================================

    void MeshConstructionComponent::setActivated(bool value)
    {
        this->activated->setValue(value);

        if (false == value && this->bConnected)
        {
            NOWA::GraphicsModule::getInstance()->removeTrackedClosure(this->trackedClosureId());

            NOWA::GraphicsModule::RenderCommand hide = [this]()
            {
                if (nullptr != this->barLineNode)
                {
                    this->barLineNode->setVisible(false);
                }
                if (nullptr != this->textLineNode)
                {
                    this->textLineNode->setVisible(false);
                }
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(hide), "MeshConstructionComponent::setActivated::hide");
        }
    }

    // =========================================================================
    //  axisComponent  —  returns the X, Y or Z component of v based on the
    //  constructionAxis Variant.  Used for centroid sort and cutoff comparison.
    // =========================================================================

    float MeshConstructionComponent::axisComponent(const Ogre::Vector3& v) const
    {
        const Ogre::String axis = this->constructionAxis->getListSelectedValue();
        if (axis == "X")
        {
            return v.x;
        }
        if (axis == "Z")
        {
            return v.z;
        }
        return v.y; // default "Y"
    }

    // =========================================================================
    //  uploadFinalIndicesBlocking
    //
    //  Synchronous final index upload called at 100% completion before the
    //  tracked closure is removed, guaranteeing the GPU sees the correct
    //  terminal state regardless of any closure race.
    //
    //  makeAllVisible == true  → normal mode:   complete mesh, all real indices
    //  makeAllVisible == false → inverted mode: empty mesh, all degenerate [0]
    // =========================================================================

    void MeshConstructionComponent::uploadFinalIndicesBlocking(bool makeAllVisible)
    {
        if (this->subMeshInfoList.empty())
        {
            return;
        }

        struct FullPacket
        {
            Ogre::IndexBufferPacked* buffer;
            std::vector<Ogre::uint16> data16;
            std::vector<Ogre::uint32> data32;
            size_t elementCount;
            bool is16;
        };

        std::vector<FullPacket> packets;
        packets.reserve(this->subMeshInfoList.size());

        for (const SubMeshInfo& sm : this->subMeshInfoList)
        {
            if (nullptr == sm.indexBuffer)
            {
                continue;
            }

            const bool use16 = (sm.vertexCount <= 65535u);
            const size_t iC = sm.indexCount;

            FullPacket pkt;
            pkt.buffer = sm.indexBuffer;
            pkt.elementCount = iC;
            pkt.is16 = use16;

            if (makeAllVisible)
            {
                // Real indices — complete mesh visible.
                if (use16)
                {
                    pkt.data16.resize(iC);
                    for (size_t i = 0; i < iC; ++i)
                    {
                        pkt.data16[i] = static_cast<Ogre::uint16>(this->indices[sm.indexOffset + i] - sm.vertexOffset);
                    }
                }
                else
                {
                    pkt.data32.resize(iC);
                    for (size_t i = 0; i < iC; ++i)
                    {
                        pkt.data32[i] = static_cast<Ogre::uint32>(this->indices[sm.indexOffset + i] - sm.vertexOffset);
                    }
                }
            }
            else
            {
                // All degenerate — mesh fully invisible (demolition complete).
                if (use16)
                {
                    pkt.data16.assign(iC, 0u);
                }
                else
                {
                    pkt.data32.assign(iC, 0u);
                }
            }

            packets.push_back(std::move(pkt));
        }

        NOWA::GraphicsModule::RenderCommand cmd = [packets = std::move(packets)]() mutable
        {
            for (FullPacket& pkt : packets)
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
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "MeshConstructionComponent::uploadFinalIndicesBlocking");
    }

    // =========================================================================
    //  update  —  logic thread
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

        // ── 1. Advance timer ──────────────────────────────────────────────────────
        this->elapsed += dt;
        this->constructionProgress = Ogre::Math::Clamp(this->elapsed / this->constructionTime->getReal(), 0.0f, 1.0f);

        // Local-space cutoff along the chosen axis, used with upper_bound(sortedCentroid).
        // Triangles with centroid <= cutoff are visible.
        //
        // Normal  (invert=false): cutoff rises  meshAxisMin → meshAxisMax
        // Inverted(invert=true ): cutoff falls  meshAxisMax → meshAxisMin
        //
        // Epsilon offsets ensure boundary triangles are always included.
        float cutoff;
        if (false == this->invert->getBool())
        {
            cutoff = (this->constructionProgress >= 1.0f) ? this->meshAxisMax + 1.0f : static_cast<float>(this->meshAxisMin + this->constructionProgress * (this->meshAxisMax - this->meshAxisMin));
        }
        else
        {
            cutoff = (this->constructionProgress <= 0.0f)   ? this->meshAxisMax + 1.0f
                     : (this->constructionProgress >= 1.0f) ? this->meshAxisMin - 1.0f
                                                            : static_cast<float>(this->meshAxisMax - this->constructionProgress * (this->meshAxisMax - this->meshAxisMin));
        }

        // ── 2. Index packets ──────────────────────────────────────────────────────
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

            const auto it = std::upper_bound(sm.sortedCentroid.begin(), sm.sortedCentroid.end(), cutoff);
            const size_t visibleTris = static_cast<size_t>(std::distance(sm.sortedCentroid.begin(), it));
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

        // ── 3. Capture for render closure ─────────────────────────────────────────
        const Ogre::Real capturedProgress = this->constructionProgress;
        const float capturedCutoff = cutoff;
        const bool doBar = this->showProgressBar->getBool();
        const bool doText = this->showPercentageText->getBool();
        const int pct = static_cast<int>(capturedProgress * 100.0f);
        const Ogre::String pctStr = Ogre::StringConverter::toString(pct) + " %";
        const float capturedMeshMaxY = this->meshMaxY;

        // ── 4. updateTrackedClosure ───────────────────────────────────────────────
        //
        //  CRITICAL: barCouldDraw is a member variable that
        //  PERSIST across frames — they are NEVER reset inside the closure.
        //  Frame 1: getNumSections()==0  → begin() is called → barCouldDraw=true
        //  Frame 2: getNumSections()>0 AND barCouldDraw==true → beginUpdate(0)
        //  This exactly mirrors ValueBarComponent::update() / drawValueBar().

        auto renderClosure = [this, packets = std::move(packets), capturedProgress, capturedCutoff, doBar, doText, pctStr, capturedMeshMaxY](Ogre::Real) mutable
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

            // ── Derive camera-facing orientation ──────────────────────────────────
            //
            // Follows ValueBarComponent::drawValueBar() convention EXACTLY:
            //   p  = world position of the bar/text anchor
            //   o  = Quaternion::IDENTITY  (strip object rotation)
            //   so = MathHelper::lookAt(objPos - cameraPos)  (camera-facing)
            //   sp = Vector3::ZERO
            //   vertex = p + (o * (so * (sp + localOffset)))

            Ogre::Camera* cam = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
            if (nullptr == cam)
            {
                return;
            }

            const Ogre::Vector3 objPos = this->gameObjectPtr->getSceneNode()->_getDerivedPosition();
            const Ogre::Vector3 scale = this->gameObjectPtr->getSceneNode()->_getDerivedScale();
            const Ogre::Vector3 dir = objPos - cam->getDerivedPosition();
            const Ogre::Quaternion so = MathHelper::getInstance()->lookAt(dir);
            const Ogre::Quaternion o = Ogre::Quaternion::IDENTITY;
            const Ogre::Vector3 sp = Ogre::Vector3::ZERO;

            // World-space top of the mesh (scaled).
            const Ogre::Real meshTopWorld = objPos.y + capturedMeshMaxY * scale.y;

            // ── b) Progress bar (follows ValueBarComponent::drawValueBar) ─────────

            if (nullptr != this->barObject && doBar)
            {
                // Bar anchor: above the mesh in world Y.
                const Ogre::Real barGap = 0.4f; // world-space gap above mesh top
                const Ogre::Vector3 p(objPos.x, meshTopWorld + barGap, objPos.z);

                const Ogre::Real w = 0.5f;   // half-width of full bar
                const Ogre::Real h = 0.07f;  // bar height
                const Ogre::Real bs = 0.01f; // border thickness
                const Ogre::Real fillW = (w * 2.0f) * static_cast<Ogre::Real>(capturedProgress);
                const Ogre::Real fillR = -w + fillW; // right edge of fill

                const Ogre::ColourValue outer(0.15f, 0.15f, 0.15f, 1.0f);
                const Ogre::ColourValue fTop(0.15f, 0.9f, 0.15f, 1.0f);
                const Ogre::ColourValue fBot(0.10f, 0.65f, 0.10f, 1.0f);

                // *** THE FIX: couldDraw is NEVER reset inside the closure ***
                // Frame 1: getNumSections()==0  → begin()
                // Frame 2+: getNumSections()>0 AND barCouldDraw==true → beginUpdate(0)
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

                // Outer background — two quads (upper + lower half, same as ValueBarComponent)
                // Upper face
                this->barObject->position(p + (o * (so * (sp + Ogre::Vector3(-w - bs, h + bs, 0.0f)))));
                this->barObject->colour(outer);
                this->barObject->normal(0, 0, 1);
                this->barObject->textureCoord(0, 0);
                this->barObject->position(p + (o * (so * (sp + Ogre::Vector3(-w - bs, h + bs, 0.0f)))));
                this->barObject->colour(outer);
                this->barObject->normal(0, 0, 1);
                this->barObject->textureCoord(0, 1);
                this->barObject->position(p + (o * (so * (sp + Ogre::Vector3(w + bs, h + bs, 0.0f)))));
                this->barObject->colour(outer);
                this->barObject->normal(0, 0, 1);
                this->barObject->textureCoord(1, 1);
                this->barObject->position(p + (o * (so * (sp + Ogre::Vector3(w + bs, h + bs, 0.0f)))));
                this->barObject->colour(outer);
                this->barObject->normal(0, 0, 1);
                this->barObject->textureCoord(1, 0);
                this->barObject->quad(idx + 0, idx + 1, idx + 2, idx + 3);
                idx += 4;
                // Lower face
                this->barObject->position(p + (o * (so * (sp + Ogre::Vector3(-w - bs, h + bs, 0.0f)))));
                this->barObject->colour(outer);
                this->barObject->normal(0, 0, 1);
                this->barObject->textureCoord(0, 0);
                this->barObject->position(p + (o * (so * (sp + Ogre::Vector3(-w - bs, -bs, 0.0f)))));
                this->barObject->colour(outer);
                this->barObject->normal(0, 0, 1);
                this->barObject->textureCoord(0, 1);
                this->barObject->position(p + (o * (so * (sp + Ogre::Vector3(w + bs, -bs, 0.0f)))));
                this->barObject->colour(outer);
                this->barObject->normal(0, 0, 1);
                this->barObject->textureCoord(1, 1);
                this->barObject->position(p + (o * (so * (sp + Ogre::Vector3(w + bs, h + bs, 0.0f)))));
                this->barObject->colour(outer);
                this->barObject->normal(0, 0, 1);
                this->barObject->textureCoord(1, 0);
                this->barObject->quad(idx + 0, idx + 1, idx + 2, idx + 3);
                idx += 4;

                // Green fill (only when progress > epsilon)
                if (capturedProgress > 0.001f)
                {
                    this->barObject->position(p + (o * (so * (sp + Ogre::Vector3(-w, h, 0.0f)))));
                    this->barObject->colour(fTop);
                    this->barObject->normal(0, 0, 1);
                    this->barObject->textureCoord(0, 0);
                    this->barObject->position(p + (o * (so * (sp + Ogre::Vector3(-w, h, 0.0f)))));
                    this->barObject->colour(fTop);
                    this->barObject->normal(0, 0, 1);
                    this->barObject->textureCoord(0, 1);
                    this->barObject->position(p + (o * (so * (sp + Ogre::Vector3(fillR, h, 0.0f)))));
                    this->barObject->colour(fTop);
                    this->barObject->normal(0, 0, 1);
                    this->barObject->textureCoord(1, 1);
                    this->barObject->position(p + (o * (so * (sp + Ogre::Vector3(fillR, h, 0.0f)))));
                    this->barObject->colour(fTop);
                    this->barObject->normal(0, 0, 1);
                    this->barObject->textureCoord(1, 0);
                    this->barObject->quad(idx + 0, idx + 1, idx + 2, idx + 3);
                    idx += 4;

                    this->barObject->position(p + (o * (so * (sp + Ogre::Vector3(-w, h, 0.0f)))));
                    this->barObject->colour(fTop);
                    this->barObject->normal(0, 0, 1);
                    this->barObject->textureCoord(0, 0);
                    this->barObject->position(p + (o * (so * (sp + Ogre::Vector3(-w, 0.0f, 0.0f)))));
                    this->barObject->colour(fBot);
                    this->barObject->normal(0, 0, 1);
                    this->barObject->textureCoord(0, 1);
                    this->barObject->position(p + (o * (so * (sp + Ogre::Vector3(fillR, 0.0f, 0.0f)))));
                    this->barObject->colour(fBot);
                    this->barObject->normal(0, 0, 1);
                    this->barObject->textureCoord(1, 1);
                    this->barObject->position(p + (o * (so * (sp + Ogre::Vector3(fillR, h, 0.0f)))));
                    this->barObject->colour(fTop);
                    this->barObject->normal(0, 0, 1);
                    this->barObject->textureCoord(1, 0);
                    this->barObject->quad(idx + 0, idx + 1, idx + 2, idx + 3);
                    idx += 4;
                }

                // Mark successful draw for beginUpdate on the NEXT frame.
                this->barCouldDraw = true;

                this->barObject->index(0);
                this->barObject->end();

                this->barLineNode->setVisible(capturedProgress < 1.0f);
            }

            // ── c) Percentage text ────────────────────────────────────────────────
            //
            // textLineNode is a child of ROOT (same as barLineNode), positioned
            // in world space via updateNodePosition / updateNodeOrientation,
            // just like GameObjectTitleComponent does for its textNode.
            // addTrackedNode() is NOT needed here because we drive the position
            // manually every frame — same approach as the bar.

            if (nullptr != this->percentageText && doText)
            {
                this->percentageText->setCaption(pctStr);

                const Ogre::Real textGap = 0.4f + 0.13f; // bar gap + bar height + small extra
                const Ogre::Vector3 textPos(objPos.x, meshTopWorld + textGap, objPos.z);

                // Follow GameObjectTitleComponent convention: set orientation first, then position.
                NOWA::GraphicsModule::getInstance()->updateNodeOrientation(this->textLineNode, so);
                NOWA::GraphicsModule::getInstance()->updateNodePosition(this->textLineNode, textPos);

                this->textLineNode->setVisible(capturedProgress < 1.0f);
            }
            else if (nullptr != this->textLineNode)
            {
                this->textLineNode->setVisible(false);
            }
        };

        NOWA::GraphicsModule::getInstance()->updateTrackedClosure(this->trackedClosureId(), renderClosure, false);

        // ── 5. Completion ─────────────────────────────────────────────────────────
        if (this->constructionProgress >= 1.0f && false == this->isConstructionFinished)
        {
            this->isConstructionFinished = true;

            // FIX: Upload complete index buffer SYNCHRONOUSLY before removing the
            // tracked closure.  This guarantees the final frame shows the complete
            // mesh regardless of whether the closure runs one more time.
            // Normal: show complete mesh. Inverted: show empty mesh (demolished).
            this->uploadFinalIndicesBlocking(!this->invert->getBool());

            // Now safe to stop the animation.
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
                    catch (luabind::error& err)
                    {
                        luabind::object msg(luabind::from_stack(err.state(), -1));
                        std::stringstream ss;
                        ss << msg;
                        Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[MeshConstructionComponent] reactOnConstructionDone error: " + Ogre::String(err.what()) + " | " + ss.str());
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
        else if (AttrInvert() == attribute->getName())
        {
            this->setInvert(attribute->getBool());
        }
        else if (AttrConstructionAxis() == attribute->getName())
        {
            this->setConstructionAxis(attribute->getListSelectedValue());
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
        add("12", AttrInvert(), XMLConverter::ConvertString(doc, this->invert->getBool()));
        add("9", AttrConstructionAxis(), XMLConverter::ConvertString(doc, this->constructionAxis->getListSelectedValue()));
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
                                const float sc = 1.f / 32767.f;
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

            if (nullptr != vao->getIndexBuffer())
            {
                Ogre::IndexBufferPacked* ibp = vao->getIndexBuffer();
                Ogre::AsyncTicketPtr t = ibp->readRequest(0, ibp->getNumElements());
                const void* r = t->map();
                const bool is32 = (ibp->getIndexType() == Ogre::IndexBufferPacked::IT_32BIT);
                for (size_t ii = 0; ii < smIC; ++ii)
                {
                    Ogre::uint32 idx = is32 ? reinterpret_cast<const Ogre::uint32*>(r)[ii] : static_cast<Ogre::uint32>(reinterpret_cast<const Ogre::uint16*>(r)[ii]);
                    this->indices.push_back(static_cast<Ogre::uint32>(gvOff + idx));
                }
                t->unmap();
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

        // Recompute bounds from ACTUAL vertex data (getBounds() can differ slightly).
        // meshMinY/meshMaxY  — always Y, used to position the progress bar above the object.
        // meshAxisMin/Max    — along the chosen construction axis, used for the cutoff sweep.
        {
            float minY = std::numeric_limits<float>::max();
            float maxY = -std::numeric_limits<float>::max();
            float minA = std::numeric_limits<float>::max();
            float maxA = -std::numeric_limits<float>::max();
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
                const float a = this->axisComponent(v);
                if (a < minA)
                {
                    minA = a;
                }
                if (a > maxA)
                {
                    maxA = a;
                }
            }
            this->meshMinY = minY;
            this->meshMaxY = maxY;
            this->meshAxisMin = minA;
            this->meshAxisMax = maxA;
            if (this->meshAxisMin >= this->meshAxisMax)
            {
                // Degenerate — fall back to Y bounds.
                this->meshAxisMin = this->meshMinY;
                this->meshAxisMax = this->meshMaxY;
            }
        }

        // Sort triangles by centroid along the chosen construction axis (one-time O(n log n)).
        for (SubMeshInfo& si : this->subMeshInfoList)
        {
            const size_t triCount = si.indexCount / 3;
            si.sortedTriIndices.resize(triCount);
            si.sortedCentroid.resize(triCount);

            for (size_t t = 0; t < triCount; ++t)
            {
                const size_t i0 = this->indices[si.indexOffset + t * 3];
                const size_t i1 = this->indices[si.indexOffset + t * 3 + 1];
                const size_t i2 = this->indices[si.indexOffset + t * 3 + 2];
                si.sortedCentroid[t] = (this->axisComponent(this->vertices[i0]) + this->axisComponent(this->vertices[i1]) + this->axisComponent(this->vertices[i2])) / 3.0f;
                si.sortedTriIndices[t] = t;
            }
            std::sort(si.sortedTriIndices.begin(), si.sortedTriIndices.end(),
                [&si](size_t a, size_t b)
                {
                    return si.sortedCentroid[a] < si.sortedCentroid[b];
                });

            std::vector<float> sortedC(triCount);
            for (size_t i = 0; i < triCount; ++i)
            {
                sortedC[i] = si.sortedCentroid[si.sortedTriIndices[i]];
            }
            si.sortedCentroid = std::move(sortedC);
        }

        // Build GPU mesh.
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
        this->constructionItem = nullptr;
        if (false == this->constructionMesh.isNull())
        {
            Ogre::MeshManager::getSingleton().remove(this->constructionMesh);
            this->constructionMesh.reset();
        }
    }

    // =========================================================================
    //  createOverlays  —  render thread
    //
    //  ALL nodes (bar, text) are children of the ROOT scene node.
    //  This means there is NO inherited scale, rotation or position from the
    //  game object.  All positions are computed in world space inside the
    //  render closure each frame.
    //
    //  The text node uses addTrackedNode() for automatic camera-facing.
    // =========================================================================

    void MeshConstructionComponent::createOverlays(void)
    {
        Ogre::SceneManager* sm = this->gameObjectPtr->getSceneManager();
        Ogre::SceneNode* root = sm->getRootSceneNode();
        const Ogre::SceneMemoryMgrTypes memType = this->gameObjectPtr->isDynamic() ? Ogre::SCENE_DYNAMIC : Ogre::SCENE_STATIC;

        this->barCouldDraw = false;

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
        }

        // ── Percentage text ───────────────────────────────────────────────────────
        if (this->showPercentageText->getBool())
        {
            Ogre::NameValuePairList params;
            params["name"] = Ogre::StringConverter::toString(this->gameObjectPtr->getId()) + "_ConstrText";
            params["Caption"] = "0 %";
            params["fontName"] = "BlueHighway";

            this->percentageText = new MovableText(Ogre::Id::generateNewId<Ogre::MovableObject>(), &sm->_getEntityMemoryManager(Ogre::SCENE_DYNAMIC), sm, &params);
            this->percentageText->setCaption("0 %");
            this->percentageText->setCharacterHeight(0.4f);
            this->percentageText->showOnTop(true);
            this->percentageText->setColor(Ogre::ColourValue(1.0f, 1.0f, 0.2f, 1.0f));
            this->percentageText->setTextAlignment(MovableText::H_CENTER, MovableText::V_CENTER);
            this->percentageText->setQueryFlags(0 << 0);

            // Child of ROOT — no inherited transform. Position driven each frame.
            this->textLineNode = root->createChildSceneNode(memType);
            this->textLineNode->attachObject(this->percentageText);

            // addTrackedNode: render thread will orient this node toward the camera.
            // Same call as GameObjectTitleComponent::postInit.
            NOWA::GraphicsModule::getInstance()->addTrackedNode(this->textLineNode);
            this->textLineNode->setVisible(false);
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

        if (nullptr != this->percentageText)
        {
            NOWA::GraphicsModule::getInstance()->removeTrackedNode(this->textLineNode);
            this->textLineNode->detachAllObjects();
            delete this->percentageText;
            this->percentageText = nullptr;
        }
        if (nullptr != this->textLineNode)
        {
            this->textLineNode->getParentSceneNode()->removeAndDestroyChild(this->textLineNode);
            this->textLineNode = nullptr;
        }
        this->barCouldDraw = false;
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
                .def("setInvert", &MeshConstructionComponent::setInvert)
                .def("getInvert", &MeshConstructionComponent::getInvert)
                .def("setConstructionAxis", &MeshConstructionComponent::setConstructionAxis)
                .def("getConstructionAxis", &MeshConstructionComponent::getConstructionAxis)
                .def("reactOnConstructionDone", &MeshConstructionComponent::reactOnConstructionDone)];

        LuaScriptApi::getInstance()->addClassToCollection("MeshConstructionComponent", "class inherits GameObjectComponent", MeshConstructionComponent::getStaticInfoText());
        LuaScriptApi::getInstance()->addClassToCollection("MeshConstructionComponent", "void reactOnConstructionDone(func closureFunction)", "Lua callback at 100%. Signature: function()");
        LuaScriptApi::getInstance()->addClassToCollection("MeshConstructionComponent", "float getConstructionProgress()", "Returns progress [0..1].");
        LuaScriptApi::getInstance()->addClassToCollection("MeshConstructionComponent", "void setInvert(bool invert)", "If true: mesh deconstructs top-to-bottom (demolition). If false: builds bottom-to-top.");
        LuaScriptApi::getInstance()->addClassToCollection("MeshConstructionComponent", "void setConstructionAxis(String axis)", "Axis along which construction sweeps. 'X'=left/right, 'Y'=bottom/top, 'Z'=front/back.");

        gameObjectClass.def("getMeshConstructionComponent", (MeshConstructionComponent * (*)(GameObject*)) & getMeshConstructionComponent);
        gameObjectClass.def("getMeshConstructionComponentFromName", &getMeshConstructionComponentFromName);
        gameObjectControllerClass.def("castMeshConstructionComponent", &GameObjectController::cast<MeshConstructionComponent>);

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "MeshConstructionComponent getMeshConstructionComponent()", "Gets the component.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "MeshConstructionComponent castMeshConstructionComponent(MeshConstructionComponent other)", "Casts for Lua auto-completion.");
    }

} // namespace NOWA
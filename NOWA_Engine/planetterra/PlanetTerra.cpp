#include "NOWAPrecompiled.h"
#include "PlanetTerra.h"

#include "OgreBitwise.h"
#include "OgreHlmsManager.h"
#include "OgreLogManager.h"
#include "OgreMeshManager2.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreRenderSystem.h"
#include "OgreRoot.h"
#include "OgreSubMesh2.h"
#include "OgreTextureGpuManager.h"
#include "Vao/OgreVertexArrayObject.h"

#include "OgreMeshManager2.h"
#include "OgreConfigFile.h"
#include "OgreLodConfig.h"
#include "OgreLodStrategyManager.h"
#include "OgreMeshLodGenerator.h"
#include "OgreMesh2Serializer.h"
#include "OgrePixelCountLodStrategy.h"
#include "OgreSubMesh2.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace NOWA
{
    // =========================================================================
    //  Constructor / Destructor
    // =========================================================================

    PlanetTerra::PlanetTerra(const Ogre::String& name, Ogre::SceneManager* sm) : objectName(name), sceneManager(sm)
    {
    }

    PlanetTerra::~PlanetTerra()
    {
        // Caller (PlanetTerraComponent) must call destroy() on the render thread first.
    }

    // =========================================================================
    //  RENDER THREAD  —  create / destroy
    // =========================================================================

    bool PlanetTerra::create(float r, int segH, int segV, int blendTex, Ogre::SceneNode* attachNode, const Ogre::String& datablockName, float lodDistance, bool useWeightTexture)
    {
        this->radius = r;
        this->segmentsH = segH;
        this->segmentsV = segV;
        this->blendTexSize = blendTex;
        this->attachedNode = attachNode;
        this->useWeightTexture = useWeightTexture;
        this->lodDistance = lodDistance;

        // Build CPU geometry (safe on any thread)
        generateBaseSphere();
        rebuildGeometryFromHeight();
        recalculateNormals();
        recalculateTangents();
        buildVertexAdjacency();

        if (true == this->useWeightTexture)
        {
            // Default blend: fully layer-0
            this->blendData.assign(static_cast<size_t>(this->blendTexSize) * this->blendTexSize * 4u, 0u);
            for (size_t i = 0u; i < this->blendData.size(); i += 4u)
            {
                this->blendData[i] = 255u;
                this->blendData[i + 1] = 0u;
                this->blendData[i + 2] = 0u;
                this->blendData[i + 3] = 0u;
            }
        }

        const Ogre::String meshName = objectName + "_PlanetMesh";
        this->planetItem = buildItemFromCPUArrays(meshName);
        if (nullptr == this->planetItem)
        {
            return false;
        }

        setDatablockByName(datablockName);
        if (true == this->useWeightTexture)
        {
            createBlendWeightTexture();
            uploadBlendData();
        }
        // Match the item's static state to the node before re-attaching.
        if (this->planetItem->isStatic() != attachedNode->isStatic())
        {
            this->planetItem->setStatic(attachedNode->isStatic());
        }
        attachedNode->attachObject(this->planetItem); 

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PlanetTerra] Created planet '" + objectName + "' radius=" + Ogre::StringConverter::toString(radius) + " segH=" + Ogre::StringConverter::toString(segmentsH) +
                                                                               " segV=" + Ogre::StringConverter::toString(segmentsV) + " verts=" + Ogre::StringConverter::toString(static_cast<unsigned int>(vertexCount)));

        return true;
    }

    void PlanetTerra::destroy()
    {
        destroyBlendWeightTexture();

        if (planetItem)
        {
            if (planetItem->isAttached())
            {
                planetItem->detachFromParent();
            }
            sceneManager->destroyItem(planetItem);
            planetItem = nullptr;
            dynamicVB = nullptr; // destroyed with mesh/VAO
        }
        if (!planetMesh.isNull())
        {
            Ogre::MeshManager::getSingleton().remove(planetMesh->getHandle());
            planetMesh.reset();
        }
    }

    // =========================================================================
    //  RENDER THREAD  —  internal VAO/Item builder  (shared by create + rebuild)
    // =========================================================================

    Ogre::Item* PlanetTerra::buildItemFromCPUArrays(const Ogre::String& meshName)
    {
        Ogre::Root* root = Ogre::Root::getSingletonPtr();
        Ogre::VaoManager* vm = root->getRenderSystem()->getVaoManager();
        const Ogre::String groupName = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;

        // Remove any stale mesh registered under this name
        {
            Ogre::MeshPtr existing = Ogre::MeshManager::getSingleton().getByName(meshName, groupName);
            if (!existing.isNull())
            {
                Ogre::MeshManager::getSingleton().remove(existing->getHandle());
            }
        }

        planetMesh = Ogre::MeshManager::getSingleton().createManual(meshName, groupName);

        // Vertex layout: pos(3) + normal(3) + tangent(4) + uv0(2) + uv1(2) = 14 floats/vertex
        // UV0 = equirectangular 0..1 - used by detail textures and paint brush
        // UV1 = UV0 * baseUVScale  - used by main diffuse and normal textures
        Ogre::VertexElement2Vec elems;
        elems.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_POSITION));
        elems.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_NORMAL));
        elems.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT4, Ogre::VES_TANGENT));
        elems.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES));
        elems.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES));

        constexpr size_t fpv = 14u;

        // Pack vertex data
        float* vd = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(vertexCount * fpv * sizeof(float), Ogre::MEMCATEGORY_GEOMETRY));

        for (size_t i = 0u; i < vertexCount; ++i)
        {
            size_t o = i * fpv;
            vd[o++] = vertices[i].x;
            vd[o++] = vertices[i].y;
            vd[o++] = vertices[i].z;
            vd[o++] = normals[i].x;
            vd[o++] = normals[i].y;
            vd[o++] = normals[i].z;
            vd[o++] = tangents[i].x;
            vd[o++] = tangents[i].y;
            vd[o++] = tangents[i].z;
            vd[o++] = tangents[i].w;
            // UV0: equirectangular, used for detail textures + blend map + paint
            vd[o++] = uvCoords[i].x;
            vd[o++] = uvCoords[i].y;
            // UV1: scaled for main diffuse + normal, tiles at baseUVScale
            vd[o++] = uvCoords[i].x * this->baseUVScale;
            vd[o] = uvCoords[i].y * this->baseUVScale;
        }

        Ogre::VertexBufferPacked* vb = nullptr;
        try
        {
            // BT_DYNAMIC_DEFAULT: we own vd, upload it, then free
            vb = vm->createVertexBuffer(elems, vertexCount, Ogre::BT_DYNAMIC_DEFAULT, vd, false);
        }
        catch (Ogre::Exception& e)
        {
            OGRE_FREE_SIMD(vd, Ogre::MEMCATEGORY_GEOMETRY);
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PlanetTerra] createVertexBuffer failed: " + e.getDescription());
            return nullptr;
        }
        OGRE_FREE_SIMD(vd, Ogre::MEMCATEGORY_GEOMETRY);
        dynamicVB = vb;

        // Index buffer — BT_IMMUTABLE, topology never changes for a given seg-count
        Ogre::uint32* id = reinterpret_cast<Ogre::uint32*>(OGRE_MALLOC_SIMD(indexCount * sizeof(Ogre::uint32), Ogre::MEMCATEGORY_GEOMETRY));
        memcpy(id, indices.data(), indexCount * sizeof(Ogre::uint32));

        Ogre::IndexBufferPacked* ib = nullptr;
        try
        {
            ib = vm->createIndexBuffer(Ogre::IndexBufferPacked::IT_32BIT, indexCount, Ogre::BT_IMMUTABLE, id, true /*keepAsShadow*/);
        }
        catch (Ogre::Exception& e)
        {
            OGRE_FREE_SIMD(id, Ogre::MEMCATEGORY_GEOMETRY);
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PlanetTerra] createIndexBuffer failed: " + e.getDescription());
            return nullptr;
        }

        Ogre::VertexBufferPackedVec vbVec;
        vbVec.push_back(vb);
        Ogre::VertexArrayObject* vao = vm->createVertexArrayObject(vbVec, ib, Ogre::OT_TRIANGLE_LIST);

        Ogre::SubMesh* subMesh = planetMesh->createSubMesh();
        subMesh->mVao[Ogre::VpNormal].push_back(vao);
        subMesh->mVao[Ogre::VpShadow].push_back(vao);

        // Bounds: use computed max radius with a small margin
        const float maxR = computedMaxRadius * 1.02f;
        Ogre::Aabb aabb(Ogre::Vector3::ZERO, Ogre::Vector3(maxR));
        this->planetMesh->_setBounds(aabb, false);
        this->planetMesh->_setBoundingSphereRadius(maxR);

        // In buildItemFromCPUArrays, BEFORE calling buildLodVaos:
        // Use forceSameBuffers=true so VpShadow shares VpNormal VAOs at all LOD levels.
        // Must be called before buildLodVaos so the shadow VAO array gets all LOD entries.
        if (!this->planetMesh->hasValidShadowMappingVaos())
        {
            this->planetMesh->prepareForShadowMapping(true);
        }

        // Add LOD VAOs after the full-detail VAO is already in subMesh->mVao[VpNormal][0].
        // Use renderDistance as the LOD distance -- caller sets this later via setRenderDistance.
        // Default to radius * 4 which is a reasonable far distance for a planet.
        // this->buildLodVaos(subMesh);

        Ogre::Item* item = sceneManager->createItem(planetMesh, Ogre::SCENE_DYNAMIC);
        item->setName(objectName + "_PlanetItem");

        return item;
    }

    void PlanetTerra::buildLodVaos(Ogre::SubMesh* subMesh)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PlanetTerra] buildLodVaos called"
                                                                            " VpNormal.size=" +
                                                                                Ogre::StringConverter::toString(static_cast<unsigned int>(subMesh->mVao[Ogre::VpNormal].size())) + " VpShadow.size=" +
                                                                                Ogre::StringConverter::toString(static_cast<unsigned int>(subMesh->mVao[Ogre::VpShadow].size())) + " lodDistance=" + Ogre::StringConverter::toString(lodDistance));

       // LOD distances must be large multiples of the radius to avoid
        // visible polygon degradation when the camera is still close.
        // Use lodDistance as the LAST (lowest quality) transition,
        // but enforce a minimum of 3x the radius.
        const float minLodDist = this->radius * 3.0f;
        // const float effectiveLodDist = std::max(this->lodDistance, minLodDist);
        const float effectiveLodDist = minLodDist;

        // Generate 3 LOD levels at 1/2, 1/4, 1/8 of the full segment count.
        // Each LOD level gets its own immutable VAO pushed into mVao[VpNormal].
        // mLodValues gets a matching entry so Ogre knows when to switch.
        Ogre::Root* root = Ogre::Root::getSingletonPtr();
        Ogre::VaoManager* vm = root->getRenderSystem()->getVaoManager();

        const int lodDivisors[] = {2, 4, 8};
        const int numLods = 3;

        Ogre::LodStrategy* strategy = Ogre::LodStrategyManager::getSingleton().getStrategy(planetMesh->getLodStrategyName());
        if (nullptr == strategy)
        {
            strategy = Ogre::LodStrategyManager::getSingleton().getDefaultStrategy();
        }

        Ogre::Mesh::LodValueArray* lodValues = const_cast<Ogre::Mesh::LodValueArray*>(planetMesh->_getLodValueArray());

        for (int lod = 0; lod < numLods; ++lod)
        {
            const int div = lodDivisors[lod];
            const int lodSegH = std::max(4, segmentsH / div);
            const int lodSegV = std::max(4, segmentsV / div);

            const size_t lodVertCount = static_cast<size_t>((lodSegH + 1) * (lodSegV + 1));
            const size_t lodIndexCount = static_cast<size_t>(lodSegH) * static_cast<size_t>(lodSegV) * 6u;

            // Build simplified sphere positions only (no normals/tangents needed for LOD geo)
            constexpr size_t fpv = 14u;
            float* vd = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(lodVertCount * fpv * sizeof(float), Ogre::MEMCATEGORY_GEOMETRY));

            const float PI = Ogre::Math::PI;
            const float TWO_PI = Ogre::Math::TWO_PI;

            for (int v = 0; v <= lodSegV; ++v)
            {
                const float phi = static_cast<float>(v) / static_cast<float>(lodSegV) * PI;
                for (int h = 0; h <= lodSegH; ++h)
                {
                    const float theta = static_cast<float>(h) / static_cast<float>(lodSegH) * TWO_PI;
                    const size_t idx = static_cast<size_t>(v * (lodSegH + 1) + h);
                    const size_t o = idx * fpv;

                    Ogre::Vector3 dir(Ogre::Math::Sin(phi) * Ogre::Math::Cos(theta), Ogre::Math::Cos(phi), Ogre::Math::Sin(phi) * Ogre::Math::Sin(theta));
                    dir.normalise();

                    // Sample height at nearest full-res vertex
                    // Simple approximation: use base radius only for LOD levels
                    const float r = radius;
                    const Ogre::Vector3 pos = dir * r;

                    vd[o + 0] = pos.x;
                    vd[o + 1] = pos.y;
                    vd[o + 2] = pos.z;
                    vd[o + 3] = dir.x; // normal = direction for sphere
                    vd[o + 4] = dir.y;
                    vd[o + 5] = dir.z;
                    vd[o + 6] = 1.0f; // tangent
                    vd[o + 7] = 0.0f;
                    vd[o + 8] = 0.0f;
                    vd[o + 9] = 1.0f;
                    vd[o + 10] = static_cast<float>(h) / static_cast<float>(lodSegH); // uv0
                    vd[o + 11] = static_cast<float>(v) / static_cast<float>(lodSegV);
                    vd[o + 12] = vd[o + 10] * baseUVScale; // uv1
                    vd[o + 13] = vd[o + 11] * baseUVScale;
                }
            }

            // Build index buffer
            Ogre::uint32* id = reinterpret_cast<Ogre::uint32*>(OGRE_MALLOC_SIMD(lodIndexCount * sizeof(Ogre::uint32), Ogre::MEMCATEGORY_GEOMETRY));

            size_t iIdx = 0u;
            for (int v = 0; v < lodSegV; ++v)
            {
                for (int h = 0; h < lodSegH; ++h)
                {
                    const Ogre::uint32 tl = static_cast<Ogre::uint32>(v * (lodSegH + 1) + h);
                    const Ogre::uint32 tr = static_cast<Ogre::uint32>(v * (lodSegH + 1) + h + 1);
                    const Ogre::uint32 bl = static_cast<Ogre::uint32>((v + 1) * (lodSegH + 1) + h);
                    const Ogre::uint32 br = static_cast<Ogre::uint32>((v + 1) * (lodSegH + 1) + h + 1);
                    id[iIdx++] = tl;
                    id[iIdx++] = tr;
                    id[iIdx++] = bl;
                    id[iIdx++] = tr;
                    id[iIdx++] = br;
                    id[iIdx++] = bl;
                }
            }

            Ogre::VertexElement2Vec elems;
            elems.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_POSITION));
            elems.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_NORMAL));
            elems.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT4, Ogre::VES_TANGENT));
            elems.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES));
            elems.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES));

            Ogre::VertexBufferPacked* lodVB = vm->createVertexBuffer(elems, lodVertCount, Ogre::BT_IMMUTABLE, vd, true);
            OGRE_FREE_SIMD(vd, Ogre::MEMCATEGORY_GEOMETRY);

            Ogre::IndexBufferPacked* lodIB = vm->createIndexBuffer(Ogre::IndexBufferPacked::IT_32BIT, lodIndexCount, Ogre::BT_IMMUTABLE, id, true);

            Ogre::VertexBufferPackedVec vbVec;
            vbVec.push_back(lodVB);
            Ogre::VertexArrayObject* lodVao = vm->createVertexArrayObject(vbVec, lodIB, Ogre::OT_TRIANGLE_LIST);

            subMesh->mVao[Ogre::VpNormal].push_back(lodVao);
            subMesh->mVao[Ogre::VpShadow].push_back(lodVao);

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PlanetTerra] LOD " + Ogre::StringConverter::toString(lod) +
                                                                                    " pushed VpNormal.size=" + Ogre::StringConverter::toString(static_cast<unsigned int>(subMesh->mVao[Ogre::VpNormal].size())) +
                                                                                    " VpShadow.size=" + Ogre::StringConverter::toString(static_cast<unsigned int>(subMesh->mVao[Ogre::VpShadow].size())));

            // LOD distance: evenly spaced up to lodDistance
            const float dist = effectiveLodDist * (static_cast<float>(lod + 1) / static_cast<float>(numLods));
            lodValues->push_back(strategy->transformUserValue(dist));

            // Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PlanetTerra] LOD " + Ogre::StringConverter::toString(lod) + " segH=" + Ogre::StringConverter::toString(lodSegH) + " segV=" + Ogre::StringConverter::toString(lodSegV) +
            //                                                                        " verts=" + Ogre::StringConverter::toString(static_cast<unsigned int>(lodVertCount)) + " dist=" + Ogre::StringConverter::toString(dist));
        }
    }

    // =========================================================================
    //  RENDER THREAD  —  full topology rebuild  (called when seg-count changes)
    // =========================================================================

    void PlanetTerra::rebuildDynamicBuffers()
    {
        if (!attachedNode)
        {
            return;
        }

        ++rebuildCounter;
        const Ogre::String newMeshName = objectName + "_PlanetMesh_" + Ogre::StringConverter::toString(rebuildCounter);

        // CPU side already updated by caller (segmentsH/V changed → generateBaseSphere called)
        Ogre::Item* newItem = buildItemFromCPUArrays(newMeshName);
        if (!newItem)
        {
            return;
        }

        // Datablock carry-over
        if (planetItem && planetItem->getNumSubItems() > 0 && newItem->getNumSubItems() > 0)
        {
            Ogre::HlmsDatablock* db = planetItem->getSubItem(0)->getDatablock();
            if (db)
            {
                newItem->getSubItem(0)->setDatablock(db);
            }
        }

        // Atomic scene-node swap
        if (planetItem)
        {
            if (planetItem->isAttached())
            {
                planetItem->detachFromParent();
            }
            sceneManager->destroyItem(planetItem);
            dynamicVB = nullptr;
        }
        if (!planetMesh.isNull())
        {
            Ogre::MeshManager::getSingleton().remove(planetMesh->getHandle());
            planetMesh.reset();
        }

        // Build new mesh is already stored in planetMesh by buildItemFromCPUArrays
        planetItem = newItem;
        attachedNode->attachObject(planetItem);

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PlanetTerra] Rebuilt buffers '" + objectName + "' verts=" + Ogre::StringConverter::toString(static_cast<unsigned int>(vertexCount)));
    }

    // =========================================================================
    //  RENDER THREAD  —  fast VBO re-upload  (brush stroke hot-path)
    // =========================================================================

    void PlanetTerra::uploadVertexData()
    {
        if (!dynamicVB)
        {
            return;
        }

        constexpr size_t fpv = 14u;
        float* vd = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(vertexCount * fpv * sizeof(float), Ogre::MEMCATEGORY_GEOMETRY));

        for (size_t i = 0u; i < vertexCount; ++i)
        {
            size_t o = i * fpv;
            vd[o++] = vertices[i].x;
            vd[o++] = vertices[i].y;
            vd[o++] = vertices[i].z;
            vd[o++] = normals[i].x;
            vd[o++] = normals[i].y;
            vd[o++] = normals[i].z;
            vd[o++] = tangents[i].x;
            vd[o++] = tangents[i].y;
            vd[o++] = tangents[i].z;
            vd[o++] = tangents[i].w;
            // UV0
            vd[o++] = uvCoords[i].x;
            vd[o++] = uvCoords[i].y;
            // UV1: scaled for main diffuse + normal
            vd[o++] = uvCoords[i].x * this->baseUVScale;
            vd[o] = uvCoords[i].y * this->baseUVScale;
        }

        dynamicVB->upload(vd, 0u, static_cast<Ogre::uint32>(vertexCount));
        OGRE_FREE_SIMD(vd, Ogre::MEMCATEGORY_GEOMETRY);

        // Keep mesh bounds in sync with current geometry
        const float maxR = computedMaxRadius * 1.02f;
        if (!planetMesh.isNull())
        {
            Ogre::Aabb aabb(Ogre::Vector3::ZERO, Ogre::Vector3(maxR));
            planetMesh->_setBounds(aabb, false);
            planetMesh->_setBoundingSphereRadius(maxR);
        }
    }

    // =========================================================================
    //  RENDER THREAD  —  blend weight texture
    // =========================================================================

    void PlanetTerra::createBlendWeightTexture()
    {
        destroyBlendWeightTexture();

        Ogre::TextureGpuManager* texMgr = sceneManager->getDestinationRenderSystem()->getTextureGpuManager();

        blendWeightTex = texMgr->createTexture(this->objectName + "_blendWeight", Ogre::GpuPageOutStrategy::SaveToSystemRam, Ogre::TextureFlags::RenderToTexture, Ogre::TextureTypes::Type2D);
        blendWeightTex->setResolution(blendTexSize, blendTexSize);
        blendWeightTex->setPixelFormat(Ogre::PFG_RGBA8_UNORM_SRGB);
        // Must use _transitionTo (immediate) NOT scheduleTransitionTo (async).
        // Matches Terra::createBlendWeightTexture exactly.
        blendWeightTex->_transitionTo(Ogre::GpuResidency::Resident, reinterpret_cast<Ogre::uint8*>(0));
        blendWeightTex->_setNextResidencyStatus(Ogre::GpuResidency::Resident);

        // Staging texture is created once and kept persistent across all paint operations.
        // Recreating it on every upload resets the data — Terra never recreates it.
        if (nullptr == blendStagingTex)
        {
            blendStagingTex = texMgr->getStagingTexture(blendTexSize, blendTexSize, 1u, 1u, Ogre::PFG_RGBA8_UNORM_SRGB);
        }

        blendStagingTex->startMapRegion();
        Ogre::TextureBox texBox = blendStagingTex->mapRegion(blendTexSize, blendTexSize, 1u, 1u, Ogre::PFG_RGBA8_UNORM_SRGB);

        const size_t bytesPerPixel = texBox.bytesPerPixel;

        //// Default: NO detail layer active -> base diffuse (ground_dirt_gardenD) shows
        for (Ogre::uint32 y = 0u; y < static_cast<Ogre::uint32>(blendTexSize); ++y)
        {
            Ogre::uint8* RESTRICT_ALIAS pix = reinterpret_cast<Ogre::uint8 * RESTRICT_ALIAS>(texBox.at(0, y, 0));
            for (Ogre::uint32 x = 0u; x < static_cast<Ogre::uint32>(blendTexSize); ++x)
            {
                float rgba[4] = {0.0f, 0.0f, 0.0f, 0.0f}; // was {1,0,0,0} — that forced adesert_cracks!
                Ogre::PixelFormatGpuUtils::packColour(rgba, Ogre::PFG_RGBA8_UNORM_SRGB, &pix[x * bytesPerPixel]);
            }
        }

        // Keep CPU blendData in sync with staging data.
        // Do NOT use texBox.bytesPerImage -- getStagingTexture() may return a pooled staging
        // texture larger than requested, making bytesPerImage > blendTexSize*blendTexSize*4.
        // Copy row-by-row using the actual pixel dimensions to handle row-pitch alignment.
        const size_t rowBytes = static_cast<size_t>(blendTexSize) * 4u;
        const size_t totalBytes = rowBytes * static_cast<size_t>(blendTexSize);
        blendData.resize(totalBytes, 0u);
        for (Ogre::uint32 row = 0u; row < static_cast<Ogre::uint32>(blendTexSize); ++row)
        {
            const Ogre::uint8* srcRow = reinterpret_cast<const Ogre::uint8*>(texBox.at(0, row, 0));
            memcpy(&blendData[row * rowBytes], srcRow, rowBytes);
        }

        // Keep CPU blendData in sync with staging data
        // blendData.resize(static_cast<size_t>(blendTexSize) * blendTexSize * 4u, 0u);
        // memcpy(blendData.data(), texBox.data, texBox.bytesPerImage);

        blendStagingTex->stopMapRegion();
        blendStagingTex->upload(texBox, blendWeightTex, 0u);
    }

    void PlanetTerra::destroyBlendWeightTexture()
    {
        Ogre::TextureGpuManager* texMgr = sceneManager->getDestinationRenderSystem()->getTextureGpuManager();

        if (blendStagingTex)
        {
            texMgr->removeStagingTexture(blendStagingTex);
            blendStagingTex = nullptr;
        }
        if (blendWeightTex)
        {
            texMgr->destroyTexture(blendWeightTex);
            blendWeightTex = nullptr;
        }
    }

    void PlanetTerra::uploadBlendData()
    {
        if (!this->blendWeightTex || !this->blendStagingTex)
        {
            return;
        }

        if (this->blendWeightTex->getResidencyStatus() != Ogre::GpuResidency::Resident)
        {
            this->blendWeightTex->_transitionTo(Ogre::GpuResidency::Resident, reinterpret_cast<Ogre::uint8*>(0));
            this->blendWeightTex->_setNextResidencyStatus(Ogre::GpuResidency::Resident);
        }

        // Guard: staging texture _getSizeBytes() must match blendData size exactly.
        // If size differs (row padding), recreate the staging texture like Terra does.
        const size_t expectedBytes = static_cast<size_t>(this->blendTexSize) * this->blendTexSize * 4u;
        if (this->blendStagingTex->_getSizeBytes() != expectedBytes)
        {
            Ogre::TextureGpuManager* texMgr = this->sceneManager->getDestinationRenderSystem()->getTextureGpuManager();
            texMgr->removeStagingTexture(this->blendStagingTex);
            this->blendStagingTex = nullptr;

            bool correctCandidate = false;
            while (!correctCandidate)
            {
                this->blendStagingTex = texMgr->getStagingTexture(this->blendTexSize, this->blendTexSize, 1u, 1u, Ogre::PFG_RGBA8_UNORM_SRGB);
                if (this->blendStagingTex->_getSizeBytes() == expectedBytes)
                {
                    correctCandidate = true;
                }
            }
        }

        this->blendStagingTex->startMapRegion();
        Ogre::TextureBox texBox = this->blendStagingTex->mapRegion(this->blendTexSize, this->blendTexSize, 1u, 1u, Ogre::PFG_RGBA8_UNORM_SRGB);

        memcpy(texBox.data, this->blendData.data(), this->blendStagingTex->_getSizeBytes());

        this->blendStagingTex->stopMapRegion();
        this->blendStagingTex->upload(texBox, this->blendWeightTex, 0u);
    }

    bool PlanetTerra::saveBlendDataToFile(const Ogre::String& filePathName) const
    {
        if (false == this->useWeightTexture)
        {
            return true;
        }
        // Write directly from the CPU blendData array via Image2.
        // writeContentsToFile reads from the GPU system-RAM copy which is not
        // reliably populated when using _transitionTo(Resident, nullptr).
        // The CPU blendData array is always authoritative (all paint strokes
        // are applied there first, then uploaded to GPU).
        if (this->blendData.empty() || this->blendTexSize <= 0)
        {
            return false;
        }

        try
        {
            Ogre::Image2 img;
            img.createEmptyImage(static_cast<Ogre::uint32>(this->blendTexSize), static_cast<Ogre::uint32>(this->blendTexSize), 1u, Ogre::TextureTypes::Type2D, Ogre::PFG_RGBA8_UNORM_SRGB);

            Ogre::TextureBox box = img.getData(0u);
            const size_t copyBytes = std::min(this->blendData.size(), static_cast<size_t>(box.bytesPerRow) * this->blendTexSize);
            memcpy(box.data, this->blendData.data(), copyBytes);

            img.save(filePathName, 0u, 1u);

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PlanetTerra] Saved blend data to: " + filePathName);
            return true;
        }
        catch (Ogre::Exception& e)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PlanetTerra] Failed to save blend data to '" + filePathName + "': " + e.getDescription());
            return false;
        }
    }

    void PlanetTerra::setDatablockByName(const Ogre::String& name)
    {
        if (nullptr == this->planetItem || name.empty())
        {
            return;
        }

        Ogre::HlmsPbsDatablock* db = static_cast<Ogre::HlmsPbsDatablock*>(Ogre::Root::getSingleton().getHlmsManager()->getDatablockNoDefault(name));

        if (nullptr == db)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PlanetTerra] Datablock '" + name + "' not found.");
            return;
        }

        this->planetItem->getSubItem(0)->setDatablock(db);
    }

    // =========================================================================
    //  MAIN THREAD  —  sphere geometry generation
    // =========================================================================

    const std::vector<Ogre::Vector2>& PlanetTerra::getUvCoords(void) const
    {
        return this->uvCoords;
    }

    void PlanetTerra::setDynamic(bool dynamic)
    {
        if (nullptr != planetItem)
        {
            this->planetItem->setStatic(!dynamic);
        }
    }

    bool PlanetTerra::findFlatLandingVertex(const Ogre::Vector3& outwardDir, float searchHalfAngleDeg, Ogre::Vector3& outLocalPos, Ogre::Vector3& outLocalNormal) const
    {
        if (0u == this->vertexCount)
        {
            return false;
        }

        const float cosHalfAngle = Ogre::Math::Cos(Ogre::Degree(searchHalfAngleDeg));

        // A vertex qualifies if its base direction is within searchHalfAngleDeg of outwardDir.
        // Among qualifying vertices, pick the one whose surface normal is most aligned with
        // outwardDir (i.e. the flattest, least-sloped spot near the ship).
        float bestDot = -2.0f;
        size_t bestIdx = 0u;
        bool found = false;

        for (size_t i = 0u; i < this->vertexCount; ++i)
        {
            // baseDirs[i] is the unit sphere direction for this vertex — always outward.
            const float coneDot = this->baseDirs[i].dotProduct(outwardDir);
            if (coneDot < cosHalfAngle)
            {
                continue;
            }

            // normals[i] is the face-averaged surface normal, pointing outward.
            // A flat surface has normal == baseDirs[i] (== outwardDir at this location).
            // The dot of the vertex normal with the ideal radial direction is 1.0 for
            // a perfectly flat surface, less for slopes.
            const float flatnessDot = this->normals[i].dotProduct(this->baseDirs[i]);
            if (flatnessDot > bestDot)
            {
                bestDot = flatnessDot;
                bestIdx = i;
                found = true;
            }
        }

        if (false == found)
        {
            return false;
        }

        outLocalPos = this->vertices[bestIdx];   // already at radius + heightData[i]
        outLocalNormal = this->normals[bestIdx]; // outward, accurate after recalculateNormals
        return true;
    }

    void PlanetTerra::collectSurfaceSamples(float slopeMaxDeg, float heightMinLocal, float heightMaxLocal, std::vector<SurfaceSample>& outSamples) const
    {
        collectSurfaceSamplesInCone(Ogre::Vector3::ZERO, // capDir ignored when halfAngle >= 180
            180.0f, slopeMaxDeg, heightMinLocal, heightMaxLocal, outSamples);
    }

    void PlanetTerra::collectSurfaceSamplesInCone(const Ogre::Vector3& capDir, float capHalfAngleDeg, float slopeMaxDeg, float heightMinLocal, float heightMaxLocal, std::vector<SurfaceSample>& outSamples) const
    {
        if (0u == this->vertexCount)
        {
            return;
        }

        const bool fullSphere = (capHalfAngleDeg >= 179.9f);
        const float cosHalfAngle = fullSphere ? -1.0f : Ogre::Math::Cos(Ogre::Degree(capHalfAngleDeg));
        const float cosSlopeMax = Ogre::Math::Cos(Ogre::Degree(Ogre::Math::Clamp(slopeMaxDeg, 0.0f, 180.0f)));

        for (int v = 0; v <= this->segmentsV; ++v)
        {
            for (int h = 0; h < this->segmentsH; ++h) // NOTE: < not <=, skip seam duplicate
            {
                const size_t i = static_cast<size_t>(v * (this->segmentsH + 1) + h);

                // ---- cone filter ----
                if (false == fullSphere)
                {
                    if (this->baseDirs[i].dotProduct(capDir) < cosHalfAngle)
                    {
                        continue;
                    }
                }

                // ---- height filter ----
                const float vertR = this->vertices[i].length();
                if (vertR < heightMinLocal || vertR > heightMaxLocal)
                {
                    continue;
                }

                // ---- slope filter ----
                // flatness = normal · baseDirs  (1 = flat, 0 = 90-deg slope, -1 = overhanging)
                const float flatness = this->normals[i].dotProduct(this->baseDirs[i]);
                if (flatness < cosSlopeMax)
                {
                    continue;
                }

                // ---- build sample ----
                SurfaceSample s;
                s.localPos = this->vertices[i];
                s.normal = this->normals[i];
                s.uv = this->uvCoords[i];
                s.slopeDeg = Ogre::Math::ACos(Ogre::Math::Clamp(flatness, -1.0f, 1.0f)).valueDegrees();

                outSamples.push_back(s);
            }
        }
    }

    bool PlanetTerra::sampleHeightAndNormalAtDirection(const Ogre::Vector3& dirWorld, Ogre::Vector3& outLocalPos, Ogre::Vector3& outLocalNormal) const
    {
        // Bilinearly interpolates heightData (and the corresponding surface
        // normal) at an arbitrary world direction, rather than snapping to
        // the nearest existing mesh vertex. This is used by foliage placement
        // (ProceduralFoliageVolumeComponent::calculatePlanetFoliagePositions)
        // when jittering instance positions within a mesh cell for higher-
        // than-vertex-resolution density -- without this, jittered points
        // were re-projected at the ORIGINAL sample vertex's radius rather
        // than the true displaced height at the jittered position, causing
        // foliage to visibly float above or sink below the actual terrain
        // surface on bumpy/hilly planets.
        //
        // This is a direct grid bilinear sample (cheap, O(1)) rather than the
        // O(vertexCount) linear scan in findFlatLandingVertex -- safe to call
        // many times per foliage generation pass.
        if (0u == this->vertexCount || this->segmentsH <= 0 || this->segmentsV <= 0)
        {
            return false;
        }

        Ogre::Vector3 dir = dirWorld;
        dir.normalise();

        // Invert the exact mapping used in generateBaseSphere:
        //   dir.y = cos(phi)                       -> phi   = acos(dir.y)
        //   dir.x = sin(phi)*cos(theta)
        //   dir.z = sin(phi)*sin(theta)             -> theta = atan2(dir.z, dir.x)
        const float phi = Ogre::Math::ACos(Ogre::Math::Clamp(dir.y, -1.0f, 1.0f)).valueRadians();
        float theta = Ogre::Math::ATan2(dir.z, dir.x).valueRadians();
        if (theta < 0.0f)
        {
            theta += Ogre::Math::TWO_PI;
        }

        // Continuous grid coordinates (fractional row/column).
        const float vf = (phi / Ogre::Math::PI) * static_cast<float>(this->segmentsV);
        const float hf = (theta / Ogre::Math::TWO_PI) * static_cast<float>(this->segmentsH);

        int v0 = static_cast<int>(std::floor(vf));
        int h0 = static_cast<int>(std::floor(hf));
        v0 = std::max(0, std::min(v0, this->segmentsV - 1));

        // Wrap horizontally (theta is cyclic); rows do not wrap (poles are hard edges).
        const int hWrapDivisor = this->segmentsH;
        int h0w = ((h0 % hWrapDivisor) + hWrapDivisor) % hWrapDivisor;
        int h1w = (h0w + 1) % hWrapDivisor;
        int v1 = std::min(v0 + 1, this->segmentsV);

        const float tV = Ogre::Math::Clamp(vf - static_cast<float>(v0), 0.0f, 1.0f);
        const float tH = Ogre::Math::Clamp(hf - static_cast<float>(h0), 0.0f, 1.0f);

        const size_t i00 = static_cast<size_t>(v0 * (this->segmentsH + 1) + h0w);
        const size_t i10 = static_cast<size_t>(v0 * (this->segmentsH + 1) + h1w);
        const size_t i01 = static_cast<size_t>(v1 * (this->segmentsH + 1) + h0w);
        const size_t i11 = static_cast<size_t>(v1 * (this->segmentsH + 1) + h1w);

        if (i00 >= this->vertexCount || i10 >= this->vertexCount || i01 >= this->vertexCount || i11 >= this->vertexCount)
        {
            return false;
        }

        // Bilinear blend of height displacement.
        const float h00 = this->heightData[i00];
        const float h10 = this->heightData[i10];
        const float h01 = this->heightData[i01];
        const float h11 = this->heightData[i11];
        const float hTop = Ogre::Math::lerp(h00, h10, tH);
        const float hBottom = Ogre::Math::lerp(h01, h11, tH);
        const float blendedHeight = Ogre::Math::lerp(hTop, hBottom, tV);

        // Bilinear blend of the surface normal (renormalised after blending --
        // linear blending of unit vectors is an approximation but adequate at
        // the sub-mesh-cell jitter distances this is used for). Component-wise
        // lerp is used explicitly rather than assuming a Vector3 overload of
        // Ogre::Math::lerp exists.
        const Ogre::Vector3& n00 = this->normals[i00];
        const Ogre::Vector3& n10 = this->normals[i10];
        const Ogre::Vector3& n01 = this->normals[i01];
        const Ogre::Vector3& n11 = this->normals[i11];
        Ogre::Vector3 nTop(Ogre::Math::lerp(n00.x, n10.x, tH), Ogre::Math::lerp(n00.y, n10.y, tH), Ogre::Math::lerp(n00.z, n10.z, tH));
        Ogre::Vector3 nBottom(Ogre::Math::lerp(n01.x, n11.x, tH), Ogre::Math::lerp(n01.y, n11.y, tH), Ogre::Math::lerp(n01.z, n11.z, tH));
        Ogre::Vector3 blendedNormal(Ogre::Math::lerp(nTop.x, nBottom.x, tV), Ogre::Math::lerp(nTop.y, nBottom.y, tV), Ogre::Math::lerp(nTop.z, nBottom.z, tV));
        if (blendedNormal.squaredLength() < 1e-12f)
        {
            blendedNormal = dir; // degenerate fallback
        }
        blendedNormal.normalise();

        // Reconstruct local position: base radius (sphere radius) + blended
        // height displacement, along the queried direction -- matching how
        // rebuildGeometryFromHeight places vertices (radius + heightData[i])
        // along baseDirs[i].
        outLocalPos = dir * (this->radius + blendedHeight);
        outLocalNormal = blendedNormal;
        return true;
    }

    void PlanetTerra::generateBaseSphere()
    {
        // UV sphere: (segmentsH+1) * (segmentsV+1) vertices
        // Row v=0 = north pole ring, v=segmentsV = south pole ring.
        vertexCount = static_cast<size_t>((segmentsH + 1) * (segmentsV + 1));
        indexCount = static_cast<size_t>(segmentsH) * static_cast<size_t>(segmentsV) * 6u;

        baseDirs.resize(vertexCount);
        uvCoords.resize(vertexCount);
        indices.resize(indexCount);
        heightData.assign(vertexCount, 0.0f);
        vertices.resize(vertexCount);
        normals.resize(vertexCount, Ogre::Vector3::UNIT_Y);
        tangents.resize(vertexCount, Ogre::Vector4(1.0f, 0.0f, 0.0f, 1.0f));

        const float PI = Ogre::Math::PI;
        const float TWO_PI = Ogre::Math::TWO_PI;

        size_t vIdx = 0u;
        for (int v = 0; v <= segmentsV; ++v)
        {
            const float phi = static_cast<float>(v) / static_cast<float>(segmentsV) * PI;
            for (int h = 0; h <= segmentsH; ++h)
            {
                const float theta = static_cast<float>(h) / static_cast<float>(segmentsH) * TWO_PI;

                Ogre::Vector3 dir;
                dir.x = Ogre::Math::Sin(phi) * Ogre::Math::Cos(theta);
                dir.y = Ogre::Math::Cos(phi);
                dir.z = Ogre::Math::Sin(phi) * Ogre::Math::Sin(theta);
                // Ensure unit length despite float imprecision at poles
                dir.normalise();

                baseDirs[vIdx] = dir;
                uvCoords[vIdx] = Ogre::Vector2(static_cast<float>(h) / static_cast<float>(segmentsH), static_cast<float>(v) / static_cast<float>(segmentsV));
                ++vIdx;
            }
        }

        // Indices — CCW winding for outward-facing normals.
        // Quad layout (viewed from outside the sphere):
        //   tl --- tr
        //   |  \   |
        //   bl --- br
        // CCW from outside: tl→tr→bl  and  tr→br→bl.
        // Proof via cross product: (tr-tl) × (bl-tl) points outward. ✓
        size_t iIdx = 0u;
        for (int v = 0; v < segmentsV; ++v)
        {
            for (int h = 0; h < segmentsH; ++h)
            {
                const Ogre::uint32 tl = static_cast<Ogre::uint32>(v * (segmentsH + 1) + h);
                const Ogre::uint32 tr = static_cast<Ogre::uint32>(v * (segmentsH + 1) + h + 1);
                const Ogre::uint32 bl = static_cast<Ogre::uint32>((v + 1) * (segmentsH + 1) + h);
                const Ogre::uint32 br = static_cast<Ogre::uint32>((v + 1) * (segmentsH + 1) + h + 1);

                // Upper triangle
                indices[iIdx++] = tl;
                indices[iIdx++] = tr;
                indices[iIdx++] = bl;

                // Lower triangle
                indices[iIdx++] = tr;
                indices[iIdx++] = br;
                indices[iIdx++] = bl;
            }
        }
    }

    // =========================================================================
    //  MAIN THREAD  —  geometry from heightData
    // =========================================================================

    void PlanetTerra::rebuildGeometryFromHeight()
    {
        computedMaxRadius = radius;
        for (size_t i = 0u; i < vertexCount; ++i)
        {
            const float r = radius + heightData[i];
            vertices[i] = baseDirs[i] * r;
            if (r > computedMaxRadius)
            {
                computedMaxRadius = r;
            }
        }
    }

    // =========================================================================
    //  MAIN THREAD  —  normals
    // =========================================================================

    void PlanetTerra::recalculateNormals()
    {
        for (auto& n : normals)
        {
            n = Ogre::Vector3::ZERO;
        }

        for (size_t i = 0u; i < indexCount; i += 3u)
        {
            const size_t i0 = indices[i], i1 = indices[i + 1], i2 = indices[i + 2];
            const Ogre::Vector3 fn = (vertices[i1] - vertices[i0]).crossProduct(vertices[i2] - vertices[i0]);
            normals[i0] += fn;
            normals[i1] += fn;
            normals[i2] += fn;
        }
        for (auto& n : normals)
        {
            n.normalise();
        }
    }

    // =========================================================================
    //  MAIN THREAD  —  tangents  (UV-based Gram–Schmidt)
    // =========================================================================

    void PlanetTerra::recalculateTangents()
    {
        std::vector<Ogre::Vector3> t1(vertexCount, Ogre::Vector3::ZERO);
        std::vector<Ogre::Vector3> t2(vertexCount, Ogre::Vector3::ZERO);

        for (size_t i = 0u; i < indexCount; i += 3u)
        {
            const size_t i0 = indices[i], i1 = indices[i + 1], i2 = indices[i + 2];
            const Ogre::Vector3& v0 = vertices[i0];
            const Ogre::Vector3& v1 = vertices[i1];
            const Ogre::Vector3& v2 = vertices[i2];
            const Ogre::Vector2& u0 = uvCoords[i0];
            const Ogre::Vector2& u1 = uvCoords[i1];
            const Ogre::Vector2& u2 = uvCoords[i2];

            const Ogre::Vector3 e1 = v1 - v0, e2 = v2 - v0;
            const float du1 = u1.x - u0.x, dv1 = u1.y - u0.y;
            const float du2 = u2.x - u0.x, dv2 = u2.y - u0.y;
            float d = du1 * dv2 - du2 * dv1;
            if (std::abs(d) < 1e-6f)
            {
                d = 1e-6f;
            }
            const float r = 1.0f / d;

            const Ogre::Vector3 s((dv2 * e1.x - dv1 * e2.x) * r, (dv2 * e1.y - dv1 * e2.y) * r, (dv2 * e1.z - dv1 * e2.z) * r);
            const Ogre::Vector3 tt((du1 * e2.x - du2 * e1.x) * r, (du1 * e2.y - du2 * e1.y) * r, (du1 * e2.z - du2 * e1.z) * r);

            t1[i0] += s;
            t1[i1] += s;
            t1[i2] += s;
            t2[i0] += tt;
            t2[i1] += tt;
            t2[i2] += tt;
        }

        for (size_t i = 0u; i < vertexCount; ++i)
        {
            const Ogre::Vector3& n = normals[i];
            Ogre::Vector3 tan = (t1[i] - n * n.dotProduct(t1[i])).normalisedCopy();
            const float hand = (n.crossProduct(t1[i]).dotProduct(t2[i]) < 0.0f) ? -1.0f : 1.0f;
            tangents[i] = {tan.x, tan.y, tan.z, hand};
        }
    }

    // =========================================================================
    //  MAIN THREAD  —  vertex adjacency + position groups
    // =========================================================================

    void PlanetTerra::buildVertexAdjacency()
    {
        vertexNeighbors.clear();
        vertexNeighbors.resize(vertexCount);

        std::vector<std::unordered_set<size_t>> sets(vertexCount);
        for (size_t i = 0u; i < indexCount; i += 3u)
        {
            const size_t i0 = indices[i], i1 = indices[i + 1], i2 = indices[i + 2];
            sets[i0].insert(i1);
            sets[i0].insert(i2);
            sets[i1].insert(i0);
            sets[i1].insert(i2);
            sets[i2].insert(i0);
            sets[i2].insert(i1);
        }
        for (size_t i = 0u; i < vertexCount; ++i)
        {
            vertexNeighbors[i].assign(sets[i].begin(), sets[i].end());
        }

        buildPositionGroups();
    }

    void PlanetTerra::buildPositionGroups()
    {
        positionGroups.clear();
        vertexGroup.assign(vertexCount, -1);

        constexpr float kEps2 = 1e-8f;

        for (size_t i = 0u; i < vertexCount; ++i)
        {
            if (vertexGroup[i] >= 0)
            {
                continue;
            }

            const int g = static_cast<int>(positionGroups.size());
            positionGroups.push_back({i});
            vertexGroup[i] = g;

            const Ogre::Vector3& pi = vertices[i];
            for (size_t j = i + 1u; j < vertexCount; ++j)
            {
                if (vertexGroup[j] >= 0)
                {
                    continue;
                }
                if (vertices[j].squaredDistance(pi) <= kEps2)
                {
                    positionGroups[g].push_back(j);
                    vertexGroup[j] = g;
                }
            }
        }
    }

    // =========================================================================
    //  MAIN THREAD  —  sculpting brush
    // =========================================================================

    float PlanetTerra::sampleBrushImage(const std::vector<float>& brushData, int brushW, int brushH, float ndx, float ndz) const
    {
        // ndx, ndz in [-1..1]  →  pixel coords
        const float u = (ndx + 1.0f) * 0.5f;
        const float v = (ndz + 1.0f) * 0.5f;
        const int px = std::max(0, std::min(brushW - 1, static_cast<int>(u * (brushW - 1) + 0.5f)));
        const int py = std::max(0, std::min(brushH - 1, static_cast<int>(v * (brushH - 1) + 0.5f)));
        return brushData[static_cast<size_t>(py) * brushW + px];
    }

    void PlanetTerra::applyBrush(const Ogre::Vector3& hitPosLocal, bool invert, BrushMode mode, const std::vector<float>& brushData, int brushW, int brushH, float brushSize, float brushIntensity, float brushFalloff)
    {
        if (vertexCount == 0u || brushSize <= 0.0f)
        {
            return;
        }

        // Precompute average height for SMOOTH and FLATTEN modes
        float avgHeight = 0.0f;
        if (mode == BrushMode::SMOOTH || mode == BrushMode::FLATTEN)
        {
            int cnt = 0;
            for (size_t i = 0u; i < vertexCount; ++i)
            {
                if (vertices[i].distance(hitPosLocal) < brushSize)
                {
                    avgHeight += heightData[i];
                    ++cnt;
                }
            }
            if (cnt > 0)
            {
                avgHeight /= static_cast<float>(cnt);
            }
        }

        // Compute a tangent basis for brush-image sampling (plane perp to hit radial dir)
        const Ogre::Vector3 hitDir = hitPosLocal.normalisedCopy();
        const Ogre::Vector3 tangentRef = (std::abs(hitDir.dotProduct(Ogre::Vector3::UNIT_Y)) < 0.9f) ? Ogre::Vector3::UNIT_Y : Ogre::Vector3::UNIT_X;
        const Ogre::Vector3 btan = hitDir.crossProduct(tangentRef).normalisedCopy();
        const Ogre::Vector3 bbit = hitDir.crossProduct(btan).normalisedCopy();

        const float direction = invert ? -1.0f : 1.0f;
        const float maxDisplace = radius * 0.5f; // clamp ±50% of radius

        // Processed mask prevents double-application to co-located groups
        std::vector<bool> processed(vertexCount, false);

        for (size_t i = 0u; i < vertexCount; ++i)
        {
            if (processed[i])
            {
                continue;
            }

            const float dist = vertices[i].distance(hitPosLocal);
            if (dist >= brushSize)
            {
                continue;
            }

            // Brush image sample
            float brushSample = 1.0f;
            if (!brushData.empty() && brushW > 0 && brushH > 0)
            {
                const Ogre::Vector3 delta = vertices[i] - hitPosLocal;
                const float ndx = delta.dotProduct(btan) / brushSize;
                const float ndz = delta.dotProduct(bbit) / brushSize;
                brushSample = sampleBrushImage(brushData, brushW, brushH, ndx, ndz);
            }

            const float nd = dist / brushSize;
            const float falloff = std::pow(1.0f - nd, brushFalloff);
            const float weight = brushSample * falloff * brushIntensity;

            // Compute delta height for this vertex
            float deltaH = 0.0f;
            switch (mode)
            {
            case BrushMode::PUSH:
                deltaH = -weight * direction;
                break;
            case BrushMode::PULL:
                deltaH = weight * direction;
                break;
            case BrushMode::SMOOTH:
            {
                float sum = heightData[i];
                size_t cnt = 1u;
                for (size_t nb : vertexNeighbors[i])
                {
                    sum += heightData[nb];
                    ++cnt;
                }
                const float target = sum / static_cast<float>(cnt);
                deltaH = (target - heightData[i]) * weight;
                break;
            }
            case BrushMode::FLATTEN:
                deltaH = (avgHeight - heightData[i]) * weight;
                break;
            case BrushMode::INFLATE:
                deltaH = weight * direction;
                break;
            }

            // Apply to this vertex AND all co-located group members
            const int g = vertexGroup[i];
            if (g >= 0 && g < static_cast<int>(positionGroups.size()))
            {
                for (size_t j : positionGroups[g])
                {
                    heightData[j] = std::max(-maxDisplace, std::min(maxDisplace, heightData[j] + deltaH));
                    vertices[j] = baseDirs[j] * (radius + heightData[j]);
                    processed[j] = true;
                }
            }
            else
            {
                heightData[i] = std::max(-maxDisplace, std::min(maxDisplace, heightData[i] + deltaH));
                vertices[i] = baseDirs[i] * (radius + heightData[i]);
                processed[i] = true;
            }
        }

        // Update computedMaxRadius for bound refresh in uploadVertexData
        computedMaxRadius = radius;
        for (size_t i = 0u; i < vertexCount; ++i)
        {
            const float r = radius + heightData[i];
            if (r > computedMaxRadius)
            {
                computedMaxRadius = r;
            }
        }
    }

    // =========================================================================
    //  MAIN THREAD  —  painting brush
    // =========================================================================

    void PlanetTerra::applyPaintBrush(const Ogre::Vector2& hitUV, bool invert, int layer, const std::vector<float>& brushData, int brushW, int brushH, float brushSize, float brushIntensity, float brushFalloff)
    {
        if (layer < 0 || layer > 3 || false == this->useWeightTexture)
        {
            return;
        }
        if (this->blendData.empty())
        {
            return;
        }

        const int cx = static_cast<int>(hitUV.x * blendTexSize);
        const int cy = static_cast<int>(hitUV.y * blendTexSize);
        const int bs = static_cast<int>(brushSize);
        const float direction = invert ? -1.0f : 1.0f;

        for (int py = cy - bs; py <= cy + bs; ++py)
        {
            if (py < 0 || py >= blendTexSize)
            {
                continue;
            }
            for (int px = cx - bs; px <= cx + bs; ++px)
            {
                if (px < 0 || px >= blendTexSize)
                {
                    continue;
                }

                const float dx = static_cast<float>(px - cx) / (brushSize + 1e-4f);
                const float dz = static_cast<float>(py - cy) / (brushSize + 1e-4f);
                const float dist = std::sqrt(dx * dx + dz * dz);
                if (dist > 1.0f)
                {
                    continue;
                }

                float brushSample = 1.0f;
                if (!brushData.empty() && brushW > 0 && brushH > 0)
                {
                    brushSample = sampleBrushImage(brushData, brushW, brushH, dx, dz);
                }

                const float falloff = std::pow(1.0f - dist, brushFalloff);
                const float weight = brushSample * falloff * brushIntensity * direction;

                const size_t idx = (static_cast<size_t>(py) * blendTexSize + static_cast<size_t>(px)) * 4u;

                float channels[4];
                for (int c = 0; c < 4; ++c)
                {
                    channels[c] = static_cast<float>(this->blendData[idx + c]) / 255.0f;
                }

                channels[layer] = std::max(0.0f, std::min(1.0f, channels[layer] + weight));

                // Renormalize so all layers sum to 1
                float sum = 0.0f;
                for (int c = 0; c < 4; ++c)
                {
                    sum += channels[c];
                }
                if (sum > 1e-5f)
                {
                    for (int c = 0; c < 4; ++c)
                    {
                        channels[c] /= sum;
                    }
                }
                else
                {
                    channels[0] = 1.0f; // safety: fall back to layer 0
                }

                for (int c = 0; c < 4; ++c)
                {
                    this->blendData[idx + c] = static_cast<uint8_t>(std::max(0.0f, std::min(1.0f, channels[c])) * 255.0f + 0.5f);
                }
            }
        }
    }

    // =========================================================================
    //  MAIN THREAD  —  snapshot restore
    // =========================================================================

    void PlanetTerra::restoreHeightData(const std::vector<float>& data)
    {
        if (data.size() != vertexCount)
        {
            return;
        }
        heightData = data;
        rebuildGeometryFromHeight();
        recalculateNormals();
        recalculateTangents();
    }

    void PlanetTerra::restoreBlendData(const std::vector<uint8_t>& data)
    {
        if (data.size() != blendData.size())
        {
            return;
        }
        this->blendData = data;
        // Upload to GPU immediately so the visual matches the restored state
        if (true == this->useWeightTexture)
        {
            this->uploadBlendData();
        }
    }

} // namespace NOWA
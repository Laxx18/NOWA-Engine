#include "NOWAPrecompiled.h"
#include "PlanetOcean.h"

#include "main/Core.h"

#include "OgreHlmsManager.h"
#include "OgreHlmsPbs.h"
#include "OgreHlmsPbsDatablock.h"
#include "OgreItem.h"
#include "OgreLogManager.h"
#include "OgreMesh2.h"
#include "OgreMeshManager2.h"
#include "OgreRoot.h"
#include "OgreSceneManager.h"
#include "OgreSceneNode.h"
#include "OgreStagingTexture.h"
#include "OgreSubMesh2.h"
#include "OgreTextureBox.h"
#include "OgreTextureGpuManager.h"
#include "Vao/OgreIndexBufferPacked.h"
#include "Vao/OgreVaoManager.h"
#include "Vao/OgreVertexArrayObject.h"
#include "Vao/OgreVertexBufferPacked.h"

#include <cmath>

namespace NOWA
{
    // =========================================================================
    //  Constructor / Destructor
    // =========================================================================

    PlanetOcean::PlanetOcean(const Ogre::String& objectName, Ogre::SceneManager* sceneManager) :
        objectName(objectName),
        sceneManager(sceneManager),
        attachedNode(nullptr),
        oceanItem(nullptr),
        vertexBuffer(nullptr),
        noiseTexture(nullptr),
        normalMapWeight(0.9f),
        vertexCount(0u),
        indexCount(0u),
        oceanTime(0.0f),
        waveAmplitudeScale(1.0f),
        radius(52.0f),
        segmentsH(32),
        segmentsV(32)
    {
        // Wave 0 -- primary long swell
        this->waves[0].directionAngle = 0.0f;
        this->waves[0].amplitude = 0.08f;
        this->waves[0].frequency = 2.0f;
        this->waves[0].speed = 0.3f;
        this->waves[0].steepness = 0.5f;

        // Wave 1 -- secondary cross swell
        this->waves[1].directionAngle = 1.2f;
        this->waves[1].amplitude = 0.05f;
        this->waves[1].frequency = 3.0f;
        this->waves[1].speed = 0.2f;
        this->waves[1].steepness = 0.4f;

        // Wave 2 -- short chop
        this->waves[2].directionAngle = 2.7f;
        this->waves[2].amplitude = 0.03f;
        this->waves[2].frequency = 5.0f;
        this->waves[2].speed = 0.4f;
        this->waves[2].steepness = 0.3f;

        // Wave 3 -- fine ripple
        this->waves[3].directionAngle = 4.3f;
        this->waves[3].amplitude = 0.02f;
        this->waves[3].frequency = 8.0f;
        this->waves[3].speed = 0.6f;
        this->waves[3].steepness = 0.2f;
    }

    PlanetOcean::~PlanetOcean()
    {
        this->destroy();
    }

    // =========================================================================
    //  Lifecycle
    // =========================================================================

    bool PlanetOcean::create(float radius, int segmentsH, int segmentsV, Ogre::SceneNode* attachNode, const Ogre::String& datablockName)
    {
        this->radius = radius;
        this->segmentsH = segmentsH;
        this->segmentsV = segmentsV;
        this->attachedNode = attachNode;

        this->generateBaseSphere();

        const Ogre::String meshName = this->objectName + "_OceanMesh";
        this->oceanItem = this->buildItem(meshName);

        if (nullptr == this->oceanItem)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PlanetOcean] Failed to build item for '" + this->objectName + "'.");
            return false;
        }

        this->setDatablockByName(datablockName);
        this->initNormalMapTexture();
        this->attachedNode->attachObject(this->oceanItem);

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PlanetOcean] Created ocean '" + this->objectName + "' radius=" + Ogre::StringConverter::toString(this->radius) +
                                                                               " segH=" + Ogre::StringConverter::toString(this->segmentsH) + " segV=" + Ogre::StringConverter::toString(this->segmentsV) +
                                                                               " verts=" + Ogre::StringConverter::toString(static_cast<unsigned int>(this->vertexCount)));

        return true;
    }

    void PlanetOcean::destroy()
    {
        if (nullptr != this->oceanItem)
        {
            if (nullptr != this->attachedNode)
            {
                this->attachedNode->detachObject(this->oceanItem);
            }
            this->sceneManager->destroyItem(this->oceanItem);
            this->oceanItem = nullptr;
        }

        if (this->oceanMesh)
        {
            Ogre::MeshManager::getSingleton().remove(this->oceanMesh->getHandle());
            this->oceanMesh.reset();
        }

        // vertexBuffer is owned by the VAO, destroyed with the mesh above.
        this->vertexBuffer = nullptr;

        if (nullptr != this->noiseTexture)
        {
            Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager()->destroyTexture(this->noiseTexture);
            this->noiseTexture = nullptr;
        }
    }

    void PlanetOcean::update(float deltaTime)
    {
        this->oceanTime += deltaTime;
        this->updateAndUpload();
        this->updateNormalMap();
    }

    // =========================================================================
    //  Material helpers
    // =========================================================================

    void PlanetOcean::setDatablockByName(const Ogre::String& name)
    {
        if (nullptr == this->oceanItem || name.empty())
        {
            return;
        }

        Ogre::HlmsDatablock* db = Ogre::Root::getSingleton().getHlmsManager()->getDatablockNoDefault(name);

        if (nullptr != db)
        {
            this->oceanItem->getSubItem(0)->setDatablock(db);
        }
        else
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PlanetOcean] Datablock '" + name + "' not found.");
        }
    }

    void PlanetOcean::setDeepColour(const Ogre::Vector3& colour)
    {
        if (nullptr == this->oceanItem)
        {
            return;
        }
        Ogre::HlmsPbsDatablock* db = dynamic_cast<Ogre::HlmsPbsDatablock*>(this->oceanItem->getSubItem(0)->getDatablock());
        if (nullptr != db)
        {
            db->setDiffuse(colour);
        }
    }

    void PlanetOcean::setShallowColour(const Ogre::Vector3& colour)
    {
        if (nullptr == this->oceanItem)
        {
            return;
        }
        Ogre::HlmsPbsDatablock* db = dynamic_cast<Ogre::HlmsPbsDatablock*>(this->oceanItem->getSubItem(0)->getDatablock());
        if (nullptr != db)
        {
            db->setEmissive(colour);
        }
    }

    void PlanetOcean::setRoughness(float roughness)
    {
        if (nullptr == this->oceanItem)
        {
            return;
        }
        Ogre::HlmsPbsDatablock* db = dynamic_cast<Ogre::HlmsPbsDatablock*>(this->oceanItem->getSubItem(0)->getDatablock());
        if (nullptr != db)
        {
            db->setRoughness(roughness);
            // Re-apply normalMapWeight: setRoughness may trigger HLMS recompile
            // and the component calls setRoughness AFTER create() sets the normal map,
            // so we must ensure normalMapWeight is always re-applied here.
            db->setNormalMapWeight(this->normalMapWeight);
        }
    }

    void PlanetOcean::setTransparency(float transparency)
    {
        if (nullptr == this->oceanItem)
        {
            return;
        }
        Ogre::HlmsPbsDatablock* db = dynamic_cast<Ogre::HlmsPbsDatablock*>(this->oceanItem->getSubItem(0)->getDatablock());
        if (nullptr != db)
        {
            db->setTransparency(transparency, Ogre::HlmsPbsDatablock::Transparent, true);
        }
    }

    void PlanetOcean::setReflectionTextureName(const Ogre::String& cubemapName)
    {
        if (nullptr == this->oceanItem || cubemapName.empty())
        {
            return;
        }
        Ogre::HlmsPbsDatablock* db = dynamic_cast<Ogre::HlmsPbsDatablock*>(this->oceanItem->getSubItem(0)->getDatablock());
        if (nullptr == db)
        {
            return;
        }
        Ogre::TextureGpuManager* texMgr = Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();
        Ogre::TextureGpu* cubemap = texMgr->createOrRetrieveTexture(cubemapName, Ogre::GpuPageOutStrategy::SaveToSystemRam, 0u, Ogre::TextureTypes::TypeCube, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
            Ogre::TextureFilter::FilterTypes::TypeGenerateDefaultMipmaps, 0u);
        if (nullptr != cubemap)
        {
            cubemap->scheduleTransitionTo(Ogre::GpuResidency::Resident);
            Ogre::HlmsSamplerblock samplerRef;
            samplerRef.mU = Ogre::TAM_WRAP;
            samplerRef.mV = Ogre::TAM_WRAP;
            samplerRef.mW = Ogre::TAM_WRAP;
            db->setTexture(Ogre::PBSM_REFLECTION, cubemap, &samplerRef);
        }
    }

    // =========================================================================
    //  initNormalMapTexture -- called ONCE from create()
    //  Creates the 64x64 GPU texture, sets it on PBSM_NORMAL, and does the first fill.
    // =========================================================================

    void PlanetOcean::initNormalMapTexture()
    {
        if (nullptr == this->oceanItem)
        {
            return;
        }
        Ogre::HlmsPbsDatablock* db = dynamic_cast<Ogre::HlmsPbsDatablock*>(this->oceanItem->getSubItem(0)->getDatablock());
        if (nullptr == db)
        {
            return;
        }

        const Ogre::uint32 texSize = 128u;

        Ogre::TextureGpuManager* texMgr = this->sceneManager->getDestinationRenderSystem()->getTextureGpuManager();

        if (nullptr != this->noiseTexture)
        {
            texMgr->destroyTexture(this->noiseTexture);
            this->noiseTexture = nullptr;
        }

        // Static counter: incremented globally so each initNormalMapTexture() call
        // gets a name that has NEVER been used before, even across destroy+create cycles.
        // destroyTexture() in Ogre-Next defers name removal to end-of-frame, so a member
        // counter that resets on new PlanetOcean() always collides on the second create.
        static Ogre::uint32 s_noiseTexId = 0u;
        ++s_noiseTexId;
        const Ogre::String texName = this->objectName + "_WaterNoise_" + Ogre::StringConverter::toString(s_noiseTexId);

        this->noiseTexture = texMgr->createTexture(texName, Ogre::GpuPageOutStrategy::Discard, 0u, Ogre::TextureTypes::Type2D);

        this->noiseTexture->setResolution(texSize, texSize);
        this->noiseTexture->setNumMipmaps(1u);
        this->noiseTexture->setPixelFormat(Ogre::PFG_RGBA8_UNORM);
        this->noiseTexture->_transitionTo(Ogre::GpuResidency::Resident, static_cast<Ogre::uint8*>(nullptr));
        this->noiseTexture->_setNextResidencyStatus(Ogre::GpuResidency::Resident);

        // Bind to datablock immediately so PBS compiles with the normal map.
        Ogre::HlmsSamplerblock samplerRef;
        samplerRef.mU = Ogre::TAM_WRAP;
        samplerRef.mV = Ogre::TAM_WRAP;
        samplerRef.mW = Ogre::TAM_WRAP;
        samplerRef.mMinFilter = Ogre::FO_LINEAR;
        samplerRef.mMagFilter = Ogre::FO_LINEAR;
        samplerRef.mMipFilter = Ogre::FO_LINEAR;
        db->setTexture(Ogre::PBSM_NORMAL, this->noiseTexture, &samplerRef);
        // normalMapWeight stored as member so setRoughness() can re-apply it
        // after the component's Variant value overrides whatever we set here.
        db->setNormalMapWeight(this->normalMapWeight);

        // Fill with t=0 data immediately.
        this->updateNormalMap();

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PlanetOcean] Water normal map texture created: " + texName + " (" + Ogre::StringConverter::toString(texSize) + "x" + Ogre::StringConverter::toString(texSize) + ").");
    }

    // =========================================================================
    //  updateNormalMap -- called EVERY frame from update()
    //
    //  Regenerates the 64x64 normal map with time-varying phase offsets so the
    //  water surface appears to animate. Cost: 64*64*8 = 32768 sin() + 4096
    //  normalize() calls ≈ 0.3ms on the render thread. Negligible.
    //
    //  Frequencies are scaled by radius/50 so physical wave spacing is the same
    //  on any planet size:
    //    On r=50:  freq=6 → 314m/6 = 52m  (large swell)
    //              freq=24 → 314m/24 = 13m (medium chop)
    //              freq=48 → 314m/48 = 6.5m (fine ripple)
    //    On r=500: same physical spacing, more UV cycles to compensate
    // =========================================================================

    void PlanetOcean::updateNormalMap()
    {
        if (nullptr == this->noiseTexture || nullptr == this->oceanItem)
        {
            return;
        }

        const Ogre::uint32 texSize = 128u;
        const float TWO_PI = Ogre::Math::TWO_PI;

        // amp * freq * 2PI = steepness (gradient contribution per wave).
        // Old code had amp=0.10, freq=85 -> gradient = 0.10 * 85 * 6.28 = 53 per wave.
        // 8 waves * 53 = 424 max -> normalize(-424, 0, 1) -> normals horizontal -> giant blobs.
        // Fix: store steepness directly. c = steepness * cos(phase).
        // Total max gradient = sum of steepnesses = 0.37 -> tilt = atan(0.37) = 20 degrees.
        struct NMWave
        {
            float angle;
            float freqPhys;
            float steepness;
            float speed;
        };
        const NMWave nmWaves[8] = {
            {0.000f, 85.0f, 0.050f, 0.50f},  // East
            {0.785f, 140.0f, 0.040f, 0.70f}, // NE
            {1.571f, 110.0f, 0.045f, 0.60f}, // North
            {2.356f, 170.0f, 0.035f, 0.80f}, // NW
            {3.142f, 95.0f, 0.048f, 0.55f},  // West
            {3.927f, 155.0f, 0.038f, 0.75f}, // SW
            {4.712f, 125.0f, 0.042f, 0.90f}, // South
            {5.498f, 190.0f, 0.032f, 0.65f}  // SE
        };

        // Scale frequency by radius so physical wave spacing stays constant.
        const float radiusScale = this->radius / 50.0f;
        const float t = this->oceanTime;

        // 128x128x4 = 65536 bytes -- use vector to avoid stack overflow.
        std::vector<Ogre::uint8> pixels(static_cast<size_t>(texSize) * texSize * 4u);

        for (Ogre::uint32 py = 0u; py < texSize; ++py)
        {
            for (Ogre::uint32 px = 0u; px < texSize; ++px)
            {
                float u = (static_cast<float>(px) + 0.5f) / static_cast<float>(texSize);
                float v = (static_cast<float>(py) + 0.5f) / static_cast<float>(texSize);

                float dhdu = 0.0f;
                float dhdv = 0.0f;

                for (int w = 0; w < 8; ++w)
                {
                    float cx = std::cos(nmWaves[w].angle);
                    float sy = std::sin(nmWaves[w].angle);
                    float freq = nmWaves[w].freqPhys * radiusScale;
                    float phase = (cx * u + sy * v) * freq * TWO_PI - nmWaves[w].speed * t + nmWaves[w].angle;
                    // c = steepness * cos(phase) -- bounded by steepness, freq-independent.
                    float c = nmWaves[w].steepness * std::cos(phase);
                    dhdu += c * cx;
                    dhdv += c * sy;
                }

                float nx = -dhdu;
                float ny = -dhdv;
                float nz = 1.0f;
                float invLen = 1.0f / std::sqrt(nx * nx + ny * ny + nz * nz);
                nx *= invLen;
                ny *= invLen;
                nz *= invLen;

                Ogre::uint32 idx = (py * texSize + px) * 4u;
                pixels[idx + 0u] = static_cast<Ogre::uint8>((nx + 1.0f) * 127.5f);
                pixels[idx + 1u] = static_cast<Ogre::uint8>((ny + 1.0f) * 127.5f);
                pixels[idx + 2u] = static_cast<Ogre::uint8>((nz + 1.0f) * 127.5f);
                pixels[idx + 3u] = 255u;
            }
        }

        // Upload via staging texture.
        Ogre::TextureGpuManager* texMgr = this->sceneManager->getDestinationRenderSystem()->getTextureGpuManager();

        Ogre::StagingTexture* stagingTex = texMgr->getStagingTexture(texSize, texSize, 1u, 1u, Ogre::PFG_RGBA8_UNORM);
        stagingTex->startMapRegion();
        Ogre::TextureBox texBox = stagingTex->mapRegion(texSize, texSize, 1u, 1u, Ogre::PFG_RGBA8_UNORM);

        for (Ogre::uint32 j = 0u; j < texSize; ++j)
        {
            for (Ogre::uint32 i = 0u; i < texSize; ++i)
            {
                Ogre::uint8* dst = reinterpret_cast<Ogre::uint8*>(texBox.at(i, j, 0u));
                const Ogre::uint32 src = (j * texSize + i) * 4u;
                dst[0] = pixels[src + 0u];
                dst[1] = pixels[src + 1u];
                dst[2] = pixels[src + 2u];
                dst[3] = pixels[src + 3u];
            }
        }

        stagingTex->stopMapRegion();
        stagingTex->upload(texBox, this->noiseTexture, 0u, nullptr, nullptr, false);
        texMgr->removeStagingTexture(stagingTex);
        stagingTex = nullptr;
        this->noiseTexture->notifyDataIsReady();
    }

    void PlanetOcean::setWave(int index, const WaveParams& params)
    {
        if (index < 0 || index >= static_cast<int>(this->waves.size()))
        {
            return;
        }
        this->waves[static_cast<size_t>(index)] = params;
    }

    PlanetOcean::WaveParams PlanetOcean::getWave(int index) const
    {
        WaveParams defaults;
        defaults.directionAngle = 0.0f;
        defaults.amplitude = 0.0f;
        defaults.frequency = 2.0f;
        defaults.speed = 0.3f;
        defaults.steepness = 0.5f;

        if (index < 0 || index >= static_cast<int>(this->waves.size()))
        {
            return defaults;
        }
        return this->waves[static_cast<size_t>(index)];
    }

    int PlanetOcean::getWaveCount()
    {
        return 4;
    }

    void PlanetOcean::setWaveAmplitudeScale(float scale)
    {
        this->waveAmplitudeScale = scale;
    }

    float PlanetOcean::getWaveAmplitudeScale() const
    {
        return this->waveAmplitudeScale;
    }

    // =========================================================================
    //  Accessors
    // =========================================================================

    Ogre::Item* PlanetOcean::getItem() const
    {
        return this->oceanItem;
    }

    Ogre::SceneNode* PlanetOcean::getAttachedNode() const
    {
        return this->attachedNode;
    }

    Ogre::SceneManager* PlanetOcean::getSceneManager() const
    {
        return this->sceneManager;
    }

    float PlanetOcean::getRadius() const
    {
        return this->radius;
    }

    float PlanetOcean::getOceanTime() const
    {
        return this->oceanTime;
    }

    // =========================================================================
    //  Geometry generation (called once at create())
    // =========================================================================

    void PlanetOcean::generateBaseSphere()
    {
        this->vertexCount = static_cast<size_t>((this->segmentsH + 1) * (this->segmentsV + 1));
        this->indexCount = static_cast<size_t>(this->segmentsH) * static_cast<size_t>(this->segmentsV) * 6u;

        this->baseDirs.resize(this->vertexCount);
        this->uvCoords.resize(this->vertexCount);
        this->tangents.resize(this->vertexCount);
        this->indices.resize(this->indexCount);

        const float PI = Ogre::Math::PI;
        const float TWO_PI = Ogre::Math::TWO_PI;

        // Vertices.
        size_t vIdx = 0u;
        for (int v = 0; v <= this->segmentsV; ++v)
        {
            const float phi = static_cast<float>(v) / static_cast<float>(this->segmentsV) * PI;
            for (int h = 0; h <= this->segmentsH; ++h)
            {
                const float theta = static_cast<float>(h) / static_cast<float>(this->segmentsH) * TWO_PI;
                const float sinPhi = std::sin(phi);
                const float cosPhi = std::cos(phi);

                Ogre::Vector3 dir(sinPhi * std::cos(theta), cosPhi, sinPhi * std::sin(theta));
                this->baseDirs[vIdx] = dir;
                this->uvCoords[vIdx] = Ogre::Vector2(static_cast<float>(h) / static_cast<float>(this->segmentsH), static_cast<float>(v) / static_cast<float>(this->segmentsV));

                // Precompute tangent from UV longitude direction -- constant for UV sphere.
                // T = d(pos)/d(theta) normalised = (-sin(theta), 0, cos(theta)) projected to surface.
                Ogre::Vector3 T(-std::sin(theta), 0.0f, std::cos(theta));
                // At poles switch to a safe fallback.
                if (sinPhi < 0.01f)
                {
                    T = Ogre::Vector3(1.0f, 0.0f, 0.0f);
                }
                T.normalise();
                float w = (this->uvCoords[vIdx].x > 0.5f) ? -1.0f : 1.0f;
                this->tangents[vIdx] = Ogre::Vector4(T.x, T.y, T.z, w);

                ++vIdx;
            }
        }

        // Indices -- CCW winding for outward normals.
        size_t iIdx = 0u;
        for (int v = 0; v < this->segmentsV; ++v)
        {
            for (int h = 0; h < this->segmentsH; ++h)
            {
                const uint32_t tl = static_cast<uint32_t>(v * (this->segmentsH + 1) + h);
                const uint32_t tr = static_cast<uint32_t>(v * (this->segmentsH + 1) + h + 1);
                const uint32_t bl = static_cast<uint32_t>((v + 1) * (this->segmentsH + 1) + h);
                const uint32_t br = static_cast<uint32_t>((v + 1) * (this->segmentsH + 1) + h + 1);
                this->indices[iIdx++] = tl;
                this->indices[iIdx++] = bl;
                this->indices[iIdx++] = tr;
                this->indices[iIdx++] = tr;
                this->indices[iIdx++] = bl;
                this->indices[iIdx++] = br;
            }
        }
    }

    Ogre::Item* PlanetOcean::buildItem(const Ogre::String& meshName)
    {
        Ogre::VaoManager* vaoManager = this->sceneManager->getDestinationRenderSystem()->getVaoManager();

        // Standard 12-float vertex layout.
        Ogre::VertexElement2Vec elems;
        elems.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_POSITION));
        elems.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_NORMAL));
        elems.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT4, Ogre::VES_TANGENT));
        elems.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES));

        constexpr size_t fpv = 12u;

        // Pack initial frame: position = baseDirs * radius, normal = baseDirs.
        float* vd = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(this->vertexCount * fpv * sizeof(float), Ogre::MEMCATEGORY_GEOMETRY));

        for (size_t i = 0u; i < this->vertexCount; ++i)
        {
            const Ogre::Vector3& d = this->baseDirs[i];
            const Ogre::Vector4& tg = this->tangents[i];
            const Ogre::Vector2& uv = this->uvCoords[i];
            size_t o = i * fpv;
            vd[o++] = d.x * this->radius;
            vd[o++] = d.y * this->radius;
            vd[o++] = d.z * this->radius;
            vd[o++] = d.x; // normal = unit sphere dir
            vd[o++] = d.y;
            vd[o++] = d.z;
            vd[o++] = tg.x;
            vd[o++] = tg.y;
            vd[o++] = tg.z;
            vd[o++] = tg.w;
            vd[o++] = uv.x;
            vd[o] = uv.y;
        }

        this->vertexBuffer = vaoManager->createVertexBuffer(elems, this->vertexCount, Ogre::BT_DYNAMIC_DEFAULT, vd, false);

        OGRE_FREE_SIMD(vd, Ogre::MEMCATEGORY_GEOMETRY);

        // Index buffer.
        uint32_t* id = reinterpret_cast<uint32_t*>(OGRE_MALLOC_SIMD(this->indexCount * sizeof(uint32_t), Ogre::MEMCATEGORY_GEOMETRY));
        for (size_t i = 0u; i < this->indexCount; ++i)
        {
            id[i] = this->indices[i];
        }
        Ogre::IndexBufferPacked* indexBuffer = vaoManager->createIndexBuffer(Ogre::IndexBufferPacked::IT_32BIT, this->indexCount, Ogre::BT_IMMUTABLE, id, true);

        Ogre::VertexBufferPackedVec vertexBuffers;
        vertexBuffers.push_back(this->vertexBuffer);

        Ogre::VertexArrayObject* vao = vaoManager->createVertexArrayObject(vertexBuffers, indexBuffer, Ogre::OT_TRIANGLE_LIST);

        Ogre::MeshPtr mesh = Ogre::MeshManager::getSingleton().createManual(meshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

        Ogre::SubMesh* subMesh = mesh->createSubMesh();
        subMesh->mVao[Ogre::VpNormal].push_back(vao);
        subMesh->mVao[Ogre::VpShadow].push_back(vao);

        // Bounds: add max possible wave height (all 4 waves at peak * radiusScale).
        float maxWaveH = 0.0f;
        for (int w = 0; w < 4; ++w)
        {
            maxWaveH += this->waves[static_cast<size_t>(w)].amplitude;
        }
        maxWaveH = maxWaveH * this->waveAmplitudeScale * (this->radius / 50.0f) + 2.0f;

        const float boundsRadius = this->radius + maxWaveH;
        mesh->_setBounds(Ogre::Aabb(Ogre::Vector3::ZERO, Ogre::Vector3(boundsRadius)), false);
        mesh->_setBoundingSphereRadius(boundsRadius);

        this->oceanMesh = mesh;
        return this->sceneManager->createItem(mesh, Ogre::SCENE_DYNAMIC);
    }

    // =========================================================================
    //  updateAndUpload -- the only hot-path function, called every render frame
    //
    //  Single pass over all vertices:
    //    1. Evaluate 4 Gerstner waves (4 sin() per vertex, constants pre-hoisted)
    //    2. Compute normal as normalize(displacedPos) -- exact for a sphere
    //    3. Pack pos + normal + tangent(precomputed) + uv into SIMD staging buffer
    //    4. Upload to BT_DYNAMIC_DEFAULT vertex buffer
    // =========================================================================

    void PlanetOcean::updateAndUpload()
    {
        if (nullptr == this->vertexBuffer)
        {
            return;
        }

        const float t = this->oceanTime;
        const float radiusScale = this->radius / 50.0f;
        const float globalScale = this->waveAmplitudeScale * radiusScale;

        // Hoist all per-wave constants out of the vertex loop.
        float wCos[4];
        float wSin[4];
        float wAmp[4];
        float wFreq[4];
        float wPhaseOffset[4];
        for (int w = 0; w < 4; ++w)
        {
            const WaveParams& wv = this->waves[static_cast<size_t>(w)];
            wCos[w] = std::cos(wv.directionAngle);
            wSin[w] = std::sin(wv.directionAngle);
            wAmp[w] = wv.amplitude * globalScale;
            wFreq[w] = wv.frequency * Ogre::Math::TWO_PI;
            wPhaseOffset[w] = wv.speed * t;
        }

        constexpr size_t fpv = 12u;
        float* vd = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(this->vertexCount * fpv * sizeof(float), Ogre::MEMCATEGORY_GEOMETRY));

        for (size_t i = 0u; i < this->vertexCount; ++i)
        {
            const Ogre::Vector3& dir = this->baseDirs[i];

            // Wave displacement.
            float h = 0.0f;
            for (int w = 0; w < 4; ++w)
            {
                float dot = dir.x * wCos[w] + dir.z * wSin[w];
                float phase = wFreq[w] * dot - wPhaseOffset[w];
                h += wAmp[w] * std::sin(phase);
            }

            // Displaced position.
            float r = this->radius + h;
            float px = dir.x * r;
            float py = dir.y * r;
            float pz = dir.z * r;

            // Normal = normalize(displacedPos).
            // Exact for a sphere; negligible error for small wave amplitudes.
            float invLen = 1.0f / std::sqrt(px * px + py * py + pz * pz);
            float nx = px * invLen;
            float ny = py * invLen;
            float nz = pz * invLen;

            // Pack.
            const Ogre::Vector4& tg = this->tangents[i];
            const Ogre::Vector2& uv = this->uvCoords[i];
            size_t o = i * fpv;
            vd[o++] = px;
            vd[o++] = py;
            vd[o++] = pz;
            vd[o++] = nx;
            vd[o++] = ny;
            vd[o++] = nz;
            vd[o++] = tg.x;
            vd[o++] = tg.y;
            vd[o++] = tg.z;
            vd[o++] = tg.w;
            vd[o++] = uv.x;
            vd[o] = uv.y;
        }

        this->vertexBuffer->upload(vd, 0u, this->vertexCount);
        OGRE_FREE_SIMD(vd, Ogre::MEMCATEGORY_GEOMETRY);
    }

} // namespace NOWA
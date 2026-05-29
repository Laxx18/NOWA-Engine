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
#include "OgreSubMesh2.h"
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
        vertexCount(0u),
        indexCount(0u),
        oceanTime(0.0f),
        waveAmplitudeScale(1.0f),
        radius(50.0f),
        segmentsH(64),
        segmentsV(64)
    {
        // Wave 0 - primary long swell
        this->waves[0].directionAngle = 0.0f;
        this->waves[0].amplitude = 0.4f;
        this->waves[0].frequency = 0.25f;
        this->waves[0].speed = 1.2f;
        this->waves[0].steepness = 0.5f;

        // Wave 1 - secondary cross swell
        this->waves[1].directionAngle = 1.2f;
        this->waves[1].amplitude = 0.25f;
        this->waves[1].frequency = 0.4f;
        this->waves[1].speed = 0.9f;
        this->waves[1].steepness = 0.4f;

        // Wave 2 - short chop
        this->waves[2].directionAngle = 2.7f;
        this->waves[2].amplitude = 0.15f;
        this->waves[2].frequency = 0.6f;
        this->waves[2].speed = 1.6f;
        this->waves[2].steepness = 0.3f;

        // Wave 3 - fine ripple
        this->waves[3].directionAngle = 4.3f;
        this->waves[3].amplitude = 0.1f;
        this->waves[3].frequency = 0.9f;
        this->waves[3].speed = 2.0f;
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

        this->vertices = this->basePositions;
        this->normals.resize(this->vertexCount, Ogre::Vector3::UNIT_Y);
        this->tangents.resize(this->vertexCount, Ogre::Vector4(1.0f, 0.0f, 0.0f, 1.0f));

        this->recalculateNormals();
        this->recalculateTangents();

        const Ogre::String meshName = this->objectName + "_OceanMesh";
        this->oceanItem = this->buildItemFromCPUArrays(meshName);

        if (nullptr == this->oceanItem)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PlanetOcean] Failed to build item for '" + this->objectName + "'.");
            return false;
        }

        this->setDatablockByName(datablockName);
        this->attachedNode->attachObject(this->oceanItem);

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PlanetOcean] Created ocean sphere '" + this->objectName + "' radius=" + Ogre::StringConverter::toString(this->radius) +
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

        // vertexBuffer is owned by the VAO which is destroyed with the mesh.
        this->vertexBuffer = nullptr;
    }

    void PlanetOcean::update(float deltaTime)
    {
        this->oceanTime += deltaTime;
        this->applyWaves();
        this->recalculateNormals();
        this->recalculateTangents();
        this->uploadVertexData();
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
            // Shallow colour stored as emissive tint, blended at wave crests in material.
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

        Ogre::TextureGpu* cubemap = texMgr->createOrRetrieveTexture(cubemapName, Ogre::GpuPageOutStrategy::SaveToSystemRam, Ogre::TextureFlags::PrefersLoadingFromFileAsSRGB, Ogre::TextureTypes::TypeCube,
            Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME, Ogre::TextureFilter::FilterTypes::TypeGenerateDefaultMipmaps, 0u);

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
    //  Wave configuration
    // =========================================================================

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
        defaults.frequency = 0.3f;
        defaults.speed = 1.0f;
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
    //  Geometry generation
    // =========================================================================

    void PlanetOcean::generateBaseSphere()
    {
        this->vertexCount = static_cast<size_t>((this->segmentsH + 1) * (this->segmentsV + 1));
        this->indexCount = static_cast<size_t>(this->segmentsH) * static_cast<size_t>(this->segmentsV) * 6u;

        this->baseDirs.resize(this->vertexCount);
        this->basePositions.resize(this->vertexCount);
        this->uvCoords.resize(this->vertexCount);
        this->indices.resize(this->indexCount);

        const float PI = Ogre::Math::PI;
        const float TWO_PI = Ogre::Math::TWO_PI;

        size_t vIdx = 0u;
        for (int v = 0; v <= this->segmentsV; ++v)
        {
            const float phi = static_cast<float>(v) / static_cast<float>(this->segmentsV) * PI;

            for (int h = 0; h <= this->segmentsH; ++h)
            {
                const float theta = static_cast<float>(h) / static_cast<float>(this->segmentsH) * TWO_PI;
                const float sinPhi = std::sin(phi);
                const float cosPhi = std::cos(phi);
                const float sinTheta = std::sin(theta);
                const float cosTheta = std::cos(theta);

                Ogre::Vector3 dir(sinPhi * cosTheta, cosPhi, sinPhi * sinTheta);

                this->baseDirs[vIdx] = dir;
                this->basePositions[vIdx] = dir * this->radius;
                this->uvCoords[vIdx] = Ogre::Vector2(static_cast<float>(h) / static_cast<float>(this->segmentsH), static_cast<float>(v) / static_cast<float>(this->segmentsV));

                ++vIdx;
            }
        }

        // CCW winding for outward-facing normals.
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

    void PlanetOcean::recalculateNormals()
    {
        for (size_t i = 0u; i < this->vertexCount; ++i)
        {
            this->normals[i] = Ogre::Vector3::ZERO;
        }

        const size_t triCount = this->indexCount / 3u;
        for (size_t t = 0u; t < triCount; ++t)
        {
            const uint32_t i0 = this->indices[t * 3u + 0u];
            const uint32_t i1 = this->indices[t * 3u + 1u];
            const uint32_t i2 = this->indices[t * 3u + 2u];

            const Ogre::Vector3& p0 = this->vertices[i0];
            const Ogre::Vector3& p1 = this->vertices[i1];
            const Ogre::Vector3& p2 = this->vertices[i2];

            const Ogre::Vector3 faceNormal = (p1 - p0).crossProduct(p2 - p0);
            this->normals[i0] += faceNormal;
            this->normals[i1] += faceNormal;
            this->normals[i2] += faceNormal;
        }

        for (size_t i = 0u; i < this->vertexCount; ++i)
        {
            if (this->normals[i] != Ogre::Vector3::ZERO)
            {
                this->normals[i].normalise();
            }
            else
            {
                this->normals[i] = this->baseDirs[i];
            }
        }
    }

    void PlanetOcean::recalculateTangents()
    {
        for (size_t i = 0u; i < this->vertexCount; ++i)
        {
            const Ogre::Vector3& N = this->normals[i];

            Ogre::Vector3 up(0.0f, 1.0f, 0.0f);
            if (std::fabs(N.dotProduct(up)) > 0.99f)
            {
                up = Ogre::Vector3(1.0f, 0.0f, 0.0f);
            }

            Ogre::Vector3 T = up.crossProduct(N);
            T.normalise();

            float w = 1.0f;
            if (this->uvCoords[i].x > 0.5f)
            {
                w = -1.0f;
            }

            this->tangents[i] = Ogre::Vector4(T.x, T.y, T.z, w);
        }
    }

    Ogre::Item* PlanetOcean::buildItemFromCPUArrays(const Ogre::String& meshName)
    {
        Ogre::VaoManager* vaoManager = this->sceneManager->getDestinationRenderSystem()->getVaoManager();

        // Standard 12-float vertex layout.
        Ogre::VertexElement2Vec elems;
        elems.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_POSITION));
        elems.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_NORMAL));
        elems.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT4, Ogre::VES_TANGENT));
        elems.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES));

        constexpr size_t fpv = 12u;

        float* vd = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(this->vertexCount * fpv * sizeof(float), Ogre::MEMCATEGORY_GEOMETRY));

        for (size_t i = 0u; i < this->vertexCount; ++i)
        {
            size_t o = i * fpv;
            vd[o++] = this->vertices[i].x;
            vd[o++] = this->vertices[i].y;
            vd[o++] = this->vertices[i].z;
            vd[o++] = this->normals[i].x;
            vd[o++] = this->normals[i].y;
            vd[o++] = this->normals[i].z;
            vd[o++] = this->tangents[i].x;
            vd[o++] = this->tangents[i].y;
            vd[o++] = this->tangents[i].z;
            vd[o++] = this->tangents[i].w;
            vd[o++] = this->uvCoords[i].x;
            vd[o] = this->uvCoords[i].y;
        }

        // BT_DYNAMIC_DEFAULT allows upload() every frame for wave animation.
        this->vertexBuffer = vaoManager->createVertexBuffer(elems, this->vertexCount, Ogre::BT_DYNAMIC_DEFAULT, vd, false);

        OGRE_FREE_SIMD(vd, Ogre::MEMCATEGORY_GEOMETRY);

        // Index buffer (immutable).
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

        const float boundsRadius = this->radius + 4.0f;
        mesh->_setBounds(Ogre::Aabb(Ogre::Vector3::ZERO, Ogre::Vector3(boundsRadius)), false);
        mesh->_setBoundingSphereRadius(boundsRadius);

        this->oceanMesh = mesh;

        return this->sceneManager->createItem(mesh, Ogre::SCENE_DYNAMIC);
    }

    // =========================================================================
    //  Wave simulation
    // =========================================================================

    void PlanetOcean::applyWaves()
    {
        const float t = this->oceanTime;

        for (size_t idx = 0u; idx < this->vertexCount; ++idx)
        {
            const Ogre::Vector3& dir = this->baseDirs[idx];
            float totalHeight = 0.0f;

            for (int w = 0; w < 4; ++w)
            {
                const WaveParams& wave = this->waves[static_cast<size_t>(w)];

                // 3D wave propagation direction in equatorial plane.
                const float ca = std::cos(wave.directionAngle);
                const float sa = std::sin(wave.directionAngle);
                const Ogre::Vector3 waveDir3(ca, 0.0f, sa);

                // Phase = dot of vertex direction onto wave direction, scaled by radius.
                const float phase = wave.frequency * dir.dotProduct(waveDir3) * this->radius - wave.speed * t;

                totalHeight += wave.amplitude * std::sin(phase);
            }

            totalHeight *= this->waveAmplitudeScale;

            this->vertices[idx] = dir * (this->radius + totalHeight);
        }
    }

    void PlanetOcean::uploadVertexData()
    {
        if (nullptr == this->vertexBuffer)
        {
            return;
        }

        constexpr size_t fpv = 12u;
        const size_t dataSize = this->vertexCount * fpv;

        float* vd = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(dataSize * sizeof(float), Ogre::MEMCATEGORY_GEOMETRY));

        for (size_t i = 0u; i < this->vertexCount; ++i)
        {
            size_t o = i * fpv;
            vd[o++] = this->vertices[i].x;
            vd[o++] = this->vertices[i].y;
            vd[o++] = this->vertices[i].z;
            vd[o++] = this->normals[i].x;
            vd[o++] = this->normals[i].y;
            vd[o++] = this->normals[i].z;
            vd[o++] = this->tangents[i].x;
            vd[o++] = this->tangents[i].y;
            vd[o++] = this->tangents[i].z;
            vd[o++] = this->tangents[i].w;
            vd[o++] = this->uvCoords[i].x;
            vd[o] = this->uvCoords[i].y;
        }

        this->vertexBuffer->upload(vd, 0u, this->vertexCount);

        OGRE_FREE_SIMD(vd, Ogre::MEMCATEGORY_GEOMETRY);
    }

} // namespace NOWA
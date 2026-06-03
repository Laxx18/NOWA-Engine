#include "NOWAPrecompiled.h"
#include "PlanetSun.h"

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

    PlanetSun::PlanetSun(const Ogre::String& objectName, Ogre::SceneManager* sceneManager) :
        objectName(objectName),
        sceneManager(sceneManager),
        attachedNode(nullptr),
        sunItem(nullptr),
        vertexBuffer(nullptr),
        vertexCount(0u),
        indexCount(0u),
        sunTime(0.0f),
        animationSpeed(0.08f),
        plasmaScale(3.5f),
        radius(55.0f),
        segmentsH(32),
        segmentsV(32)
    {
    }

    PlanetSun::~PlanetSun()
    {
        this->destroy();
    }

    // =========================================================================
    //  Lifecycle
    // =========================================================================

    bool PlanetSun::create(float radius, int segmentsH, int segmentsV, Ogre::SceneNode* attachNode, const Ogre::String& datablockName)
    {
        this->radius = radius;
        this->segmentsH = segmentsH;
        this->segmentsV = segmentsV;
        this->attachedNode = attachNode;

        this->generateBaseSphere();

        const Ogre::String meshName = this->objectName + "_SunMesh";
        this->sunItem = this->buildItem(meshName);

        if (nullptr == this->sunItem)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PlanetSun] Failed to build item for '" + this->objectName + "'.");
            return false;
        }

        this->setDatablockByName(datablockName);
        this->initPlasmaTexture();
        // Match the item's static state to the node before re-attaching.
        if (sunItem->isStatic() != attachedNode->isStatic())
        {
            sunItem->setStatic(attachedNode->isStatic());
        }

        this->attachedNode->attachObject(this->sunItem);

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PlanetSun] Created sun '" + this->objectName + "' radius=" + Ogre::StringConverter::toString(this->radius) + " segH=" + Ogre::StringConverter::toString(this->segmentsH) +
                                                                               " segV=" + Ogre::StringConverter::toString(this->segmentsV) + " verts=" + Ogre::StringConverter::toString(static_cast<unsigned int>(this->vertexCount)));

        return true;
    }

    void PlanetSun::destroy()
    {
        if (nullptr != this->sunItem)
        {
            if (nullptr != this->attachedNode)
            {
                this->attachedNode->detachObject(this->sunItem);
            }
            this->sceneManager->destroyItem(this->sunItem);
            this->sunItem = nullptr;
        }

        if (this->sunMesh)
        {
            Ogre::MeshManager::getSingleton().remove(this->sunMesh->getHandle());
            this->sunMesh.reset();
        }

        this->vertexBuffer = nullptr;
    }

    bool PlanetSun::recreate(float r, int segH, int segV, Ogre::SceneNode* attachNode, const Ogre::String& datablockName)
    {
        this->destroy();
        return this->create(r, segH, segV, attachNode, datablockName);
    }

    void PlanetSun::update(float deltaTime)
    {
        this->sunTime += deltaTime;
        this->updateAndUpload();
    }

    // =========================================================================
    //  Material helpers
    // =========================================================================

    void PlanetSun::setDatablockByName(const Ogre::String& name)
    {
        if (nullptr == this->sunItem || name.empty())
        {
            return;
        }

        Ogre::HlmsDatablock* db = Ogre::Root::getSingleton().getHlmsManager()->getDatablockNoDefault(name);
        if (nullptr != db)
        {
            this->sunItem->getSubItem(0)->setDatablock(db);
        }
        else
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PlanetSun] Datablock '" + name + "' not found.");
        }
    }

    void PlanetSun::setEmissiveColour(const Ogre::Vector3& colour)
    {
        if (nullptr == this->sunItem)
        {
            return;
        }
        Ogre::HlmsPbsDatablock* db = dynamic_cast<Ogre::HlmsPbsDatablock*>(this->sunItem->getSubItem(0)->getDatablock());
        if (nullptr != db)
        {
            db->setEmissive(colour);
        }
    }

    void PlanetSun::setDiffuseColour(const Ogre::Vector3& colour)
    {
        if (nullptr == this->sunItem)
        {
            return;
        }
        Ogre::HlmsPbsDatablock* db = dynamic_cast<Ogre::HlmsPbsDatablock*>(this->sunItem->getSubItem(0)->getDatablock());
        if (nullptr != db)
        {
            db->setDiffuse(colour);
        }
    }

    void PlanetSun::setRoughness(float roughness)
    {
        if (nullptr == this->sunItem)
        {
            return;
        }
        Ogre::HlmsPbsDatablock* db = dynamic_cast<Ogre::HlmsPbsDatablock*>(this->sunItem->getSubItem(0)->getDatablock());
        if (nullptr != db)
        {
            db->setRoughness(roughness);
        }
    }

    void PlanetSun::setAnimationSpeed(float speed)
    {
        this->animationSpeed = speed;
    }

    float PlanetSun::getAnimationSpeed() const
    {
        return this->animationSpeed;
    }

    void PlanetSun::setPlasmaScale(float scale)
    {
        this->plasmaScale = scale;
    }

    float PlanetSun::getPlasmaScale() const
    {
        return this->plasmaScale;
    }

    Ogre::Item* PlanetSun::getItem() const
    {
        return this->sunItem;
    }

    Ogre::SceneNode* PlanetSun::getAttachedNode() const
    {
        return this->attachedNode;
    }

    Ogre::SceneManager* PlanetSun::getSceneManager() const
    {
        return this->sceneManager;
    }

    float PlanetSun::getRadius() const
    {
        return this->radius;
    }

    void PlanetSun::setDynamic(bool dynamic)
    {
        if (nullptr != this->sunItem)
        {
            this->sunItem->setStatic(!dynamic);
        }
    }

    // =========================================================================
    //  initPlasmaTexture -- called ONCE from create()
    //  Creates a 128x128 GPU texture used as an emissive animation map,
    //  bound to PBSM_EMISSIVE so the sun glows without lighting dependency.
    // =========================================================================

    void PlanetSun::initPlasmaTexture()
    {
        if (nullptr == this->sunItem)
        {
            return;
        }

        Ogre::HlmsPbsDatablock* db = dynamic_cast<Ogre::HlmsPbsDatablock*>(this->sunItem->getSubItem(0)->getDatablock());
        if (nullptr == db)
        {
            return;
        }

        Ogre::TextureGpuManager* texMgr = this->sceneManager->getDestinationRenderSystem()->getTextureGpuManager();

        // Load sun_1_d.jpg as a standard resident texture — Ogre manages lifetime.
        // No procedural generation needed — UV scrolling in the vertex buffer
        // provides the animation at near-zero CPU cost.
        Ogre::TextureGpu* sunTex =
            texMgr->createOrRetrieveTexture("sun_1_d.jpg", Ogre::GpuPageOutStrategy::Discard, 0u, Ogre::TextureTypes::Type2D, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME, Ogre::TextureFilter::FilterTypes::TypeGenerateDefaultMipmaps);

        sunTex->scheduleTransitionTo(Ogre::GpuResidency::Resident);

        Ogre::HlmsSamplerblock samplerRef;
        samplerRef.mU = Ogre::TAM_WRAP;
        samplerRef.mV = Ogre::TAM_WRAP;
        samplerRef.mW = Ogre::TAM_WRAP;
        samplerRef.mMinFilter = Ogre::FO_ANISOTROPIC;
        samplerRef.mMagFilter = Ogre::FO_ANISOTROPIC;
        samplerRef.mMipFilter = Ogre::FO_ANISOTROPIC;
        samplerRef.mMaxAnisotropy = 8u;

        db->setTexture(Ogre::PBSM_DIFFUSE, sunTex, &samplerRef);

        // Constant emissive so the sun glows independently of scene lighting.
        db->setEmissive(Ogre::Vector3(0.4f, 0.2f, 0.02f));
        db->setRoughness(0.95f);
        db->setFresnel(Ogre::Vector3(0.01f, 0.01f, 0.01f), false);

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PlanetSun] Sun texture bound, UV scroll animation enabled for '" + this->objectName + "'.");
    }

#if 0
    void PlanetSun::updateAndUpload()
    {
        if (nullptr == this->vertexBuffer)
        {
            return;
        }

        const float t = this->sunTime * this->animationSpeed;
        const float sc = this->plasmaScale;
        const float speedA = t * 0.07f;
        const float speedB = t * 0.04f;

        // Flow axes in world space
        const float cosA = std::cos(0.3f), sinA = std::sin(0.3f);
        const float cosB = std::cos(1.9f), sinB = std::sin(1.9f);

        constexpr size_t fpv = 12u;
        float* vd = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(this->vertexCount * fpv * sizeof(float), Ogre::MEMCATEGORY_GEOMETRY));

        for (size_t i = 0u; i < this->vertexCount; ++i)
        {
            const Ogre::Vector3& d = this->baseDirs[i];
            const Ogre::Vector4& tg = this->tangents[i];

            float px = d.x * this->radius;
            float py = d.y * this->radius;
            float pz = d.z * this->radius;

            // Spherical projection from 3D direction — camera-independent.
            // atan2/asin give a stable equirectangular mapping anchored to
            // world space, not UV0 which has a visible seam.
            float su = (std::atan2(d.z, d.x) / Ogre::Math::TWO_PI + 0.5f) * sc;
            float sv = (std::asin(Ogre::Math::Clamp(d.y, -1.0f, 1.0f)) / Ogre::Math::PI + 0.5f) * sc;

            // Add world-space flow scroll — fixed to sphere surface
            float dotA = d.x * cosA + d.z * sinA;
            float dotB = d.y * cosB + d.z * sinB;
            su += dotA * speedA;
            sv += dotB * speedB;

            size_t o = i * fpv;
            vd[o++] = px;
            vd[o++] = py;
            vd[o++] = pz;
            vd[o++] = d.x;
            vd[o++] = d.y;
            vd[o++] = d.z;
            vd[o++] = tg.x;
            vd[o++] = tg.y;
            vd[o++] = tg.z;
            vd[o++] = tg.w;
            vd[o++] = su;
            vd[o] = sv;
        }

        this->vertexBuffer->upload(vd, 0u, this->vertexCount);
        OGRE_FREE_SIMD(vd, Ogre::MEMCATEGORY_GEOMETRY);
    }
#endif

    void PlanetSun::updateAndUpload()
    {
        if (nullptr == this->vertexBuffer)
        {
            return;
        }

        const float t = this->sunTime * this->animationSpeed;
        const float sc = this->plasmaScale;

        // Two flow directions in 3D space — avoids UV-space camera dependency.
        // Flow A: along XZ diagonal. Flow B: along YZ diagonal.
        // Using dot product with sphere direction (same as ocean wave dot) keeps
        // the pattern fixed relative to the sphere surface, not the camera.
        const float cosA = std::cos(0.3f), sinA = std::sin(0.3f); // flow A angle
        const float cosB = std::cos(1.9f), sinB = std::sin(1.9f); // flow B angle
        const float speedA = t * 0.07f;
        const float speedB = t * 0.04f;

        constexpr size_t fpv = 12u;
        float* vd = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(this->vertexCount * fpv * sizeof(float), Ogre::MEMCATEGORY_GEOMETRY));

        for (size_t i = 0u; i < this->vertexCount; ++i)
        {
            const Ogre::Vector3& d = this->baseDirs[i];
            const Ogre::Vector4& tg = this->tangents[i];
            const Ogre::Vector2& uv = this->uvCoords[i];

            float px = d.x * this->radius;
            float py = d.y * this->radius;
            float pz = d.z * this->radius;

            // Project direction onto flow axes — world-space dot product,
            // identical to how PlanetOcean computes wave phase per vertex.
            // This makes the scroll fixed to the sphere, not the camera.
            float dotA = d.x * cosA + d.z * sinA; // flow A
            float dotB = d.y * cosB + d.z * sinB; // flow B (different plane)

            // UV: base sphere UV scaled for tiling + world-space scroll offset.
            // No fmod — let GPU sampler's TAM_WRAP handle the repeat.
            float su = uv.x * sc + dotA * speedA;
            float sv = uv.y * sc + dotB * speedB;

            size_t o = i * fpv;
            vd[o++] = px;
            vd[o++] = py;
            vd[o++] = pz;
            vd[o++] = d.x;
            vd[o++] = d.y;
            vd[o++] = d.z;
            vd[o++] = tg.x;
            vd[o++] = tg.y;
            vd[o++] = tg.z;
            vd[o++] = tg.w;
            vd[o++] = su;
            vd[o] = sv;
        }

        this->vertexBuffer->upload(vd, 0u, this->vertexCount);
        OGRE_FREE_SIMD(vd, Ogre::MEMCATEGORY_GEOMETRY);
    }

    // =========================================================================
    //  Geometry generation
    // =========================================================================

    void PlanetSun::generateBaseSphere()
    {
        this->vertexCount = static_cast<size_t>((this->segmentsH + 1) * (this->segmentsV + 1));
        this->indexCount = static_cast<size_t>(this->segmentsH) * static_cast<size_t>(this->segmentsV) * 6u;

        this->baseDirs.resize(this->vertexCount);
        this->uvCoords.resize(this->vertexCount);
        this->tangents.resize(this->vertexCount);
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

                Ogre::Vector3 dir(sinPhi * std::cos(theta), cosPhi, sinPhi * std::sin(theta));
                this->baseDirs[vIdx] = dir;
                this->uvCoords[vIdx] = Ogre::Vector2(static_cast<float>(h) / static_cast<float>(this->segmentsH), static_cast<float>(v) / static_cast<float>(this->segmentsV));

                Ogre::Vector3 T(-std::sin(theta), 0.0f, std::cos(theta));
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

    Ogre::Item* PlanetSun::buildItem(const Ogre::String& meshName)
    {
        Ogre::VaoManager* vaoManager = this->sceneManager->getDestinationRenderSystem()->getVaoManager();

        Ogre::VertexElement2Vec elems;
        elems.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_POSITION));
        elems.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_NORMAL));
        elems.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT4, Ogre::VES_TANGENT));
        elems.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES));

        constexpr size_t fpv = 12u;

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
            vd[o++] = d.x;
            vd[o++] = d.y;
            vd[o++] = d.z;
            vd[o++] = tg.x;
            vd[o++] = tg.y;
            vd[o++] = tg.z;
            vd[o++] = tg.w;
            vd[o++] = uv.x;
            vd[o] = uv.y;
        }

        // BT_DYNAMIC_DEFAULT + false (keep ownership) — mirrors PlanetOcean exactly.
        this->vertexBuffer = vaoManager->createVertexBuffer(elems, this->vertexCount, Ogre::BT_DYNAMIC_DEFAULT, vd, false);

        OGRE_FREE_SIMD(vd, Ogre::MEMCATEGORY_GEOMETRY);

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

        // Sun has no vertex displacement — bounds = exact radius + small margin.
        const float boundsRadius = this->radius + 2.0f;
        mesh->_setBounds(Ogre::Aabb(Ogre::Vector3::ZERO, Ogre::Vector3(boundsRadius)), false);
        mesh->_setBoundingSphereRadius(boundsRadius);

        this->sunMesh = mesh;
        return this->sceneManager->createItem(mesh, Ogre::SCENE_DYNAMIC);
    }

} // namespace NOWA

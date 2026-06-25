#include "NOWAPrecompiled.h"
#include "HlmsWind.h"

namespace NOWA
{
    // =========================================================================
    // HlmsWindListener
    // =========================================================================

    HlmsWindListener::HlmsWindListener() : pendingInteractorCount(0), renderInteractorCount(0)
    {
        memset(this->pendingInteractors, 0, sizeof(this->pendingInteractors));
        memset(this->renderInteractors, 0, sizeof(this->renderInteractors));
        memset(this->slotOccupied, 0, sizeof(this->slotOccupied));
    }

    // -------------------------------------------------------------------------
    // Logic-thread scalar setters — write pendingParams only.
    // -------------------------------------------------------------------------

    void HlmsWindListener::setTime(Ogre::Real time)
    {
        this->pendingParams.globalTime = time;
    }

    void HlmsWindListener::addTime(Ogre::Real dt)
    {
        this->pendingParams.globalTime += dt;
    }

    void HlmsWindListener::setWindStrength(Ogre::Real strength)
    {
        this->pendingParams.windStrength = strength;
    }

    Ogre::Real HlmsWindListener::getWindStrength() const
    {
        return this->pendingParams.windStrength;
    }

    void HlmsWindListener::setWindDirection(const Ogre::Vector3& direction)
    {
        this->pendingParams.windDirection = direction.normalisedCopy();
    }

    const Ogre::Vector3& HlmsWindListener::getWindDirection() const
    {
        return this->pendingParams.windDirection;
    }

    void HlmsWindListener::setWindFrequency(Ogre::Real frequency)
    {
        this->pendingParams.windFrequency = frequency;
    }

    Ogre::Real HlmsWindListener::getWindFrequency() const
    {
        return this->pendingParams.windFrequency;
    }

    int HlmsWindListener::acquireInteractorSlot()
    {
        for (int i = 0; i < WIND_MAX_INTERACTORS; ++i)
        {
            if (false == this->slotOccupied[i])
            {
                this->slotOccupied[i] = true;
                return i;
            }
        }
        return -1;
    }

    void HlmsWindListener::releaseInteractorSlot(int slot)
    {
        if (slot < 0 || slot >= WIND_MAX_INTERACTORS)
        {
            return;
        }
        this->slotOccupied[slot] = false;
        this->pendingInteractors[slot] = {};
        this->renderInteractors[slot] = {};
    }

    void HlmsWindListener::updateInteractorSlot(int slot, float worldX, float worldZ, float radius, float strength)
    {
        if (slot < 0 || slot >= WIND_MAX_INTERACTORS)
        {
            return;
        }
        WindInteractor& s = this->pendingInteractors[slot];
        s.worldX = worldX;
        s.worldZ = worldZ;
        s.radius = radius;
        s.strength = strength;
    }

    void HlmsWindListener::commitPendingToRender()
    {
        this->renderParams = this->pendingParams;

        // Copy all slots — occupied ones carry live data, released ones are zeroed.
        // No count management, no clearing. Slots own their own lifetime.
        memcpy(this->renderInteractors, this->pendingInteractors, sizeof(this->renderInteractors));

        // Count active slots for the shader
        this->renderInteractorCount = 0;
        for (int i = 0; i < WIND_MAX_INTERACTORS; ++i)
        {
            if (true == this->slotOccupied[i])
            {
                ++this->renderInteractorCount;
            }
        }
    }

    // -------------------------------------------------------------------------
    // Ogre HlmsListener callbacks — render thread, read renderParams / renderInteractors only.
    // -------------------------------------------------------------------------

    Ogre::uint32 HlmsWindListener::getPassBufferSize(
        const Ogre::CompositorShadowNode* /*shadowNode*/,
        bool casterPass, bool /*dualParaboloid*/,
        Ogre::SceneManager* /*sceneManager*/) const
    {
        if (casterPass)
        {
            return 0u;
        }

        // float4 0 : windStrength, globalTime, windFrequency, pad        =  16 bytes
        // float4 1 : windDirection.xyz, pad                              =  16 bytes
        // float4 2 : activeInteractorCount, pad, pad, pad                =  16 bytes
        // float4 3..3+N-1 : worldX, worldZ, radius, strength (x N)      = N*16 bytes
        // Total with N=8: 16+16+16+128 = 176 bytes
        return static_cast<Ogre::uint32>(
            sizeof(float) * 8u +                           // float4 0 + float4 1
            sizeof(float) * 4u +                           // float4 2 (interactorCount)
            sizeof(float) * 4u * WIND_MAX_INTERACTORS);   // interactor slots
    }

    float* HlmsWindListener::preparePassBuffer(
        const Ogre::CompositorShadowNode* /*shadowNode*/,
        bool casterPass, bool /*dualParaboloid*/,
        Ogre::SceneManager* /*sceneManager*/,
        float* passBufferPtr)
    {
        if (casterPass)
        {
            return passBufferPtr;
        }

        // Read exclusively from render-side copies — never touch pending* here.
        const WindParams& p = this->renderParams;

        // float4 0: windStrength, globalTime, windFrequency, pad
        *passBufferPtr++ = p.windStrength;
        *passBufferPtr++ = p.globalTime;
        *passBufferPtr++ = p.windFrequency;
        *passBufferPtr++ = 0.0f;

        // float4 1: windDirection.xyz, pad
        *passBufferPtr++ = p.windDirection.x;
        *passBufferPtr++ = p.windDirection.y;
        *passBufferPtr++ = p.windDirection.z;
        *passBufferPtr++ = 0.0f;

        // float4 2: activeInteractorCount, pad, pad, pad
        *passBufferPtr++ = static_cast<float>(this->renderInteractorCount);
        *passBufferPtr++ = 0.0f;
        *passBufferPtr++ = 0.0f;
        *passBufferPtr++ = 0.0f;

        // float4 3..N: interactor slots — always upload full array (unused slots are zero)
        for (int i = 0; i < WIND_MAX_INTERACTORS; ++i)
        {
            *passBufferPtr++ = this->renderInteractors[i].worldX;
            *passBufferPtr++ = this->renderInteractors[i].worldZ;
            *passBufferPtr++ = this->renderInteractors[i].radius;
            *passBufferPtr++ = this->renderInteractors[i].strength;
        }

        return passBufferPtr;
    }

    // =========================================================================
    // HlmsWind
    // =========================================================================

    HlmsWind::HlmsWind(Ogre::Archive* dataFolder, Ogre::ArchiveVec* libraryFolders)
        : Ogre::HlmsPbs(dataFolder, libraryFolders)
        , noiseSamplerBlock(nullptr)
        , noiseTexture(nullptr)
    {
        mType        = Ogre::HLMS_USER0;
        mTypeName    = "Wind";
        mTypeNameStr = "Wind";

        // windListener is a member — its lifetime equals this object's lifetime,
        // so the pointer remains valid for the entire engine session.
        setListener(&this->windListener);

        // Reserve two texture slots for the Perlin noise volume (slot 14)
        // and one additional slot inherited from PBS.
        mReservedTexSlots = 2u;
    }

    // -------------------------------------------------------------------------
    // Setup / teardown
    // -------------------------------------------------------------------------

    void HlmsWind::setup(Ogre::SceneManager* sceneManager)
    {
        this->loadTexturesAndSamplers(sceneManager);
    }

    void HlmsWind::shutdown(Ogre::SceneManager* sceneManager)
    {
        if (nullptr != this->noiseTexture)
        {
            Ogre::TextureGpuManager* textureManager =
                sceneManager->getDestinationRenderSystem()->getTextureGpuManager();
            textureManager->destroyTexture(this->noiseTexture);
            this->noiseTexture = nullptr;
        }

        if (nullptr != this->noiseSamplerBlock)
        {
            Ogre::Root::getSingleton().getHlmsManager()->destroySamplerblock(
                this->noiseSamplerBlock);
            this->noiseSamplerBlock = nullptr;
        }
    }

    // -------------------------------------------------------------------------
    // Logic-thread scalar API — delegate to listener pending state.
    // -------------------------------------------------------------------------

    void HlmsWind::addTime(float dt)
    {
        this->windListener.addTime(dt);
    }

    void HlmsWind::setWindStrength(float strength)
    {
        this->windListener.setWindStrength(strength);
    }

    float HlmsWind::getWindStrength() const
    {
        return this->windListener.getWindStrength();
    }

    void HlmsWind::setWindDirection(const Ogre::Vector3& direction)
    {
        this->windListener.setWindDirection(direction);
    }

    const Ogre::Vector3& HlmsWind::getWindDirection() const
    {
        return this->windListener.getWindDirection();
    }

    void HlmsWind::setWindFrequency(float frequency)
    {
        this->windListener.setWindFrequency(frequency);
    }

    float HlmsWind::getWindFrequency() const
    {
        return this->windListener.getWindFrequency();
    }

    int HlmsWind::acquireInteractorSlot()
    {
        return this->windListener.acquireInteractorSlot();
    }

    void HlmsWind::releaseInteractorSlot(int slot)
    {
        this->windListener.releaseInteractorSlot(slot);
    }

    void HlmsWind::updateInteractorSlot(int slot, float worldX, float worldZ, float radius, float strength)
    {
        this->windListener.updateInteractorSlot(slot, worldX, worldZ, radius, strength);
    }

    // -------------------------------------------------------------------------
    // Render-thread commit
    // -------------------------------------------------------------------------

    void HlmsWind::commitPendingToRender()
    {
        this->windListener.commitPendingToRender();
    }

    // -------------------------------------------------------------------------
    // Ogre overrides
    // -------------------------------------------------------------------------

    Ogre::Hlms::PropertiesMergeStatus HlmsWind::notifyPropertiesMergedPreGenerationStep(
        size_t tid, Ogre::PiecesMap* inOutPieces)
    {
        PropertiesMergeStatus status =
            HlmsPbs::notifyPropertiesMergedPreGenerationStep(tid, inOutPieces);

        if (status == PropertiesMergeStatusError)
        {
            return status;
        }

        setTextureReg(tid, Ogre::VertexShader, "texPerlinNoise", 14);

        // Enforce consistency between hlms_fog and atmosky_npr for alpha_hash /
        // alpha-test Wind materials (grass, foliage cutout).
        //
        // AtmosphereNpr::preparePassHash() always sets BOTH hlms_fog=1 and
        // atmosky_npr=slot as a pair. For alpha_hash grass this corrupts the
        // alpha-discard path. We suppress both together after the merge so the
        // generated shader is always self-consistent.
        //
        // Suppressing only hlms_fog while leaving atmosky_npr → AtmoSettings
        // cbuffer declared but the fog block that uses it is absent → compile error.
        // Suppressing only atmosky_npr while leaving hlms_fog → fog block references
        // atmoSettings without the cbuffer → same error. Both must always match.
        const Ogre::int32 hasAlphaHash  = getProperty(tid, Ogre::HlmsBaseProp::AlphaHash);
        const Ogre::int32 hasAlphaTest  = getProperty(tid, Ogre::HlmsBaseProp::AlphaTest);
        const Ogre::int32 hasAlphaBlend = getProperty(tid, Ogre::HlmsBaseProp::AlphaBlend);
        const Ogre::int32 hasFog        = getProperty(tid, Ogre::HlmsBaseProp::Fog);

        if (hasFog != 0 && (hasAlphaHash != 0 || (hasAlphaTest != 0 && hasAlphaBlend == 0)))
        {
            setProperty(tid, Ogre::HlmsBaseProp::Fog, 0);
            setProperty(tid, "atmosky_npr", 0);
        }

        return status;
    }

    Ogre::uint32 HlmsWind::fillBuffersForV1(
        const Ogre::HlmsCache* cache,
        const Ogre::QueuedRenderable& queuedRenderable,
        bool casterPass, Ogre::uint32 lastCacheHash,
        Ogre::CommandBuffer* commandBuffer)
    {
        return fillBuffersFor(cache, queuedRenderable, casterPass,
                              lastCacheHash, commandBuffer, true);
    }

    Ogre::uint32 HlmsWind::fillBuffersForV2(
        const Ogre::HlmsCache* cache,
        const Ogre::QueuedRenderable& queuedRenderable,
        bool casterPass, Ogre::uint32 lastCacheHash,
        Ogre::CommandBuffer* commandBuffer)
    {
        return fillBuffersFor(cache, queuedRenderable, casterPass,
                              lastCacheHash, commandBuffer, false);
    }

    Ogre::uint32 HlmsWind::fillBuffersFor(
        const Ogre::HlmsCache* cache,
        const Ogre::QueuedRenderable& queuedRenderable,
        bool casterPass, Ogre::uint32 lastCacheHash,
        Ogre::CommandBuffer* commandBuffer,
        bool isV1)
    {
        // Bind Perlin noise volume to vertex-shader slot 14 before PBS fill.
        *commandBuffer->addCommand<Ogre::CbTexture>() =
            Ogre::CbTexture(14, const_cast<Ogre::TextureGpu*>(this->noiseTexture),
                            this->noiseSamplerBlock);

        return Ogre::HlmsPbs::fillBuffersFor(cache, queuedRenderable, casterPass,
                                             lastCacheHash, commandBuffer, isV1);
    }

    void HlmsWind::getDefaultPaths(Ogre::String& outDataFolderPath,
                                   Ogre::StringVector& outLibraryFoldersPaths)
    {
        Ogre::RenderSystem* renderSystem = Ogre::Root::getSingleton().getRenderSystem();
        Ogre::String shaderSyntax = "GLSL";
        if (renderSystem->getName() == "Direct3D11 Rendering Subsystem")
        {
            shaderSyntax = "HLSL";
        }
        else if (renderSystem->getName() == "Metal Rendering Subsystem")
        {
            shaderSyntax = "Metal";
        }

        outLibraryFoldersPaths.clear();
        outLibraryFoldersPaths.push_back("Hlms/Common/" + shaderSyntax);
        outLibraryFoldersPaths.push_back("Hlms/Common/Any");
        outLibraryFoldersPaths.push_back("Hlms/Pbs/Any");
        outLibraryFoldersPaths.push_back("Hlms/Pbs/Any/Main");
        outLibraryFoldersPaths.push_back("Hlms/Wind/Any");

        outDataFolderPath = "Hlms/pbs/" + shaderSyntax;
    }

    void HlmsWind::calculateHashForPreCreate(Ogre::Renderable* renderable,
                                             Ogre::PiecesMap* inOutPieces)
    {
        HlmsPbs::calculateHashForPreCreate(renderable, inOutPieces);
        setProperty(kNoTid, "wind_enabled", 1);
        setProperty(kNoTid, "wind_max_interactors", WIND_MAX_INTERACTORS);
        // Fog suppression is handled in notifyPropertiesMergedPreGenerationStep
        // after pass properties are merged, where the full picture is visible.
    }

    void HlmsWind::calculateHashForPreCaster(Ogre::Renderable* renderable,
                                             Ogre::PiecesMap* inOutPieces,
                                             const Ogre::PiecesMap* normalPassPieces)
    {
        HlmsPbs::calculateHashForPreCaster(renderable, inOutPieces, normalPassPieces);
        setProperty(kNoTid, "wind_enabled", 1);
        setProperty(kNoTid, "wind_max_interactors", WIND_MAX_INTERACTORS);
    }

    void HlmsWind::loadTexturesAndSamplers(Ogre::SceneManager* sceneManager)
    {
        Ogre::TextureGpuManager* textureManager =
            sceneManager->getDestinationRenderSystem()->getTextureGpuManager();

        Ogre::HlmsSamplerblock samplerblock;
        samplerblock.mU             = Ogre::TextureAddressingMode::TAM_WRAP;
        samplerblock.mV             = Ogre::TextureAddressingMode::TAM_WRAP;
        samplerblock.mW             = Ogre::TextureAddressingMode::TAM_WRAP;
        samplerblock.mMaxAnisotropy = 8;
        samplerblock.mMagFilter     = Ogre::FO_ANISOTROPIC;
        this->noiseSamplerBlock =
            Ogre::Root::getSingletonPtr()->getHlmsManager()->getSamplerblock(samplerblock);

        this->noiseTexture = textureManager->createOrRetrieveTexture(
            "windNoise.dds",
            Ogre::GpuPageOutStrategy::Discard,
            Ogre::TextureFlags::PrefersLoadingFromFileAsSRGB,
            Ogre::TextureTypes::Type3D,
            Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);

        this->noiseTexture->scheduleTransitionTo(Ogre::GpuResidency::Resident);
    }

} // namespace NOWA
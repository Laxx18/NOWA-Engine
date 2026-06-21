#include "NOWAPrecompiled.h"
#include "HlmsWind.h"

namespace NOWA
{
    HlmsWindListener::HlmsWindListener()
        : windStrength(0.5f),
        globalTime(0.0f),
        windFrequency(1.0f),
        windDirection(Ogre::Vector3(1.0f, 0.0f, 0.0f)),
        activeInteractorCount(0)
    {
        memset(this->activeInteractors, 0, sizeof(this->activeInteractors));
    }

    void HlmsWindListener::setTime(Ogre::Real time)
    {
        this->globalTime = time;
    }

    void HlmsWindListener::addTime(Ogre::Real time)
    {
        this->globalTime += time;
    }

    void HlmsWindListener::setWindStrength(Ogre::Real strength)
    {
        this->windStrength = strength;
    }

    Ogre::Real HlmsWindListener::getWindStrength() const
    {
        return this->windStrength;
    }

    void HlmsWindListener::setWindDirection(const Ogre::Vector3& direction)
    {
        this->windDirection = direction.normalisedCopy();
    }

    const Ogre::Vector3& HlmsWindListener::getWindDirection() const
    {
        return this->windDirection;
    }

    void HlmsWindListener::setWindFrequency(Ogre::Real frequency)
    {
        this->windFrequency = frequency;
    }

    Ogre::Real HlmsWindListener::getWindFrequency() const
    {
        return this->windFrequency;
    }

    void HlmsWindListener::setInteractors(const std::vector<WindInteractor>& interactors)
    {
        this->activeInteractorCount = static_cast<int>(std::min(static_cast<int>(interactors.size()), WIND_MAX_INTERACTORS));

        for (int i = 0; i < this->activeInteractorCount; ++i)
        {
            this->activeInteractors[i] = interactors[static_cast<size_t>(i)];
        }

        for (int i = this->activeInteractorCount; i < WIND_MAX_INTERACTORS; ++i)
        {
            this->activeInteractors[i] = WindInteractor{0.0f, 0.0f, 0.0f, 0.0f};
        }
    }

    void HlmsWindListener::clearInteractors()
    {
        this->activeInteractorCount = 0;
        memset(this->activeInteractors, 0, sizeof(this->activeInteractors));
    }

    void HlmsWindListener::addInteractor(float worldX, float worldZ, float radius, float strength)
    {
        if (this->activeInteractorCount >= WIND_MAX_INTERACTORS)
        {
            return;
        }
        this->activeInteractors[this->activeInteractorCount].worldX = worldX;
        this->activeInteractors[this->activeInteractorCount].worldZ = worldZ;
        this->activeInteractors[this->activeInteractorCount].radius = radius;
        this->activeInteractors[this->activeInteractorCount].strength = strength;
        ++this->activeInteractorCount;
    }

    Ogre::uint32 HlmsWindListener::getPassBufferSize(const Ogre::CompositorShadowNode* shadowNode, bool casterPass, bool dualParaboloid, Ogre::SceneManager* sceneManager) const
    {
        Ogre::uint32 size = 0;

        if (!casterPass)
        {
            // Original: float4 0 (windStrength, globalTime, windFrequency, pad)
            //           float4 1 (windDirection.xyz, pad)
            //           = 32 bytes
            // New:      float4 2 (interactorCount, pad, pad, pad)
            //           float4 3..10 (worldX, worldZ, radius, strength) x WIND_MAX_INTERACTORS
            //           = 16 + (16 * WIND_MAX_INTERACTORS) bytes extra
            // Total with WIND_MAX_INTERACTORS=8: 32 + 16 + 128 = 176 bytes
            size += sizeof(float) * 8;                        // original 2x float4
            size += sizeof(float) * 4;                        // interactorCount float4
            size += sizeof(float) * 4 * WIND_MAX_INTERACTORS; // interactor slots
        }
        return size;
    }

    float* HlmsWindListener::preparePassBuffer(const Ogre::CompositorShadowNode* shadowNode, bool casterPass, bool dualParaboloid, Ogre::SceneManager* sceneManager, float* passBufferPtr)
    {
        if (!casterPass)
        {
            // float4 0: windStrength, globalTime, windFrequency, padding  (unchanged)
            *passBufferPtr++ = this->windStrength;
            *passBufferPtr++ = this->globalTime;
            *passBufferPtr++ = this->windFrequency;
            *passBufferPtr++ = 0.0f;
            // float4 1: windDirection.xyz, padding  (unchanged)
            *passBufferPtr++ = this->windDirection.x;
            *passBufferPtr++ = this->windDirection.y;
            *passBufferPtr++ = this->windDirection.z;
            *passBufferPtr++ = 0.0f;
            // float4 2: interactorCount, pad, pad, pad
            *passBufferPtr++ = static_cast<float>(this->activeInteractorCount);
            *passBufferPtr++ = 0.0f;
            *passBufferPtr++ = 0.0f;
            *passBufferPtr++ = 0.0f;
            // float4 3..6: interactor slots (worldX, worldZ, radius, strength)
            for (int i = 0; i < WIND_MAX_INTERACTORS; ++i)
            {
                *passBufferPtr++ = this->activeInteractors[i].worldX;
                *passBufferPtr++ = this->activeInteractors[i].worldZ;
                *passBufferPtr++ = this->activeInteractors[i].radius;
                *passBufferPtr++ = this->activeInteractors[i].strength;
            }
        }
        return passBufferPtr;
    }

    /////////////////////////////////////////////////////////////////////////////////////

    HlmsWind::HlmsWind(Ogre::Archive* dataFolder, Ogre::ArchiveVec* libraryFolders) : Ogre::HlmsPbs(dataFolder, libraryFolders), noiseSamplerBlock(nullptr), noiseTexture(nullptr)
    {
        mType = Ogre::HLMS_USER0;
        mTypeName = "Wind";
        mTypeNameStr = "Wind";

        setListener(&this->windListener);

        mReservedTexSlots = 2u;
    }

    void HlmsWind::addTime(float time)
    {
        this->windListener.addTime(time);
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

    void HlmsWind::setInteractors(const std::vector<WindInteractor>& interactors)
    {
        this->windListener.setInteractors(interactors);
    }

    void HlmsWind::clearInteractors()
    {
        this->windListener.clearInteractors();
    }

    void HlmsWind::addInteractor(float worldX, float worldZ, float radius, float strength)
    {
        this->windListener.addInteractor(worldX, worldZ, radius, strength);
    }

    void HlmsWind::setup(Ogre::SceneManager* sceneManager)
    {
        this->loadTexturesAndSamplers(sceneManager);
    }

    void HlmsWind::shutdown(Ogre::SceneManager* sceneManager)
    {
        Ogre::TextureGpuManager* textureManager = sceneManager->getDestinationRenderSystem()->getTextureGpuManager();
        textureManager->destroyTexture(this->noiseTexture);
        this->noiseTexture = nullptr;

        Ogre::Root::getSingleton().getHlmsManager()->destroySamplerblock(this->noiseSamplerBlock);
        this->noiseSamplerBlock = nullptr;
    }

    Ogre::Hlms::PropertiesMergeStatus HlmsWind::notifyPropertiesMergedPreGenerationStep(size_t tid, Ogre::PiecesMap* inOutPieces)
    {
        PropertiesMergeStatus status = HlmsPbs::notifyPropertiesMergedPreGenerationStep(tid, inOutPieces);

        if (status == PropertiesMergeStatusError)
        {
            return status;
        }

        setTextureReg(tid, Ogre::VertexShader, "texPerlinNoise", 14);

        // After all properties are merged (renderable + pass), enforce consistency
        // between hlms_fog and atmosky_npr for alpha_hash / cutout Wind materials.
        //
        // AtmosphereNpr::preparePassHash() always sets BOTH hlms_fog=1 and atmosky_npr=slot
        // as a pair. For alpha_hash grass this corrupts the alpha discard. We suppress both
        // together here — after the merge — so the generated shader is always consistent:
        // either both are present (transparent Wind objects that want fog) or both are absent
        // (cutout/opaque Wind objects that must not have fog touch their alpha path).
        //
        // Suppressing only Fog while leaving atmosky_npr would declare AtmoSettings cbuffer
        // but skip the @property(hlms_fog) block that uses it → undeclared identifier error.
        // Suppressing only atmosky_npr while leaving Fog would reference atmoSettings in the
        // fog block without the cbuffer declaration → same error.
        // Both must always match.
        const Ogre::int32 hasAlphaHash = getProperty(tid, Ogre::HlmsBaseProp::AlphaHash);
        const Ogre::int32 hasAlphaTest = getProperty(tid, Ogre::HlmsBaseProp::AlphaTest);
        const Ogre::int32 hasAlphaBlend = getProperty(tid, Ogre::HlmsBaseProp::AlphaBlend);
        const Ogre::int32 hasFog = getProperty(tid, Ogre::HlmsBaseProp::Fog);

        if (hasFog != 0 && (hasAlphaHash != 0 || (hasAlphaTest != 0 && hasAlphaBlend == 0)))
        {
            setProperty(tid, Ogre::HlmsBaseProp::Fog, 0);
            setProperty(tid, "atmosky_npr", 0);
        }

        return status;
    }

    Ogre::uint32 HlmsWind::fillBuffersForV1(const Ogre::HlmsCache* cache, const Ogre::QueuedRenderable& queuedRenderable, bool casterPass, Ogre::uint32 lastCacheHash, Ogre::CommandBuffer* commandBuffer)
    {
        return fillBuffersFor(cache, queuedRenderable, casterPass, lastCacheHash, commandBuffer, true);
    }

    Ogre::uint32 HlmsWind::fillBuffersForV2(const Ogre::HlmsCache* cache, const Ogre::QueuedRenderable& queuedRenderable, bool casterPass, Ogre::uint32 lastCacheHash, Ogre::CommandBuffer* commandBuffer)
    {
        return fillBuffersFor(cache, queuedRenderable, casterPass, lastCacheHash, commandBuffer, false);
    }

    Ogre::uint32 HlmsWind::fillBuffersFor(const Ogre::HlmsCache* cache, const Ogre::QueuedRenderable& queuedRenderable, bool casterPass, Ogre::uint32 lastCacheHash, Ogre::CommandBuffer* commandBuffer, bool isV1)
    {
        *commandBuffer->addCommand<Ogre::CbTexture>() = Ogre::CbTexture(14, const_cast<Ogre::TextureGpu*>(this->noiseTexture), this->noiseSamplerBlock);

        return Ogre::HlmsPbs::fillBuffersFor(cache, queuedRenderable, casterPass, lastCacheHash, commandBuffer, isV1);
    }

    void HlmsWind::getDefaultPaths(Ogre::String& outDataFolderPath, Ogre::StringVector& outLibraryFoldersPaths)
    {
        // We need to know what RenderSystem is currently in use, as the
        // name of the compatible shading language is part of the path
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

        // Fill the library folder paths with the relevant folders
        outLibraryFoldersPaths.clear();
        outLibraryFoldersPaths.push_back("Hlms/Common/" + shaderSyntax);
        outLibraryFoldersPaths.push_back("Hlms/Common/Any");
        outLibraryFoldersPaths.push_back("Hlms/Pbs/Any");
        outLibraryFoldersPaths.push_back("Hlms/Pbs/Any/Main");
        outLibraryFoldersPaths.push_back("Hlms/Wind/Any");

        // Fill the data folder path
        outDataFolderPath = "Hlms/pbs/" + shaderSyntax;
    }

    void HlmsWind::calculateHashForPreCreate(Ogre::Renderable* renderable, Ogre::PiecesMap* inOutPieces)
    {
        HlmsPbs::calculateHashForPreCreate(renderable, inOutPieces);
        setProperty(kNoTid, "wind_enabled", 1);
        setProperty(kNoTid, "wind_max_interactors", WIND_MAX_INTERACTORS);
        // No fog manipulation here — done in notifyPropertiesMergedPreGenerationStep
        // after pass properties are merged in, where the full picture is visible.
    }

    void HlmsWind::calculateHashForPreCaster(Ogre::Renderable* renderable, Ogre::PiecesMap* inOutPieces, const Ogre::PiecesMap* normalPassPieces)
    {
        HlmsPbs::calculateHashForPreCaster(renderable, inOutPieces, normalPassPieces);
        setProperty(kNoTid, "wind_enabled", 1);
        setProperty(kNoTid, "wind_max_interactors", WIND_MAX_INTERACTORS);
    }

    void HlmsWind::loadTexturesAndSamplers(Ogre::SceneManager* sceneManager)
    {
        Ogre::TextureGpuManager* textureManager = sceneManager->getDestinationRenderSystem()->getTextureGpuManager();

        Ogre::HlmsSamplerblock samplerblock;
        samplerblock.mU = Ogre::TextureAddressingMode::TAM_WRAP;
        samplerblock.mV = Ogre::TextureAddressingMode::TAM_WRAP;
        samplerblock.mW = Ogre::TextureAddressingMode::TAM_WRAP;
        samplerblock.mMaxAnisotropy = 8;
        samplerblock.mMagFilter = Ogre::FO_ANISOTROPIC;
        this->noiseSamplerBlock = Ogre::Root::getSingletonPtr()->getHlmsManager()->getSamplerblock(samplerblock);

        this->noiseTexture =
            textureManager->createOrRetrieveTexture("windNoise.dds", Ogre::GpuPageOutStrategy::Discard, Ogre::TextureFlags::PrefersLoadingFromFileAsSRGB, Ogre::TextureTypes::Type3D, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);

        this->noiseTexture->scheduleTransitionTo(Ogre::GpuResidency::Resident);
    }

}; // namespace end
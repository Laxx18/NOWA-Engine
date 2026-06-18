#include "NOWAPrecompiled.h"
#include "HlmsWind.h"

namespace NOWA
{
    HlmsWindListener::HlmsWindListener() : windStrength(0.5f), globalTime(0.0f), windFrequency(1.0f), windDirection(Ogre::Vector3(1.0f, 0.0f, 0.0f))
    {
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

    Ogre::uint32 HlmsWindListener::getPassBufferSize(const Ogre::CompositorShadowNode* shadowNode, bool casterPass, bool dualParaboloid, Ogre::SceneManager* sceneManager) const
    {
        Ogre::uint32 size = 0;

        if (!casterPass)
        {
            // float4 0: windStrength, globalTime, windFrequency, pad
            // float4 1: windDirection.xyz, pad
            // Total: 8 floats = 32 bytes, 16-byte aligned.
            size += sizeof(float) * 8;
        }
        return size;
    }

    float* HlmsWindListener::preparePassBuffer(const Ogre::CompositorShadowNode* shadowNode, bool casterPass, bool dualParaboloid, Ogre::SceneManager* sceneManager, float* passBufferPtr)
    {
        if (!casterPass)
        {
            // float4 0: windStrength, globalTime, windFrequency, padding
            *passBufferPtr++ = this->windStrength;
            *passBufferPtr++ = this->globalTime;
            *passBufferPtr++ = this->windFrequency;
            *passBufferPtr++ = 0.0f;
            // float4 1: windDirection.xyz, padding
            *passBufferPtr++ = this->windDirection.x;
            *passBufferPtr++ = this->windDirection.y;
            *passBufferPtr++ = this->windDirection.z;
            *passBufferPtr++ = 0.0f;
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
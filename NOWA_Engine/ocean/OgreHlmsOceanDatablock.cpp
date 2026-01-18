// HlmsOceanDatablock.cpp - Fixed version matching Terra's pattern

#include "NOWAPrecompiled.h"
#include "OgreHlmsOceanDatablock.h"
#include "OgreHlmsOcean.h"

// ADD THIS: Include texture base class implementation
#define _OgreHlmsTextureBaseClassExport
#define OGRE_HLMS_TEXTURE_BASE_CLASS HlmsOceanBaseTextureDatablock
#define OGRE_HLMS_TEXTURE_BASE_MAX_TEX NUM_OCEAN_TEXTURE_TYPES
#define OGRE_HLMS_CREATOR_CLASS HlmsOcean
#include "OgreHlmsTextureBaseClass.inl"
#undef _OgreHlmsTextureBaseClassExport
#undef OGRE_HLMS_TEXTURE_BASE_CLASS
#undef OGRE_HLMS_TEXTURE_BASE_MAX_TEX
#undef OGRE_HLMS_CREATOR_CLASS

#include "OgreHlmsOceanDatablock.cpp.inc"

namespace Ogre
{
    HlmsOceanDatablock::HlmsOceanDatablock(IdString name, HlmsOcean* creator,
        const HlmsMacroblock* macroblock,
        const HlmsBlendblock* blendblock,
        const HlmsParamVec& params) :
        HlmsOceanBaseTextureDatablock(name, creator, macroblock, blendblock, params),
        mDeepColour(0.0f, 0.03f * 0.318309886f, 0.05f * 0.318309886f, 1.0f),  // .w = reflectionStrength
        mShallowColour(0.0f, 0.08f * 0.318309886f, 0.1f * 0.318309886f, 1.0f), // .w = waveScale
        mkD(1.0f, 1.0f, 1.0f, 0.0f), // White diffuse, shadowBias = 0
        mRoughness(0.01f, 0.5f, 1.0f, 0.0f), // x=baseRoughness, y=foamRoughness, z=1.0f (fully opaque by default) 
        mMetalness(0.5f, 0.01f, 1.0f, 0.0f), // x=ambientReduction, y=diffuseScale, z=foamIntensity
        mBrdf(OceanBrdf::Default)
    {
        // Initialize detail offset/scale (not used but must be initialized)
        for (int i = 0; i < 4; ++i)
            mDetailOffsetScale[i] = Vector4(0.0f, 0.0f, 1.0f, 1.0f);

        memset(mIndices0_7, 0, sizeof(mIndices0_7));
        memset(mIndices8_15, 0, sizeof(mIndices8_15));
        memset(mIndices16_24, 0, sizeof(mIndices16_24));

        creator->requestSlot(0, this, false);
        calculateHash();
    }
    //-----------------------------------------------------------------------------------
    HlmsOceanDatablock::~HlmsOceanDatablock()
    {
        if (mAssignedPool)
            static_cast<HlmsOcean*>(mCreator)->releaseSlot(this);
    }
    //-----------------------------------------------------------------------------------
    void HlmsOceanDatablock::calculateHash()
    {
        IdString hash;

        if (mTexturesDescSet)
        {
            FastArray<const TextureGpu*>::const_iterator itor = mTexturesDescSet->mTextures.begin();
            FastArray<const TextureGpu*>::const_iterator end = mTexturesDescSet->mTextures.end();
            while (itor != end)
            {
                hash += (*itor)->getName();
                ++itor;
            }
        }
        if (mSamplersDescSet)
        {
            FastArray<const HlmsSamplerblock*>::const_iterator itor =
                mSamplersDescSet->mSamplers.begin();
            FastArray<const HlmsSamplerblock*>::const_iterator end = mSamplersDescSet->mSamplers.end();
            while (itor != end)
            {
                hash += IdString((*itor)->mId);
                ++itor;
            }
        }

        if (static_cast<HlmsOcean*>(mCreator)->getOptimizationStrategy() == HlmsOcean::LowerGpuOverhead)
        {
            const size_t poolIdx = static_cast<HlmsOcean*>(mCreator)->getPoolIndex(this);
            const uint32 finalHash = (hash.getU32Value() & 0xFFFFFE00) | (poolIdx & 0x000001FF);
            mTextureHash = finalHash;
        }
        else
        {
            const size_t poolIdx = static_cast<HlmsOcean*>(mCreator)->getPoolIndex(this);
            const uint32 finalHash = (hash.getU32Value() & 0xFFFFFFF0) | (poolIdx & 0x0000000F);
            mTextureHash = finalHash;
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsOceanDatablock::scheduleConstBufferUpdate()
    {
        static_cast<HlmsOcean*>(mCreator)->scheduleForUpdate(this);
    }
    //-----------------------------------------------------------------------------------
    // In HlmsOceanDatablock.cpp - Fix uploadToConstBuffer:

    void HlmsOceanDatablock::uploadToConstBuffer(char* dstPtr, uint8 dirtyFlags)
    {
        // Check dirty flags and update descriptor sets if needed
        // This is what triggers calculateHashForPreCreate when textures change for reflection texture!
        if (dirtyFlags & (ConstBufferPool::DirtyTextures | ConstBufferPool::DirtySamplers))
        {
            // Must be called first so mTexIndices[i] gets updated before uploading to GPU.
            updateDescriptorSets((dirtyFlags & ConstBufferPool::DirtyTextures) != 0,
                (dirtyFlags & ConstBufferPool::DirtySamplers) != 0);
        }

        // Copy deepColour (16 bytes)
        memcpy(dstPtr, &mDeepColour, sizeof(Vector4));
        dstPtr += sizeof(Vector4);

        // Copy shallowColour (16 bytes)
        memcpy(dstPtr, &mShallowColour, sizeof(Vector4));
        dstPtr += sizeof(Vector4);

        // Copy kD (16 bytes)
        memcpy(dstPtr, &mkD, sizeof(Vector4));
        dstPtr += sizeof(Vector4);

        // Copy roughness (16 bytes)
        memcpy(dstPtr, &mRoughness, sizeof(Vector4));
        dstPtr += sizeof(Vector4);

        // Copy metalness (16 bytes)
        memcpy(dstPtr, &mMetalness, sizeof(Vector4));
        dstPtr += sizeof(Vector4);

        // Copy detailOffsetScale[4] (64 bytes)
        memcpy(dstPtr, mDetailOffsetScale, sizeof(Vector4) * 4);
        dstPtr += sizeof(Vector4) * 4;

        // Copy indices0_7 (16 bytes)
        memcpy(dstPtr, mIndices0_7, sizeof(uint32) * 4);
        dstPtr += sizeof(uint32) * 4;

        // Copy indices8_15 (16 bytes)
        memcpy(dstPtr, mIndices8_15, sizeof(uint32) * 4);
        dstPtr += sizeof(uint32) * 4;

        // Copy indices16_24 (16 bytes)
        memcpy(dstPtr, mIndices16_24, sizeof(uint32) * 4);

        // Total: 192 bytes
    }
    //-----------------------------------------------------------------------------------
    void HlmsOceanDatablock::setDeepColour(const Vector3& deepColour)
    {
        const float invPI = 0.318309886f;
        mDeepColour.x = deepColour.x * invPI;
        mDeepColour.y = deepColour.y * invPI;
        mDeepColour.z = deepColour.z * invPI;
        // mDeepColour.w is padding, leave it
        scheduleConstBufferUpdate();
    }
    //-----------------------------------------------------------------------------------
    Vector3 HlmsOceanDatablock::getDeepColour() const
    {
        return Vector3(mDeepColour.x, mDeepColour.y, mDeepColour.z) * Ogre::Math::PI;
    }
    //-----------------------------------------------------------------------------------
    void HlmsOceanDatablock::setShallowColour(const Vector3& shallowColour)
    {
        const float invPI = 0.318309886f;
        mShallowColour.x = shallowColour.x * invPI;
        mShallowColour.y = shallowColour.y * invPI;
        mShallowColour.z = shallowColour.z * invPI;
        // mShallowColour.w is waveScale, don't touch it here
        scheduleConstBufferUpdate();
    }
    //-----------------------------------------------------------------------------------
    Vector3 HlmsOceanDatablock::getShallowColour() const
    {
        return Vector3(mShallowColour.x, mShallowColour.y, mShallowColour.z) * Ogre::Math::PI;
    }
    //-----------------------------------------------------------------------------------
    void HlmsOceanDatablock::setReflectionStrength(float strength)
    {
        mDeepColour.w = Math::Clamp(strength, 0.0f, 1.0f);
        scheduleConstBufferUpdate();
    }
    //-----------------------------------------------------------------------------------
    float HlmsOceanDatablock::getReflectionStrength() const
    {
        return mDeepColour.w;
    }
    //-----------------------------------------------------------------------------------
    void HlmsOceanDatablock::setBaseRoughness(float roughness)
    {
        mRoughness.x = Math::Clamp(roughness, 0.001f, 1.0f);
        scheduleConstBufferUpdate();
    }
    //-----------------------------------------------------------------------------------
    float HlmsOceanDatablock::getBaseRoughness() const
    {
        return mRoughness.x;
    }
    //-----------------------------------------------------------------------------------
    void HlmsOceanDatablock::setFoamRoughness(float roughness)
    {
        mRoughness.y = Math::Clamp(roughness, 0.001f, 1.0f);
        scheduleConstBufferUpdate();
    }
    //-----------------------------------------------------------------------------------
    float HlmsOceanDatablock::getFoamRoughness() const
    {
        return mRoughness.y;
    }
    //-----------------------------------------------------------------------------------
    void HlmsOceanDatablock::setAmbientReduction(float reduction)
    {
        mMetalness.x = Math::Clamp(reduction, 0.0f, 1.0f);
        scheduleConstBufferUpdate();
    }
    //-----------------------------------------------------------------------------------
    float HlmsOceanDatablock::getAmbientReduction() const
    {
        return mMetalness.x;
    }
    //-----------------------------------------------------------------------------------
    void HlmsOceanDatablock::setDiffuseScale(float scale)
    {
        mMetalness.y = Math::Clamp(scale, 0.0f, 1.0f);
        scheduleConstBufferUpdate();
    }
    //-----------------------------------------------------------------------------------
    float HlmsOceanDatablock::getDiffuseScale() const
    {
        return mMetalness.y;
    }
    //-----------------------------------------------------------------------------------
    void HlmsOceanDatablock::setFoamIntensity(float intensity)
    {
        mMetalness.z = Math::Clamp(intensity, 0.0f, 1.0f);
        scheduleConstBufferUpdate();
    }
    //-----------------------------------------------------------------------------------
    float HlmsOceanDatablock::getFoamIntensity() const
    {
        return mMetalness.z;
    }
    //-----------------------------------------------------------------------------------
    void HlmsOceanDatablock::setWavesScale(float scale)
    {
        mShallowColour.w = scale;
        scheduleConstBufferUpdate();
    }
    //-----------------------------------------------------------------------------------
    float HlmsOceanDatablock::getWavesScale() const
    {
        return mShallowColour.w;
    }
    //-----------------------------------------------------------------------------------
    void HlmsOceanDatablock::setShadowConstantBias(float bias)
    {
        mkD.w = bias;
        scheduleConstBufferUpdate();
    }
    //-----------------------------------------------------------------------------------
    float HlmsOceanDatablock::getShadowConstantBias() const
    {
        return mkD.w;
    }
    //-----------------------------------------------------------------------------------
    void HlmsOceanDatablock::setAlphaTestThreshold(float threshold)
    {
        OGRE_EXCEPT(Exception::ERR_NOT_IMPLEMENTED,
            "Alpha testing not supported on Ocean Hlms",
            "HlmsOceanDatablock::setAlphaTestThreshold");

        HlmsDatablock::setAlphaTestThreshold(threshold);
        scheduleConstBufferUpdate();
    }
    //-----------------------------------------------------------------------------------
    void HlmsOceanDatablock::setBrdf(OceanBrdf::OceanBrdf brdf)
    {
        if (mBrdf != brdf)
        {
            mBrdf = brdf;
            flushRenderables();
        }
    }
    //-----------------------------------------------------------------------------------
    uint32 HlmsOceanDatablock::getBrdf() const
    {
        return mBrdf;
    }
    //-----------------------------------------------------------------------------------
    void HlmsOceanDatablock::setTransparency(float transparency)
    {
        transparency = Math::Clamp(transparency, 0.0f, 1.0f);

        // Determine if transparency should be enabled
        bool shouldEnable = (transparency < 1.0f);

        // Store in mRoughness.z
        if (mRoughness.z != transparency)
        {
            mRoughness.z = transparency;

            if (shouldEnable)
            {
                // Use ONE/ONE blending for additive transparency (better for water)
                HlmsBlendblock blendblock;
                blendblock.mSourceBlendFactor = SBF_ONE;
                blendblock.mDestBlendFactor = SBF_ONE_MINUS_SOURCE_ALPHA;
                blendblock.mBlendOperation = SBO_ADD;
                blendblock.setForceTransparentRenderOrder(true); // Force transparent queue
                blendblock.setBlendType(SBT_REPLACE);
                setBlendblock(blendblock, false, true);

                HlmsMacroblock macro;
                macro.mDepthCheck = true;     // still test depth
                macro.mDepthWrite = true;    // CRITICAL: do NOT write depth
                macro.mDepthFunc = CMPF_LESS_EQUAL;
                setMacroblock(macro, false, true);
            }
            else
            {
                // Opaque
                HlmsBlendblock blendblock;
                blendblock.mSourceBlendFactor = SBF_ONE;
                blendblock.mDestBlendFactor = SBF_ZERO;
                setBlendblock(blendblock, false, true);
            }
            // Force shader recompilation
            flushRenderables();

            scheduleConstBufferUpdate();
        }
    }
    //-----------------------------------------------------------------------------------
    float HlmsOceanDatablock::getTransparency() const
    {
        return mRoughness.z;
    }
    //-----------------------------------------------------------------------------------
    bool HlmsOceanDatablock::suggestUsingSRGB(OceanTextureTypes type) const
    {
        if (type == OCEAN_REFLECTION)
            return true;
        return false;
    }
}
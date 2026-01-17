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
        mDeepColour(0.0f, 0.03f * 0.318309886f, 0.05f * 0.318309886f, 0.0f),
        mShallowColour(0.0f, 0.08f * 0.318309886f, 0.1f * 0.318309886f, 1.0f),
        mkD(1.0f, 1.0f, 1.0f, 0.0f), // White diffuse, shadowBias = 0
        mRoughness(0.02f, 0.02f, 0.02f, 0.02f), // Low roughness for water
        mMetalness(0.0f, 0.0f, 0.0f, 0.0f), // Water is non-metallic
        mBrdf(OceanBrdf::Default)
    {
        // Initialize detail offset/scale (not used but must be initialized)
        for (int i = 0; i < 4; ++i)
            mDetailOffsetScale[i] = Vector4(0.0f, 0.0f, 1.0f, 1.0f);

        // Initialize indices (not used but must be initialized)
        memset(mIndices0_7, 0, sizeof(mIndices0_7));
        memset(mIndices8_15, 0, sizeof(mIndices8_15));
        memset(mIndices16_24, 0, sizeof(mIndices16_24));

        // Request slot FIRST
        creator->requestSlot(0, this, false);

        // Calculate hash
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
    bool HlmsOceanDatablock::suggestUsingSRGB(OceanTextureTypes type) const
    {
        if (type == OCEAN_REFLECTION)
            return true;
        return false;
    }
}
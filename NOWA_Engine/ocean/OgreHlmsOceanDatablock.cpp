// HlmsOceanDatablock.cpp - Fixed version matching Terra's pattern

#include "NOWAPrecompiled.h"
#include "OgreHlmsOceanDatablock.h"
#include "OgreHlmsOcean.h"

namespace Ogre
{
    const size_t HlmsOceanDatablock::MaterialSizeInGpu = 4 * 2 * 4;
    const size_t HlmsOceanDatablock::MaterialSizeInGpuAligned =
        alignToNextMultiple<uint32>(HlmsOceanDatablock::MaterialSizeInGpu, 4 * 4);

    HlmsOceanDatablock::HlmsOceanDatablock(IdString name, HlmsOcean* creator,
        const HlmsMacroblock* macroblock,
        const HlmsBlendblock* blendblock,
        const HlmsParamVec& params) :
        HlmsDatablock(name, creator, macroblock, blendblock, params),
        mDeepColourR(0.0f),
        mDeepColourG(0.03f * 0.318309886f), // Max Diffuse = 1 / PI
        mDeepColourB(0.05f * 0.318309886f),
        _padding0(0.0f),
        mShallowColourR(0.0f),
        mShallowColourG(0.08f * 0.318309886f),
        mShallowColourB(0.1f * 0.318309886f),
        mWavesScale(1.0f),
        mBrdf(OceanBrdf::Default)
    {
        // Request slot FIRST (like Terra does)
        creator->requestSlot(0, this, false);

        // Calculate hash (like Terra does)
        calculateHash();

        // DON'T set mDirtyFlags or call scheduleConstBufferUpdate here
        // The initial upload will happen automatically through HLMS system
    }

    HlmsOceanDatablock::~HlmsOceanDatablock()
    {
        if (mAssignedPool)
            static_cast<HlmsOcean*>(mCreator)->releaseSlot(this);
    }

    void HlmsOceanDatablock::calculateHash()
    {
        IdString hash;

        if (mTextureHash != hash.mHash)
        {
            mTextureHash = hash.mHash;
            static_cast<HlmsOcean*>(mCreator)->requestSlot(0, this, false);
        }
    }

    void HlmsOceanDatablock::scheduleConstBufferUpdate()
    {
        // CRITICAL: Match Terra's implementation exactly
        // Don't manually touch mDirtyFlags - let base class handle it
        static_cast<HlmsOcean*>(mCreator)->scheduleForUpdate(this);
    }

    void HlmsOceanDatablock::uploadToConstBuffer(char* dstPtr, uint8 dirtyFlags)
    {
        OGRE_UNUSED(dirtyFlags);

        // Copy the entire material data
        memcpy(dstPtr, &mDeepColourR, MaterialSizeInGpu);
    }

    void HlmsOceanDatablock::setDeepColour(const Vector3& deepColour)
    {
        const float invPI = 0.318309886f;
        mDeepColourR = deepColour.x * invPI;
        mDeepColourG = deepColour.y * invPI;
        mDeepColourB = deepColour.z * invPI;

        LogManager::getSingleton().logMessage(
            "[OceanDatablock] setDeepColour called - scheduling update");

        scheduleConstBufferUpdate();
    }

    Vector3 HlmsOceanDatablock::getDeepColour() const
    {
        return Vector3(mDeepColourR, mDeepColourG, mDeepColourB) * Ogre::Math::PI;
    }

    void HlmsOceanDatablock::setShallowColour(const Vector3& shallowColour)
    {
        const float invPI = 0.318309886f;
        mShallowColourR = shallowColour.x * invPI;
        mShallowColourG = shallowColour.y * invPI;
        mShallowColourB = shallowColour.z * invPI;
        scheduleConstBufferUpdate();
    }

    Vector3 HlmsOceanDatablock::getShallowColour() const
    {
        return Vector3(mShallowColourR, mShallowColourG, mShallowColourB) * Ogre::Math::PI;
    }

    void HlmsOceanDatablock::setWavesScale(float scale)
    {
        mWavesScale = scale;
        scheduleConstBufferUpdate();
    }

    float HlmsOceanDatablock::getWavesScale() const
    {
        return mWavesScale;
    }

    void HlmsOceanDatablock::setAlphaTestThreshold(float threshold)
    {
        OGRE_EXCEPT(Exception::ERR_NOT_IMPLEMENTED,
            "Alpha testing not supported on Ocean Hlms",
            "HlmsOceanDatablock::setAlphaTestThreshold");

        HlmsDatablock::setAlphaTestThreshold(threshold);
        scheduleConstBufferUpdate();
    }

    void HlmsOceanDatablock::setBrdf(OceanBrdf::OceanBrdf brdf)
    {
        if (mBrdf != brdf)
        {
            mBrdf = brdf;
            flushRenderables();
        }
    }

    uint32 HlmsOceanDatablock::getBrdf() const
    {
        return mBrdf;
    }
}
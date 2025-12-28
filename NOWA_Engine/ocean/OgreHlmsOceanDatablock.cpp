/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2014 Torus Knot Software Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/

#include "NOWAPrecompiled.h"

#include "OgreHlmsOceanDatablock.h"
#include "OgreHlmsOcean.h"

namespace Ogre
{
    const size_t HlmsOceanDatablock::MaterialSizeInGpu = 4 * 2 * 4;
    const size_t HlmsOceanDatablock::MaterialSizeInGpuAligned = alignToNextMultiple<uint32>(HlmsOceanDatablock::MaterialSizeInGpu, 4 * 4);

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
        calculateHash();

        // Mark as dirty initially so it gets uploaded
        mDirtyFlags = 0xFFu;

        creator->requestSlot( /*mTextureHash*/0, this, false);
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
            static_cast<HlmsOcean*>(mCreator)->requestSlot( /*mTextureHash*/0, this, false);
        }
    }

    void HlmsOceanDatablock::scheduleConstBufferUpdate()
    {
        mDirtyFlags = 0xFFu; // we upload the whole struct for now
        static_cast<HlmsOcean*>(mCreator)->scheduleForUpdate(this);
    }

    void HlmsOceanDatablock::uploadToConstBuffer(char* dstPtr, uint8 dirtyFlags)
    {
        OGRE_UNUSED(dirtyFlags);
        memcpy(dstPtr, &mDeepColourR, MaterialSizeInGpu);
    }

    void HlmsOceanDatablock::setDeepColour(const Vector3& deepColour)
    {
        const float invPI = 0.318309886f;
        mDeepColourR = deepColour.x * invPI;
        mDeepColourG = deepColour.y * invPI;
        mDeepColourB = deepColour.z * invPI;
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
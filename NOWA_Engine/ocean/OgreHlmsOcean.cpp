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

#include "OgreHlmsOcean.h"
#include "OgreHlmsOceanDatablock.h"

#if !OGRE_NO_JSON
#include "OgreHlmsJsonOcean.h"
#endif

#include "OgreHlmsManager.h"
#include "OgreHlmsListener.h"
#include "OgreLwString.h"

#include "OgreForward3D.h"
#include "OgreCamera.h"
#include "OgreAtmosphereComponent.h"
#include <OgreRoot.h>
#include <OgreTextureGpuManager.h>
#include "Cubemaps/OgreParallaxCorrectedCubemap.h"
#include "OgreIrradianceVolume.h"

#include "OgreSceneManager.h"
#include "Compositor/OgreCompositorShadowNode.h"
#include "Vao/OgreVaoManager.h"
#include "Vao/OgreConstBufferPacked.h"

#include "CommandBuffer/OgreCommandBuffer.h"
#include "CommandBuffer/OgreCbTexture.h"
#include "CommandBuffer/OgreCbShaderBuffer.h"

#include "Ocean.h"

#include <algorithm>

namespace Ogre
{
    const IdString OceanProperty::UseSkirts = IdString("use_skirts");

    HlmsOcean::HlmsOcean(Archive* dataFolder, ArchiveVec* libraryFolders) :
        HlmsPbs(dataFolder, libraryFolders),
        mShadowmapSamplerblock(0),
        mShadowmapCmpSamplerblock(0),
        mCurrentShadowmapSamplerblock(0),
        mOceanSamplerblock(0),
        mOceanDataSamplerblock(0),
        mWeightSamplerblock(0),
        mOceanDataTex(0),
        mWeightTex(0),
        mOceanTexReady(false),
        mLastBoundPool(0),
        mLastTextureHash(0),
        mLastMovableObject(0),
        mDebugPssmSplits(false),
        mShadowFilter(PCF_3x3),
        mAmbientLightMode(AmbientAuto)
    {
        mType = HLMS_USER1;
        mTypeName = "Ocean";
        mTypeNameStr = "Ocean";

        // IMPORTANT: Ocean datablock size
        mBytesPerSlot = HlmsOceanDatablock::MaterialSizeInGpuAligned;
        mOptimizationStrategy = LowerGpuOverhead;
        mSetupWorldMatBuf = false;
        mReservedTexBufferSlots = 0u;

        // Ocean uses 2 “built-in” textures in your code: terrainData + blendMap
        mReservedTexSlots = 2u;

        // Uses this to stop base from auto-requesting slots
        mSkipRequestSlotInChangeRS = true;

        // Your defaults
        mLightGatheringMode = LightGatherForwardPlus;
        mOceanDataTextureName = "oceanData.dds";
        mWeightTextureName = "weight.dds";
    }

    void HlmsOcean::_linkOcean(Ocean* ocean)
    {
        OGRE_ASSERT_LOW(ocean->mHlmsOceanIndex == std::numeric_limits<uint32>::max() &&
            "Ocean instance must be unlinked before being linked again!");

        ocean->m_hlmsOceanIndex = static_cast<uint32>(mLinkedOceans.size());
        mLinkedOceans.push_back(ocean);
    }

    void HlmsOcean::_unlinkOcean(Ocean* ocean)
    {
        if (ocean->m_hlmsOceanIndex >= mLinkedOceans.size() ||
            ocean != *(mLinkedOceans.begin() + ocean->m_hlmsOceanIndex))
        {
            OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR,
                "An Ocean instance had its mHlmsOceanIndex out of date!!! "
                "(or the instance wasn't being tracked by this HlmsOcean)",
                "HlmsOcean::_unlinkOcean");
        }

        FastArray<Ocean*>::iterator itor = mLinkedOceans.begin() + ocean->m_hlmsOceanIndex;
        itor = efficientVectorRemove(mLinkedOceans, itor);

        // The Ocean that was at the end got swapped and has now a different index
        if (itor != mLinkedOceans.end())
            (*itor)->m_hlmsOceanIndex = static_cast<uint32>(itor - mLinkedOceans.begin());

        ocean->m_hlmsOceanIndex = std::numeric_limits<uint32>::max();
    }
    //-----------------------------------------------------------------------------------
    void HlmsOcean::getDefaultPaths(String& outDataFolderPath, StringVector& outLibraryFoldersPaths)
    {
        RenderSystem* renderSystem = Root::getSingleton().getRenderSystem();
        String shaderSyntax = "GLSL";
        if (renderSystem->getName() == "Direct3D11 Rendering Subsystem")
            shaderSyntax = "HLSL";
        else if (renderSystem->getName() == "Metal Rendering Subsystem")
            shaderSyntax = "Metal";

        outLibraryFoldersPaths.clear();

        // 1. Common (platform settings)
        outLibraryFoldersPaths.push_back("Hlms/Common/" + shaderSyntax);
        outLibraryFoldersPaths.push_back("Hlms/Common/Any");

        // 2. PBS (BRDF, shadows, lighting)
        outLibraryFoldersPaths.push_back("Hlms/Pbs/Any");
        outLibraryFoldersPaths.push_back("Hlms/Pbs/Any/Main");
        outLibraryFoldersPaths.push_back("Hlms/Pbs/Any/Atmosphere");
        outLibraryFoldersPaths.push_back("Hlms/Pbs/" + shaderSyntax);

        // 3. Ocean HLSL custom + main
        outLibraryFoldersPaths.push_back("Hlms/Ocean/Any");

        outDataFolderPath = "Hlms/Ocean/" + shaderSyntax;
    }
    //-----------------------------------------------------------------------------------
    HlmsOcean::~HlmsOcean()
    {
        destroyAllBuffers();
    }
    //-----------------------------------------------------------------------------------
    void HlmsOcean::_changeRenderSystem(RenderSystem* newRs)
    {
        // Force the pool to contain only 1 entry.
        mSlotsPerPool = 1u;
        mBufferSize = HlmsOceanDatablock::MaterialSizeInGpuAligned;


        HlmsPbs::_changeRenderSystem(newRs);
        // Reset cached GPU textures when RS changes
        mOceanDataTex = nullptr;
        mWeightTex = nullptr;

        if (!newRs)
        {
            return;
        }

        // Re-request slots for datablocks (keeps your existing behaviour)
        for (auto it = mDatablocks.begin(); it != mDatablocks.end(); ++it)
        {
            HlmsOceanDatablock* datablock = static_cast<HlmsOceanDatablock*>(it->second.datablock);
            requestSlot(datablock->mTextureHash, datablock, false);
        }

        // --- Shadow samplers (your existing logic) ---
        HlmsSamplerblock samplerblock;
        samplerblock.mU = TAM_BORDER;
        samplerblock.mV = TAM_BORDER;
        samplerblock.mW = TAM_CLAMP;
        samplerblock.mBorderColour = ColourValue(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());

        if (mShaderProfile != "hlsl")
        {
            samplerblock.mMinFilter = FO_POINT;
            samplerblock.mMagFilter = FO_POINT;
            samplerblock.mMipFilter = FO_LINEAR;

            if (!mShadowmapSamplerblock)
            {
                mShadowmapSamplerblock = mHlmsManager->getSamplerblock(samplerblock);
            }
        }

        samplerblock.mMinFilter = FO_LINEAR;
        samplerblock.mMagFilter = FO_LINEAR;
        samplerblock.mMipFilter = FO_LINEAR;
        samplerblock.mCompareFunction = CMPF_LESS_EQUAL;

        if (!mShadowmapCmpSamplerblock)
        {
            mShadowmapCmpSamplerblock = mHlmsManager->getSamplerblock(samplerblock);
        }

        // --- Ocean samplers (IMPORTANT: create once, reuse forever) ---
        // Ocean data volume: point sampling is usually correct for “data textures”
        // For ocean data (3D texture):
        if (!mOceanDataSamplerblock)
        {
            HlmsSamplerblock sb;
            sb.mU = TAM_WRAP;
            sb.mV = TAM_WRAP;
            sb.mW = TAM_WRAP;

            sb.mMinFilter = FO_LINEAR;
            sb.mMagFilter = FO_LINEAR;
            sb.mMipFilter = FO_NONE;

            mOceanDataSamplerblock = mHlmsManager->getSamplerblock(sb);
        }

        // For weight map (2D texture - this can keep aniso):
        if (!mWeightSamplerblock)
        {
            HlmsSamplerblock sb;
            sb.mU = TAM_WRAP;
            sb.mV = TAM_WRAP;
            sb.mW = TAM_WRAP;

            sb.mMinFilter = FO_LINEAR;
            sb.mMagFilter = FO_LINEAR;
            sb.mMipFilter = FO_LINEAR;
            sb.mMaxAnisotropy = 8.0f;  // OK for 2D textures

            mWeightSamplerblock = mHlmsManager->getSamplerblock(sb);
        }

        // Env probe sampler (keep default, or tweak if you want)
        if (!mOceanSamplerblock)
        {
            mOceanSamplerblock = mHlmsManager->getSamplerblock(HlmsSamplerblock());
        }
    }
    //-----------------------------------------------------------------------------------
    const HlmsCache* HlmsOcean::createShaderCacheEntry(uint32 renderableHash, const HlmsCache& passCache, uint32 finalHash,
        const QueuedRenderable& queuedRenderable, HlmsCache* reservedStubEntry, uint64 deadline, const size_t tid)
    {
        const HlmsCache* retVal = Hlms::createShaderCacheEntry(renderableHash, passCache, finalHash,
            queuedRenderable, reservedStubEntry, deadline, tid);

        if (mShaderProfile == "hlsl")
        {
            mListener->shaderCacheEntryCreated(mShaderProfile, retVal, passCache, mT[tid].setProperties, queuedRenderable, tid);
            return retVal; //D3D embeds the texture slots in the shader.
        }

        //Set samplers.
        if (retVal->pso.pixelShader)
        {
            GpuProgramParametersSharedPtr psParams = retVal->pso.pixelShader->getDefaultParameters();

            int texUnit = 3; //Vertex shader consumes 1 slot with its heightmap,
            //PS consumes 2 slot with its normalmap & shadow texture.

            psParams->setNamedConstant("terrainData", 0);

            //Forward3D consumes 2 more slots.
            if (mGridBuffer)
            {
                psParams->setNamedConstant("f3dGrid", 3);
                psParams->setNamedConstant("f3dLightList", 4);
                texUnit += 2;
            }

            if (!mPreparedPass.shadowMaps.empty())
            {
                char tmpData[32];
                LwString texName = LwString::FromEmptyPointer(tmpData, sizeof(tmpData));
                texName = "texShadowMap";
                const size_t baseTexSize = texName.size();

                vector<int>::type shadowMaps;
                shadowMaps.reserve(mPreparedPass.shadowMaps.size());
                for (size_t i = 0; i < mPreparedPass.shadowMaps.size(); ++i)
                {
                    texName.resize(baseTexSize);
                    texName.a((uint32)i);   //texShadowMap0
                    psParams->setNamedConstant(texName.c_str(), &texUnit, 1, 1);
                    shadowMaps.push_back(texUnit++);
                }
            }
        }

        GpuProgramParametersSharedPtr vsParams = retVal->pso.vertexShader->getDefaultParameters();
        vsParams->setNamedConstant("terrainData", 0);
        vsParams->setNamedConstant("blendMap", 1);

        mListener->shaderCacheEntryCreated(mShaderProfile, retVal, passCache, mT[tid].setProperties, queuedRenderable, tid);

        mRenderSystem->_setPipelineStateObject(&retVal->pso);

        mRenderSystem->bindGpuProgramParameters(GPT_VERTEX_PROGRAM, vsParams, GPV_ALL);
        if (retVal->pso.pixelShader)
        {
            GpuProgramParametersSharedPtr psParams = retVal->pso.pixelShader->getDefaultParameters();
            mRenderSystem->bindGpuProgramParameters(GPT_FRAGMENT_PROGRAM, psParams, GPV_ALL);
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void HlmsOcean::calculateHashForPreCreate(Renderable* renderable, PiecesMap* inOutPieces)
    {
        assert(dynamic_cast<OceanCell*>(renderable) &&
            "This Hlms can only be used on an Ocean object!");

        OceanCell* cell = static_cast<OceanCell*>(renderable);
        HlmsOceanDatablock* datablock = static_cast<HlmsOceanDatablock*>(renderable->getDatablock());

        setProperty(kNoTid, "skip_normal_offset_bias_vs", 1);
        setProperty(kNoTid, OceanProperty::UseSkirts, cell->getUseSkirts());

        float transparency = datablock->getTransparency();
        if (transparency < 1.0f)
        {
            setProperty(kNoTid, HlmsBaseProp::AlphaBlend, 1);
            setProperty(kNoTid, "hlms_alphablend", 1);

            // Disable alpha testing when using blending
            setProperty(kNoTid, HlmsBaseProp::AlphaTest, 0);
        }
        else
        {
            setProperty(kNoTid, HlmsBaseProp::AlphaBlend, 0);
            setProperty(kNoTid, "hlms_alphablend", 0);
        }

        // PBS properties
        setProperty(kNoTid, PbsProperty::FresnelScalar, 1);
        setProperty(kNoTid, PbsProperty::FresnelWorkflow, 0);
        setProperty(kNoTid, PbsProperty::MetallicWorkflow, 1);

        // BRDF setup
        uint32 brdf = datablock->getBrdf();
        if ((brdf & OceanBrdf::BRDF_MASK) == OceanBrdf::Default)
        {
            setProperty(kNoTid, PbsProperty::BrdfDefault, 1);
            if (!(brdf & OceanBrdf::FLAG_UNCORRELATED))
                setProperty(kNoTid, PbsProperty::GgxHeightCorrelated, 1);
        }
        else if ((brdf & OceanBrdf::BRDF_MASK) == OceanBrdf::CookTorrance)
            setProperty(kNoTid, PbsProperty::BrdfCookTorrance, 1);
        else if ((brdf & OceanBrdf::BRDF_MASK) == OceanBrdf::BlinnPhong)
            setProperty(kNoTid, PbsProperty::BrdfBlinnPhong, 1);

        if (brdf & OceanBrdf::FLAG_SPERATE_DIFFUSE_FRESNEL)
            setProperty(kNoTid, PbsProperty::FresnelSeparateDiffuse, 1);

        setProperty(kNoTid, PbsProperty::NormalMap, 0);
        setProperty(kNoTid, PbsProperty::FirstValidDetailMapNm, 4);

        // ===== ENV PROBE SETUP =====

        const TextureGpu* reflectionTexture = datablock->getTexture(OCEAN_REFLECTION);

        if (datablock->mTexturesDescSet)
        {
            bool hasReflection = (reflectionTexture != nullptr);

            // Calculate NumTextures (exclude env map from count)
            bool envMap = datablock->getTexture(OCEAN_REFLECTION) != 0;
            setProperty(kNoTid, PbsProperty::NumTextures,
                int32(datablock->mTexturesDescSet->mTextures.size() - envMap));

            if (hasReflection)
            {
                // Set the texture property (sets envprobe_map_idx)
                setTextureProperty(kNoTid, PbsProperty::EnvProbeMap, datablock, OCEAN_REFLECTION);

                // Set envprobe_map to NON-ZERO
                // This is what triggers the shader to declare texEnvProbeMap
                setProperty(kNoTid, PbsProperty::EnvProbeMap,
                    static_cast<int32>(reflectionTexture->getName().getU32Value()));

                // Set TargetEnvprobeMap to DIFFERENT value to enable manual probe
                setProperty(kNoTid, PbsProperty::TargetEnvprobeMap, 0);

                // Enable env map usage
                // PBS's notifyPropertiesMergedPreGenerationStep will see envprobe_map != 0
                // and envprobe_map != target_envprobe_map, so it will set UseEnvProbeMap = 1
            }
            else
            {
                // No reflection - make sure properties are cleared
                setProperty(kNoTid, PbsProperty::EnvProbeMap, 0);
                setProperty(kNoTid, PbsProperty::TargetEnvprobeMap, 0);
            }
        }
        else
        {
            // No descriptor set = no textures at all
            setProperty(kNoTid, PbsProperty::NumTextures, 0);
            setProperty(kNoTid, PbsProperty::EnvProbeMap, 0);
            setProperty(kNoTid, PbsProperty::TargetEnvprobeMap, 0);
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsOcean::calculateHashFor(Renderable* renderable, uint32& outHash, uint32& outCasterHash)
    {
        assert(dynamic_cast<HlmsOceanDatablock*>(renderable->getDatablock()));
        HlmsOceanDatablock* datablock = static_cast<HlmsOceanDatablock*>(renderable->getDatablock());

        if (datablock->getDirtyFlags() & (DirtyTextures | DirtySamplers))
        {
            // Delay hash generation for later, when we have final descriptor sets
            outHash = 0;
            outCasterHash = 0;
        }
        else
        {
            Hlms::calculateHashFor(renderable, outHash, outCasterHash);
        }

        datablock->loadAllTextures();
    }
    //-----------------------------------------------------------------------------------
    uint32 HlmsOcean::fillBuffersFor(const HlmsCache* cache, const QueuedRenderable& queuedRenderable, bool casterPass, uint32 lastCacheHash, uint32 lastTextureHash)
    {
        OGRE_EXCEPT(Exception::ERR_NOT_IMPLEMENTED,
            "Trying to use slow-path on a desktop implementation. "
            "Change the RenderQueue settings.",
            "HlmsOcean::fillBuffersFor");
    }
    //-----------------------------------------------------------------------------------
    uint32 HlmsOcean::fillBuffersForV1(const HlmsCache* cache, const QueuedRenderable& queuedRenderable, bool casterPass, uint32 lastCacheHash, CommandBuffer* commandBuffer)
    {
        return fillBuffersFor(cache, queuedRenderable, casterPass,
            lastCacheHash, commandBuffer, true);
    }
    //-----------------------------------------------------------------------------------
    uint32 HlmsOcean::fillBuffersForV2(const HlmsCache* cache, const QueuedRenderable& queuedRenderable, bool casterPass, uint32 lastCacheHash, CommandBuffer* commandBuffer)
    {
        return fillBuffersFor(cache, queuedRenderable, casterPass, lastCacheHash, commandBuffer, false);
    }
    //-----------------------------------------------------------------------------------
    // ============================================================================
    // Bind Ocean textures EVERY frame, not just on movable change
    // ============================================================================
    uint32 HlmsOcean::fillBuffersFor(const HlmsCache* cache,
        const QueuedRenderable& queuedRenderable,
        bool casterPass, uint32 lastCacheHash,
        CommandBuffer* commandBuffer, bool isV1)
    {
        if (mBufferSize == 0)
            return 0;

        if (mCurrentPassBuffer == 0)
        {
            // Return lastCacheHash to indicate we couldn't process this renderable
            // It will be retried on the next frame when buffers are ready
            return lastCacheHash;
        }

        const HlmsOceanDatablock* datablock =
            static_cast<const HlmsOceanDatablock*>(queuedRenderable.renderable->getDatablock());

        // Changed HLMS type? Rebind everything
        if (OGRE_EXTRACT_HLMS_TYPE_FROM_CACHE_HASH(lastCacheHash) != mType)
        {
           

            // 1. Bind pass buffer (slot 0)
            ConstBufferPacked* passBuffer = mPassBuffers[mCurrentPassBuffer - 1];
            *commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer(
                VertexShader, 0, passBuffer, 0, (uint32)passBuffer->getTotalSizeBytes());
            *commandBuffer->addCommand<CbShaderBuffer>() =
                CbShaderBuffer(PixelShader, 0, passBuffer, 0, (uint32)passBuffer->getTotalSizeBytes());

            uint32 constBufferSlot = 3u;

            // 2. Bind light buffers (slots 3,4,5)
            if (mUseLightBuffers)
            {
                ConstBufferPacked* light0Buffer = mLight0Buffers[mCurrentPassBuffer - 1];
                *commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer(
                    VertexShader, 3, light0Buffer, 0, (uint32)light0Buffer->getTotalSizeBytes());
                *commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer(
                    PixelShader, 3, light0Buffer, 0, (uint32)light0Buffer->getTotalSizeBytes());

                ConstBufferPacked* light1Buffer = mLight1Buffers[mCurrentPassBuffer - 1];
                *commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer(
                    VertexShader, 4, light1Buffer, 0, (uint32)light1Buffer->getTotalSizeBytes());
                *commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer(
                    PixelShader, 4, light1Buffer, 0, (uint32)light1Buffer->getTotalSizeBytes());

                ConstBufferPacked* light2Buffer = mLight2Buffers[mCurrentPassBuffer - 1];
                *commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer(
                    VertexShader, 5, light2Buffer, 0, (uint32)light2Buffer->getTotalSizeBytes());
                *commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer(
                    PixelShader, 5, light2Buffer, 0, (uint32)light2Buffer->getTotalSizeBytes());

                constBufferSlot = 6u;
            }

            size_t texUnit = mReservedTexBufferSlots;

            if (!casterPass)
            {
                // 3. Atmosphere const buffers
                constBufferSlot += mAtmosphere->bindConstBuffers(commandBuffer, constBufferSlot);

                // 4. Forward+ buffers
                if (mGridBuffer)
                {
                    *commandBuffer->addCommand<CbShaderBuffer>() =
                        CbShaderBuffer(PixelShader, (uint16)texUnit++, mGlobalLightListBuffer, 0, 0);
                    *commandBuffer->addCommand<CbShaderBuffer>() =
                        CbShaderBuffer(PixelShader, (uint16)texUnit++, mGridBuffer, 0, 0);
                }

                // 5. Skip to after our reserved slots
                texUnit += mReservedTexSlots;

                // Pre-pass textures
                if (!mPrePassTextures->empty())
                {
                    *commandBuffer->addCommand<CbTexture>() =
                        CbTexture((uint16)texUnit++, (*mPrePassTextures)[0], 0);
                    *commandBuffer->addCommand<CbTexture>() =
                        CbTexture((uint16)texUnit++, (*mPrePassTextures)[1], 0);
                }

                if (mPrePassMsaaDepthTexture)
                {
                    *commandBuffer->addCommand<CbTexture>() =
                        CbTexture((uint16)texUnit++, mPrePassMsaaDepthTexture, 0);
                }

                if (mSsrTexture)
                {
                    *commandBuffer->addCommand<CbTexture>() =
                        CbTexture((uint16)texUnit++, mSsrTexture, 0);
                }

                // Irradiance volume
                if (mIrradianceVolume)
                {
                    TextureGpu* irradianceTex = mIrradianceVolume->getIrradianceVolumeTexture();
                    const HlmsSamplerblock* samplerblock = mIrradianceVolume->getIrradSamplerblock();
                    *commandBuffer->addCommand<CbTexture>() =
                        CbTexture((uint16)texUnit, irradianceTex, samplerblock);
                    ++texUnit;
                }

                // Area light masks
                if (mUsingAreaLightMasks)
                {
                    *commandBuffer->addCommand<CbTexture>() =
                        CbTexture((uint16)texUnit, mAreaLightMasks, mAreaLightMasksSamplerblock);
                    ++texUnit;
                }

                if (mLtcMatrixTexture)
                {
                    *commandBuffer->addCommand<CbTexture>() =
                        CbTexture((uint16)texUnit, mLtcMatrixTexture, mAreaLightMasksSamplerblock);
                    ++texUnit;
                }

                // Decals
                for (size_t i = 0; i < 3u; ++i)
                {
                    if (mDecalsTextures[i] && (i != 2u || !mDecalsDiffuseMergedEmissive))
                    {
                        *commandBuffer->addCommand<CbTexture>() =
                            CbTexture((uint16)texUnit, mDecalsTextures[i], mDecalsSamplerblock);
                        ++texUnit;
                    }
                }

                // 6. Shadow maps
                FastArray<TextureGpu*>::const_iterator itor = mPreparedPass.shadowMaps.begin();
                FastArray<TextureGpu*>::const_iterator end = mPreparedPass.shadowMaps.end();
                while (itor != end)
                {
                    *commandBuffer->addCommand<CbTexture>() =
                        CbTexture((uint16)texUnit, *itor, mCurrentShadowmapSamplerblock);
                    ++texUnit;
                    ++itor;
                }

                // 7. Parallax corrected cubemap
                if (mParallaxCorrectedCubemap && !mParallaxCorrectedCubemap->isRendering())
                {
                    TextureGpu* pccTexture = mParallaxCorrectedCubemap->getBindTexture();
                    const HlmsSamplerblock* samplerblock =
                        mParallaxCorrectedCubemap->getBindTrilinearSamplerblock();
                    *commandBuffer->addCommand<CbTexture>() =
                        CbTexture((uint16)texUnit, pccTexture, samplerblock);
                    ++texUnit;
                }
            }

            mLastDescTexture = 0;
            mLastDescSampler = 0;
            mLastMovableObject = 0;
            mLastBoundPool = 0;

            // 8. Bind instance buffer (slot 2)
            if (mCurrentConstBuffer < mConstBuffers.size() &&
                (size_t)((mCurrentMappedConstBuffer - mStartMappedConstBuffer) + 4) <=
                mCurrentConstBufferSize)
            {
                *commandBuffer->addCommand<CbShaderBuffer>() =
                    CbShaderBuffer(VertexShader, 2, mConstBuffers[mCurrentConstBuffer], 0, 0);
                *commandBuffer->addCommand<CbShaderBuffer>() =
                    CbShaderBuffer(PixelShader, 2, mConstBuffers[mCurrentConstBuffer], 0, 0);
            }

            mListener->hlmsTypeChanged(casterPass, commandBuffer, datablock, 0u);
        }

        // 9. Bind material buffer if pool changed (slot 1)
        if (mLastBoundPool != datablock->getAssignedPool())
        {
            const ConstBufferPool::BufferPool* newPool = datablock->getAssignedPool();
            *commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer(VertexShader, 1, newPool->materialBuffer, 0,
                (uint32)newPool->materialBuffer->getTotalSizeBytes());
            *commandBuffer->addCommand<CbShaderBuffer>() =
                CbShaderBuffer(PixelShader, 1, newPool->materialBuffer, 0,
                    (uint32)newPool->materialBuffer->getTotalSizeBytes());
            mLastBoundPool = newPool;
        }

        // ===== BIND PER-DATABLOCK TEXTURES (CRITICAL FOR REFLECTION) =====
        if (!casterPass || datablock->getAlphaTest() != CMPF_ALWAYS_PASS)
        {
            if (datablock->mTexturesDescSet != mLastDescTexture)
            {
                if (datablock->mTexturesDescSet)
                {
                    size_t texUnit = mTexUnitSlotStart;

                    // Bind all textures in descriptor set (including reflection cubemap)
                    *commandBuffer->addCommand<CbTextures>() =
                        CbTextures((uint16)texUnit, std::numeric_limits<uint16>::max(),
                            datablock->mTexturesDescSet);

                    if (!mHasSeparateSamplers)
                    {
                        *commandBuffer->addCommand<CbSamplers>() =
                            CbSamplers((uint16)texUnit, datablock->mSamplersDescSet);
                    }
                }

                mLastDescTexture = datablock->mTexturesDescSet;
            }

            if (datablock->mSamplersDescSet != mLastDescSampler && mHasSeparateSamplers)
            {
                if (datablock->mSamplersDescSet)
                {
                    size_t texUnit = mTexUnitSlotStart;
                    *commandBuffer->addCommand<CbSamplers>() =
                        CbSamplers((uint16)texUnit, datablock->mSamplersDescSet);
                    mLastDescSampler = datablock->mSamplersDescSet;
                }
            }
        }

        // 10. Bind Ocean textures if movable changed
        if (mLastMovableObject != queuedRenderable.movableObject)
        {
            ensureOceanTexturesLoaded();

            if (mOceanTexReady && mOceanDataTex && mWeightTex)
            {
                // Bind at the BEGINNING of texture slots (mTexBufUnitSlotEnd is where F3D ends)
                *commandBuffer->addCommand<CbTexture>() =
                    CbTexture(mTexBufUnitSlotEnd + 0u, mOceanDataTex, mOceanDataSamplerblock);
                *commandBuffer->addCommand<CbTexture>() =
                    CbTexture(mTexBufUnitSlotEnd + 1u, mWeightTex, mWeightSamplerblock);
            }

            mLastMovableObject = queuedRenderable.movableObject;
        }

        // --------------------------------------------------------------------
        // 11. Upload per-cell instance data
        // --------------------------------------------------------------------
        static constexpr uint32 kU32PerCell = 20u;
        static constexpr uint32 kMaxCells = 256u;

        uint32* RESTRICT_ALIAS currentMappedConstBuffer = mCurrentMappedConstBuffer;

        const size_t usedU32 = (size_t)(currentMappedConstBuffer - mStartMappedConstBuffer);
        const size_t usedCells = usedU32 / kU32PerCell;

        const bool mustFlush = (usedCells >= kMaxCells) || (usedU32 + kU32PerCell > mCurrentConstBufferSize);

        if (mustFlush)
        {
            currentMappedConstBuffer = mapNextConstBuffer(commandBuffer);
        }

        const size_t newUsedU32 = (size_t)(currentMappedConstBuffer - mStartMappedConstBuffer);
        const size_t newUsedCells = newUsedU32 / kU32PerCell;

        const uint32 drawId = static_cast<uint32>(newUsedCells);
        OGRE_ASSERT_LOW(drawId < kMaxCells);

        const OceanCell* oceanCell = static_cast<const OceanCell*>(queuedRenderable.renderable);
        oceanCell->uploadToGpu(currentMappedConstBuffer);
        currentMappedConstBuffer += kU32PerCell;

        mCurrentMappedConstBuffer = currentMappedConstBuffer;

        return drawId;
    }

    void HlmsOcean::ensureOceanTexturesLoaded()
    {
        if (mOceanTexReady)
            return;

        TextureGpuManager* texMgr = Root::getSingleton().getRenderSystem()->getTextureGpuManager();

        // Load 3D ocean data
        try
        {
            TextureFlags::TextureFlags flags = TextureFlags::Reinterpretable;

            mOceanDataTex = texMgr->createOrRetrieveTexture(mOceanDataTextureName, GpuPageOutStrategy::Discard, flags,
                TextureTypes::Type3D, ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);

            mOceanDataTex->setNumMipmaps(1);

            if (!mOceanDataTex)
            {
                LogManager::getSingleton().logMessage("[HlmsOcean ERROR] Failed to create ocean data texture", LML_CRITICAL);
                return;
            }

            mOceanDataTex->scheduleTransitionTo(GpuResidency::Resident);

           /* LogManager::getSingleton().logMessage(
                 "[HlmsOcean] oceanData.dds dimensions: " +
                 StringConverter::toString(mOceanDataTex->getWidth()) + "x" +
                 StringConverter::toString(mOceanDataTex->getHeight()) + "x" +
                 StringConverter::toString(mOceanDataTex->getDepth()));*/

            LogManager::getSingleton().logMessage("[HlmsOcean] Ocean data texture created: " + mOceanDataTextureName);

           
        }
        catch (const Exception& e)
        {
            LogManager::getSingleton().logMessage("[HlmsOcean ERROR] Exception loading ocean data: " + e.getDescription(), LML_CRITICAL);
            return;
        }

        // Load 2D weight map
        try
        {
            mWeightTex = texMgr->createOrRetrieveTexture(mWeightTextureName, GpuPageOutStrategy::Discard, static_cast<TextureFlags::TextureFlags>(0),
                TextureTypes::Type2D, ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);

            if (!mWeightTex)
            {
                LogManager::getSingleton().logMessage(
                    "[HlmsOcean ERROR] Failed to create weight texture", LML_CRITICAL);
                return;
            }

            mWeightTex->scheduleTransitionTo(GpuResidency::Resident);

            LogManager::getSingleton().logMessage("[HlmsOcean] Weight texture created: " + mWeightTextureName);
        }
        catch (const Exception& e)
        {
            LogManager::getSingleton().logMessage("[HlmsOcean ERROR] Exception loading weight texture: " + e.getDescription(), LML_CRITICAL);
            return;
        }

        // Wait for loading
        if (mOceanDataTex && mWeightTex)
        {
            texMgr->waitForStreamingCompletion();

            if (mOceanDataTex->getWidth() > 0u && mWeightTex->getWidth() > 0u)
            {
                mOceanTexReady = true;
                LogManager::getSingleton().logMessage("[HlmsOcean] Textures successfully loaded and ready!");
            }
            else
            {
                LogManager::getSingleton().logMessage("[HlmsOcean ERROR] Textures have zero dimensions!", LML_CRITICAL);
            }
        }
    }
    //-----------------------------------------------------------------------------------
    Hlms::PropertiesMergeStatus HlmsOcean::notifyPropertiesMergedPreGenerationStep(
        const size_t tid, PiecesMap* inOutPieces)
    {
        PropertiesMergeStatus status =
            HlmsPbs::notifyPropertiesMergedPreGenerationStep(tid, inOutPieces);

        if (status == PropertiesMergeStatusError)
            return status;

        // Set texture registers AFTER Forward+ slots
        int32 texSlotsStart = 0;
        if (getProperty(tid, HlmsBaseProp::ForwardPlus))
            texSlotsStart = getProperty(tid, "f3dGrid") + 1;

        // ONLY set registers for Ocean-specific shared textures
        setTextureReg(tid, VertexShader, "terrainData", texSlotsStart + 0);
        setTextureReg(tid, VertexShader, "blendMap", texSlotsStart + 1);

        if (!getProperty(tid, HlmsBaseProp::ShadowCaster))
        {
            setTextureReg(tid, PixelShader, "terrainData", texSlotsStart + 0);
            setTextureReg(tid, PixelShader, "blendMap", texSlotsStart + 1);

            // DO NOT set texEnvProbeMap register here - descriptor sets handle it
        }

        return status;
    }
    //-----------------------------------------------------------------------------------
    void HlmsOcean::setDebugPssmSplits(bool bDebug)
    {
        mDebugPssmSplits = bDebug;
    }
    //-----------------------------------------------------------------------------------
    void HlmsOcean::setShadowSettings(ShadowFilter filter)
    {
        mShadowFilter = filter;
    }
    //-----------------------------------------------------------------------------------
    void HlmsOcean::setAmbientLightMode(AmbientLightMode mode)
    {
        mAmbientLightMode = mode;
    }
    //-----------------------------------------------------------------------------------
    void HlmsOcean::setDatablockEnvReflection(HlmsOceanDatablock* datablock, Ogre::TextureGpu* reflectionTexture)
    {
        if (!datablock)
            return;

        // Set or clear the reflection texture on the datablock
        datablock->setTexture(OCEAN_REFLECTION, reflectionTexture);

        // Force shader recompilation for this datablock only
        datablock->flushRenderables();
    }
    //-----------------------------------------------------------------------------------
    void HlmsOcean::setDatablockEnvReflection(HlmsOceanDatablock* datablock, const Ogre::String& textureName)
    {
        if (!datablock)
            return;

        LogManager::getSingleton().logMessage(
            "[HlmsOcean::setDatablockEnvReflection] Setting reflection: " +
            (textureName.empty() ? "(clearing)" : textureName));

        // Clear reflection
        if (textureName.empty())
        {
            datablock->setTexture(OCEAN_REFLECTION, nullptr);
            // Force shader recompilation for this datablock only
            datablock->flushRenderables();
            LogManager::getSingleton().logMessage("[HlmsOcean] Reflection cleared, flushed renderables");
            return;
        }

        // Check if texture exists
        if (!Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(textureName))
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(
                Ogre::LML_CRITICAL,
                "[HlmsOcean] ERROR: Reflection cubemap does not exist: " + textureName);
            return;
        }

        Ogre::TextureGpuManager* textureManager =
            Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();

        Ogre::uint32 textureFlags = Ogre::TextureFlags::PrefersLoadingFromFileAsSRGB;
        textureFlags &= ~Ogre::TextureFlags::AutomaticBatching;

        Ogre::uint32 filters = Ogre::TextureFilter::TypeGenerateDefaultMipmaps;

        Ogre::TextureGpu* cubemap = textureManager->createOrRetrieveTexture(
            textureName,
            Ogre::GpuPageOutStrategy::SaveToSystemRam,
            textureFlags,
            Ogre::TextureTypes::TypeCube,
            Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
            filters,
            0u);

        if (!cubemap)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(
                Ogre::LML_CRITICAL,
                "[HlmsOcean] ERROR: Failed to create reflection cubemap: " + textureName);
            return;
        }

        LogManager::getSingleton().logMessage("[HlmsOcean] Scheduling transition to Resident...");
        cubemap->scheduleTransitionTo(Ogre::GpuResidency::Resident);

        LogManager::getSingleton().logMessage("[HlmsOcean] Waiting for streaming...");
        textureManager->waitForStreamingCompletion();

        // Verify it loaded
        LogManager::getSingleton().logMessage(
            "[HlmsOcean] Cubemap loaded - Type: " +
            StringConverter::toString(cubemap->getTextureType()) +
            " Width: " + StringConverter::toString(cubemap->getWidth()) +
            " Height: " + StringConverter::toString(cubemap->getHeight()));

        if (cubemap->getWidth() == 0 || cubemap->getTextureType() != Ogre::TextureTypes::TypeCube)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(
                Ogre::LML_CRITICAL,
                "[HlmsOcean] ERROR: Cubemap invalid after loading!");
            return;
        }

        // Set texture on datablock
        datablock->setTexture(OCEAN_REFLECTION, cubemap);

        datablock->flushRenderables();
    }
#if !OGRE_NO_JSON
    //-----------------------------------------------------------------------------------
    void HlmsOcean::_loadJson(const rapidjson::Value& jsonValue, const HlmsJson::NamedBlocks& blocks,
        HlmsDatablock* datablock, const String& resourceGroup,
        HlmsJsonListener* listener,
        const String& additionalTextureExtension) const
    {
        HlmsJsonOcean jsonOcean(mHlmsManager, mRenderSystem->getTextureGpuManager());
        jsonOcean.loadMaterial(jsonValue, blocks, datablock, resourceGroup);
    }
    //-----------------------------------------------------------------------------------
    void HlmsOcean::_saveJson(const HlmsDatablock* datablock, String& outString,
        HlmsJsonListener* listener,
        const String& additionalTextureExtension) const
    {
        //        HlmsJsonOcean jsonOCean( mHlmsManager );
        //        jsonOCean.saveMaterial( datablock, outString );
    }
    //-----------------------------------------------------------------------------------
    void HlmsOcean::_collectSamplerblocks(set<const HlmsSamplerblock*>::type& outSamplerblocks,
        const HlmsDatablock* datablock) const
    {
        HlmsJsonOcean::collectSamplerblocks(datablock, outSamplerblocks);
    }
#endif
    //-----------------------------------------------------------------------------------
    HlmsDatablock* HlmsOcean::createDatablockImpl(IdString datablockName, const HlmsMacroblock* macroblock, const HlmsBlendblock* blendblock, const HlmsParamVec& paramVec)
    {
        return OGRE_NEW HlmsOceanDatablock(datablockName, this, macroblock, blendblock, paramVec);
    }
    //-----------------------------------------------------------------------------------
    void HlmsOcean::setTextureProperty(size_t tid, const char* propertyName, HlmsOceanDatablock* datablock, OceanTextureTypes texType)
    {
        uint8 idx = datablock->getIndexToDescriptorTexture(texType);
        if (idx != NUM_OCEAN_TEXTURE_TYPES)
        {
            char tmpData[64];
            LwString propName = LwString::FromEmptyPointer(tmpData, sizeof(tmpData));

            propName = propertyName;  // "envprobe_map"
            const size_t basePropSize = propName.size();

            // In the template we subtract the "+1" for the index.
            // We need to increment it now otherwise @property( envprobe_map )
            // can translate to @property( 0 ) which is not what we want.
            setProperty(tid, propertyName, idx + 1);

            propName.resize(basePropSize);
            propName.a("_idx");  // "envprobe_map_idx"
            setProperty(tid, propName.c_str(), idx);

            if (mHasSeparateSamplers)
            {
                const uint8 samplerIdx = datablock->getIndexToDescriptorSampler(texType);
                propName.resize(basePropSize);
                propName.a("_sampler");  // "envprobe_map_sampler"
                setProperty(tid, propName.c_str(), samplerIdx);
            }
        }
    }
}

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
#include "OgreHlmsManager.h"
#include "OgreHlmsListener.h"
#include "OgreLwString.h"

#include "OgreForward3D.h"
#include "OgreCamera.h"
#include "OgreAtmosphereComponent.h"
#include <OgreRoot.h>
#include <OgreTextureGpuManager.h>

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
    const IdString OceanProperty::HwGammaRead = IdString("hw_gamma_read");
    const IdString OceanProperty::HwGammaWrite = IdString("hw_gamma_write");
    const IdString OceanProperty::SignedIntTex = IdString("signed_int_textures");
    const IdString OceanProperty::MaterialsPerBuffer = IdString("materials_per_buffer");
    const IdString OceanProperty::DebugPssmSplits = IdString("debug_pssm_splits");
    const IdString OceanProperty::AtmosphereNumConstBuffers = IdString("atmosphere_num_const_buffers");

    const IdString OceanProperty::UseSkirts = IdString("use_skirts");

    const char* OceanProperty::EnvProbeMap = "envprobe_map";

    const IdString OceanProperty::FresnelScalar = IdString("fresnel_scalar");
    const IdString OceanProperty::MetallicWorkflow = IdString("metallic_workflow");
    const IdString OceanProperty::ReceiveShadows = IdString("receive_shadows");

    const IdString OceanProperty::Pcf3x3 = IdString("pcf_3x3");
    const IdString OceanProperty::Pcf4x4 = IdString("pcf_4x4");
    const IdString OceanProperty::PcfIterations = IdString("pcf_iterations");

    const IdString OceanProperty::EnvMapScale = IdString("envmap_scale");
    const IdString OceanProperty::AmbientFixed = IdString("ambient_fixed");
    const IdString OceanProperty::AmbientHemisphere = IdString("ambient_hemisphere");

    const IdString OceanProperty::BrdfDefault = IdString("BRDF_Default");
    const IdString OceanProperty::BrdfCookTorrance = IdString("BRDF_CookTorrance");
    const IdString OceanProperty::BrdfBlinnPhong = IdString("BRDF_BlinnPhong");
    const IdString OceanProperty::FresnelSeparateDiffuse = IdString("fresnel_separate_diffuse");
    const IdString OceanProperty::GgxHeightCorrelated = IdString("GGX_height_correlated");

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
        mCurrentPassBuffer(0),
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

        // IMPORTANT: Ocean datablock size (Terra does this too)
        mBytesPerSlot = HlmsOceanDatablock::MaterialSizeInGpuAligned;
        mOptimizationStrategy = LowerGpuOverhead;
        mSetupWorldMatBuf = false;
        mReservedTexBufferSlots = 0u;

        // Ocean uses 2 “built-in” textures in your code: terrainData + blendMap
        mReservedTexSlots = 2u;

        // Terra uses this to stop base from auto-requesting slots; keep same behavior
        mSkipRequestSlotInChangeRS = true;

        // Your defaults
        mLightGatheringMode = LightGatherForwardPlus;
        mProbe = 0;
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
    void HlmsOcean::analyzeBarriers(BarrierSolver& barrierSolver, ResourceTransitionArray& resourceTransitions, Camera* renderingCamera, const bool bCasterPass)
    {
        // Ocean is currently GLSL-only in this integration and relies on the same
        // resource usage patterns as PBS/Terra. If you migrate the remaining legacy
        // v1 textures (oceanData.dds / weight.dds) to TextureGpu, you may want to
        // add explicit transitions here.
        (void)barrierSolver;
        (void)resourceTransitions;
        (void)renderingCamera;
        (void)bCasterPass;
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

        // 2. Ocean EARLY hooks (must be before Terra!)
        outLibraryFoldersPaths.push_back("Hlms/Ocean/Any");

        // 3. PBS (BRDF, shadows, lighting)
        outLibraryFoldersPaths.push_back("Hlms/Pbs/Any");
        outLibraryFoldersPaths.push_back("Hlms/Pbs/Any/Main");
        outLibraryFoldersPaths.push_back("Hlms/Pbs/Any/Atmosphere");
        outLibraryFoldersPaths.push_back("Hlms/Pbs/" + shaderSyntax);

        // 4. Terra (CellData, grid rendering)
        outLibraryFoldersPaths.push_back("Hlms/Terra/Any");
        outLibraryFoldersPaths.push_back("Hlms/Terra/" + shaderSyntax);
        // Causes pixelshader trouble 
        // outLibraryFoldersPaths.push_back("Hlms/Terra/" + shaderSyntax + "/PbsTerraShadows");

        // 5. Ocean HLSL custom + main
        outLibraryFoldersPaths.push_back("Hlms/Ocean/" + shaderSyntax + "/Custom");
        
        outDataFolderPath = "Hlms/Ocean/" + shaderSyntax;
    }

    void HlmsOcean::validatePassBufferState() const
    {
        LogManager::getSingleton().logMessage(
            "[HlmsOcean] Pass Buffer Diagnostics:",
            LML_NORMAL);

        LogManager::getSingleton().logMessage(
            "  Current pass buffer index: " +
            StringConverter::toString(mCurrentPassBuffer),
            LML_NORMAL);

        LogManager::getSingleton().logMessage(
            "  Total pass buffers: " +
            StringConverter::toString(mPassBuffers.size()),
            LML_NORMAL);

        if (mCurrentPassBuffer > 0 && mCurrentPassBuffer <= mPassBuffers.size())
        {
            ConstBufferPacked* buffer = mPassBuffers[mCurrentPassBuffer - 1];

            LogManager::getSingleton().logMessage(
                "  Last allocated size: " +
                StringConverter::toString(mLastAllocatedPassBufferSize),
                LML_NORMAL);

            LogManager::getSingleton().logMessage(
                "  Last written size: " +
                StringConverter::toString(mLastWrittenPassBufferSize),
                LML_NORMAL);

            LogManager::getSingleton().logMessage(
                "  Buffer total size: " +
                StringConverter::toString(buffer->getTotalSizeBytes()),
                LML_NORMAL);

            LogManager::getSingleton().logMessage(
                "  Mapping state: " +
                StringConverter::toString(buffer->getMappingState()),
                LML_NORMAL);

            if (mLastWrittenPassBufferSize > mLastAllocatedPassBufferSize)
            {
                LogManager::getSingleton().logMessage(
                    "  ERROR: Buffer overflow detected!",
                    LML_CRITICAL);
            }
        }

        LogManager::getSingleton().logMessage(
            "  Atmosphere enabled: " +
            StringConverter::toString(mAtmosphere != nullptr),
            LML_NORMAL);

        if (mAtmosphere)
        {
            LogManager::getSingleton().logMessage(
                "  Atmosphere provides code: " +
                StringConverter::toString(mAtmosphere->providesHlmsCode()),
                LML_NORMAL);
        }
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
            samplerblock.mMipFilter = FO_NONE;

            if (!mShadowmapSamplerblock)
            {
                mShadowmapSamplerblock = mHlmsManager->getSamplerblock(samplerblock);
            }
        }

        samplerblock.mMinFilter = FO_LINEAR;
        samplerblock.mMagFilter = FO_LINEAR;
        samplerblock.mMipFilter = FO_NONE;
        samplerblock.mCompareFunction = CMPF_LESS_EQUAL;

        if (!mShadowmapCmpSamplerblock)
        {
            mShadowmapCmpSamplerblock = mHlmsManager->getSamplerblock(samplerblock);
        }

        // --- Ocean samplers (IMPORTANT: create once, reuse forever) ---
        // Ocean data volume: point sampling is usually correct for “data textures”
        if (!mOceanDataSamplerblock)
		{
            HlmsSamplerblock sb;
            sb.mU = TAM_WRAP;
            sb.mV = TAM_WRAP;
            sb.mW = TAM_WRAP;

            sb.mMinFilter = FO_LINEAR;
            sb.mMagFilter = FO_LINEAR;
            sb.mMipFilter = FO_NONE;
            sb.mMaxAnisotropy = 8.0f; 

            mOceanDataSamplerblock = mHlmsManager->getSamplerblock(sb);
		}

		// Weight map: typically linear is fine
		if (!mWeightSamplerblock)
		{
            HlmsSamplerblock sb;
            sb.mU = TAM_WRAP;
            sb.mV = TAM_WRAP;
            sb.mW = TAM_WRAP;

            sb.mMinFilter = FO_LINEAR;
            sb.mMagFilter = FO_LINEAR;
            sb.mMipFilter = FO_LINEAR;
            sb.mMaxAnisotropy = 8.0f;

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

            if (getProperty(tid, OceanProperty::EnvProbeMap))
            {
                psParams->setNamedConstant("texEnvProbeMap", texUnit++);
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
        OceanCell* cell = static_cast<OceanCell*>(renderable);
        HlmsOceanDatablock* datablock =
            static_cast<HlmsOceanDatablock*>(renderable->getDatablock());

        setProperty(kNoTid, OceanProperty::UseSkirts, cell->getUseSkirts());

        // ---- PBS core (DO NOT REMOVE) ----
        setProperty(kNoTid, PbsProperty::FresnelScalar, 1);
        setProperty(kNoTid, PbsProperty::FresnelWorkflow, 0);
        setProperty(kNoTid, PbsProperty::MetallicWorkflow, 1);
        setProperty(kNoTid, PbsProperty::ReceiveShadows, 1);
        setProperty(kNoTid, "hlms_vpos", 1);
        setProperty(kNoTid, "skip_normal_offset_bias_vs", 1);

        // ---- BRDF ----
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

        // ---- Env probe hook (like Terra) ----
        if (mProbe)
        {
            setProperty(kNoTid, PbsProperty::NumTextures, 1);
            setTextureProperty(kNoTid, PbsProperty::EnvProbeMap, dynamic_cast<HlmsPbsDatablock*>(datablock), PBSM_REFLECTION);

            setProperty(kNoTid, PbsProperty::EnvProbeMap, static_cast<int32>(mProbe->getName().getU32Value()));
        }

        // Ocean uses procedural normals
        setProperty(kNoTid, PbsProperty::NormalMap, 0);
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
    }
    //-----------------------------------------------------------------------------------
    HlmsCache HlmsOcean::preparePassHash(const CompositorShadowNode* shadowNode, bool casterPass, bool dualParaboloid, SceneManager* sceneManager)
    {
        mT[kNoTid].setProperties.clear();

        // Get atmosphere FIRST (before any properties)
        mAtmosphere = sceneManager ? sceneManager->getAtmosphere() : nullptr;

        // Set atmosphere properties EARLY (before preparePassHashBase)
        if (!casterPass && mAtmosphere && mAtmosphere->providesHlmsCode())
        {
            // These properties enable the shader pieces
            setProperty(kNoTid, "hlms_atmosphere", 1);
            setProperty(kNoTid, "hlms_aerial_perspective", 1);

            // Fog support (Terra-style)
            const FogMode fogMode = sceneManager->getFogMode();
            if (fogMode != FOG_NONE)
            {
                setProperty(kNoTid, "hlms_fog", 1);
                if (fogMode == FOG_EXP)
                    setProperty(kNoTid, "hlms_fog_exp", 1);
                else if (fogMode == FOG_EXP2)
                    setProperty(kNoTid, "hlms_fog_exp2", 1);
                else if (fogMode == FOG_LINEAR)
                    setProperty(kNoTid, "hlms_fog_linear", 1);
            }
        }

        if (casterPass)
        {
            HlmsCache retVal = Hlms::preparePassHashBase(shadowNode, casterPass, dualParaboloid, sceneManager);
            return retVal;
        }

        // --- Env probe setup ---
		const bool hasProbe = (mProbe != nullptr);
		setProperty(kNoTid, OceanProperty::EnvProbeMap, hasProbe ? 1 : 0);

		// Atmosphere setup BEFORE preparePassHashBase ---
		uint32 atmosphereCbCount = 0u;
		uint32 numPassConstBuffers = 3u; // pass=0, material=1, instance=2
		if (mAtmosphere && mAtmosphere->providesHlmsCode())
		{
			// Atmosphere const buffers start at slot 3
			atmosphereCbCount = mAtmosphere->preparePassHash(this, 3u);
			numPassConstBuffers += atmosphereCbCount;
		}
		setProperty(kNoTid, OceanProperty::AtmosphereNumConstBuffers, static_cast<int32>(atmosphereCbCount));
		// This is what the shared HLMS pieces (Terra/PBS/Atmosphere) often key off.
		setProperty(kNoTid, PbsProperty::NumPassConstBuffers, static_cast<int32>(numPassConstBuffers));

        // Shadow setup
        if (shadowNode && !casterPass)
        {
            const RenderSystemCapabilities* capabilities = mRenderSystem->getCapabilities();
            if (capabilities->hasCapability(RSC_TEXTURE_GATHER))
            {
                setProperty(kNoTid, HlmsBaseProp::TexGather, 1);
            }

            if (mShadowFilter == PCF_3x3)
            {
                setProperty(kNoTid, OceanProperty::Pcf3x3, 1);
                setProperty(kNoTid, OceanProperty::PcfIterations, 4);
            }
            else if (mShadowFilter == PCF_4x4)
            {
                setProperty(kNoTid, OceanProperty::Pcf4x4, 1);
                setProperty(kNoTid, OceanProperty::PcfIterations, 9);
            }
            else
            {
                setProperty(kNoTid, OceanProperty::PcfIterations, 1);
            }

            if (mDebugPssmSplits)
            {
                int32 numPssmSplits = 0;
                const vector<Real>::type* pssmSplits = shadowNode->getPssmSplits(0);
                if (pssmSplits)
                {
                    numPssmSplits = static_cast<int32>(pssmSplits->size() - 1);
                    if (numPssmSplits > 0)
                    {
                        setProperty(kNoTid, OceanProperty::DebugPssmSplits, 1);
                    }
                }
            }
        }

        // Ambient light setup
        AmbientLightMode ambientMode = mAmbientLightMode;
        ColourValue upperHemisphere = sceneManager->getAmbientLightUpperHemisphere();
        ColourValue lowerHemisphere = sceneManager->getAmbientLightLowerHemisphere();
        const float envMapScale = upperHemisphere.a;
        upperHemisphere.a = lowerHemisphere.a = 1.0;

        if (!casterPass)
        {
            if (mAmbientLightMode == AmbientAuto)
            {
                if (upperHemisphere == lowerHemisphere)
                {
                    ambientMode = (upperHemisphere == ColourValue::Black) ?
                        AmbientNone : AmbientFixed;
                }
                else
                {
                    ambientMode = AmbientHemisphere;
                }
            }

            if (ambientMode == AmbientFixed)
                setProperty(kNoTid, OceanProperty::AmbientFixed, 1);
            if (ambientMode == AmbientHemisphere)
                setProperty(kNoTid, OceanProperty::AmbientHemisphere, 1);
            if (envMapScale != 1.0f)
                setProperty(kNoTid, OceanProperty::EnvMapScale, 1);
        }

        setProperty(kNoTid, OceanProperty::FresnelScalar, 1);
        setProperty(kNoTid, OceanProperty::MetallicWorkflow, 1);

        // Call base implementation
        HlmsCache retVal = Hlms::preparePassHashBase(shadowNode, casterPass,
            dualParaboloid, sceneManager);

        // Adjust directional lights for shadows
        if (getProperty(kNoTid, HlmsBaseProp::LightsDirNonCaster) > 0)
        {
            int32 shadowCasterDirectional = getProperty(kNoTid, HlmsBaseProp::LightsDirectional);
            shadowCasterDirectional = std::max(shadowCasterDirectional, 1);
            setProperty(kNoTid, HlmsBaseProp::LightsDirectional, shadowCasterDirectional);
        }

        // Get viewport and capabilities
        Viewport* viewport = mRenderSystem->getCurrentRenderViewports();
        const RenderSystemCapabilities* capabilities = mRenderSystem->getCapabilities();
        setProperty(kNoTid, OceanProperty::HwGammaRead,
            capabilities->hasCapability(RSC_HW_GAMMA));
        setProperty(kNoTid, OceanProperty::HwGammaWrite,
            capabilities->hasCapability(RSC_HW_GAMMA));
        setProperty(kNoTid, OceanProperty::SignedIntTex,
            capabilities->hasCapability(RSC_TEXTURE_SIGNED_INT));

        retVal.setProperties = mT[kNoTid].setProperties;

        auto camerasInProgress = sceneManager->getCamerasInProgress();
        const Camera* camera = camerasInProgress.renderingCamera;
        Matrix4 viewMatrix = camera->getViewMatrix(true);
        Matrix4 projectionMatrix = camera->getProjectionMatrixWithRSDepth();

        TextureGpu* renderTarget = viewport->getCurrentTarget();
        if (renderTarget->requiresTextureFlipping())
        {
            projectionMatrix[1][0] = -projectionMatrix[1][0];
            projectionMatrix[1][1] = -projectionMatrix[1][1];
            projectionMatrix[1][2] = -projectionMatrix[1][2];
            projectionMatrix[1][3] = -projectionMatrix[1][3];
        }

        // Get light counts
        int32 numLights = getProperty(kNoTid, HlmsBaseProp::LightsSpot);
        int32 numDirectionalLights = getProperty(kNoTid, HlmsBaseProp::LightsDirNonCaster);
        int32 numShadowMapLights = getProperty(kNoTid, HlmsBaseProp::NumShadowMapLights);
        int32 numPssmSplits = getProperty(kNoTid, HlmsBaseProp::PssmSplits);

        size_t mapSize = 0;

        // mat4 viewProj + mat4 view
        mapSize += (16 + 16) * 4;

        // Forward+ buffers
        mGridBuffer = nullptr;
        mGlobalLightListBuffer = nullptr;
        ForwardPlusBase* forwardPlus = sceneManager->_getActivePassForwardPlus();
        if (forwardPlus)
        {
            mapSize += forwardPlus->getConstBufferSize();
            mGridBuffer = forwardPlus->getGridBuffer(camera);
            mGlobalLightListBuffer = forwardPlus->getGlobalLightListBuffer(camera);
        }

        // Shadow map data: mat4 + vec2 + vec2 per shadow map
        mapSize += ((16 + 2 + 2 + 4) * numShadowMapLights) * 4;

        // mat3 invViewMatCubemap (3 vec4s)
        mapSize += (4 * 3) * 4;

        // Ambient lighting
        if (ambientMode == AmbientFixed || ambientMode == AmbientHemisphere ||
            envMapScale != 1.0f)
        {
            mapSize += 4 * 4; // vec3 + padding
        }
        if (ambientMode == AmbientHemisphere)
        {
            mapSize += 8 * 4; // vec3 + padding + vec3 + padding
        }

        // PSSM splits
        mapSize += numPssmSplits * 4;
        mapSize = alignToNextMultiple<uint32>(mapSize, 16);

        // Light data
        if (shadowNode)
        {
            // 6 variables * 4 (padded vec3) * 4 (bytes) * numLights
            mapSize += (6 * 4 * 4) * numLights;
        }
        else
        {
            // 3 variables * 4 (padded vec3) * 4 (bytes) * numDirectionalLights
            mapSize += (3 * 4 * 4) * numDirectionalLights;
        }

        mapSize = alignToNextMultiple<uint32>(mapSize, 16);

        mapSize += mListener->getPassBufferSize(shadowNode, casterPass,
            dualParaboloid, sceneManager);
        mapSize = alignToNextMultiple<uint32>(mapSize, 16);

		//// Add atmosphere buffer size if present.
		//// We align the start of the atmosphere region to 256 bytes (safe for D3D12 CBV rules
		//// and matches how Atmosphere usually expects packing).
		//if (mAtmosphere && mAtmosphere->providesHlmsCode() && atmosphereCbCount)
		//{
		//	mapSize = alignToNextMultiple<uint32>(mapSize, 256u);
		//	mapSize += size_t(atmosphereCbCount) * 256u;
		//	mapSize = alignToNextMultiple<uint32>(mapSize, 16u);
		//}

		// Create/get pass buffer with correct size
        const size_t maxBufferSize = std::max<size_t>(8 * 1024, mapSize);

        if (mCurrentPassBuffer >= mPassBuffers.size())
        {
            mPassBuffers.push_back(mVaoManager->createConstBuffer(maxBufferSize, BT_DYNAMIC_PERSISTENT, 0, false));
        }

        ConstBufferPacked* passBuffer = mPassBuffers[mCurrentPassBuffer++];

        float* passBufferPtr = reinterpret_cast<float*>(passBuffer->map(0, mapSize));
        const float* startupPtr = passBufferPtr;

        // Clear mapped region for deterministic behavior
        const size_t numFloatsToClear = mapSize / sizeof(float);
        std::fill_n(passBufferPtr, numFloatsToClear, 0.0f);

        // Helper to align to 16-byte boundaries
        auto alignPassPtr16 = [&](float*& ptr)
            {
                const size_t bytesSoFar = size_t(ptr - startupPtr) * sizeof(float);
                const size_t alignedBytes = alignToNextMultiple<uint32>(
                    static_cast<uint32>(bytesSoFar), 16u);
                const size_t padBytes = alignedBytes - bytesSoFar;
                const size_t padFloats = padBytes / sizeof(float);
                for (size_t i = 0; i < padFloats; ++i)
                    *ptr++ = 0.0f;
            };

        // Helper to align to 256-byte boundaries (important before atmosphere CB region)
		auto alignPassPtr256 = [&](float*& ptr)
			{
				const size_t bytesSoFar = size_t(ptr - startupPtr) * sizeof(float);
				const size_t alignedBytes = alignToNextMultiple<uint32>(static_cast<uint32>(bytesSoFar), 256u);
				const size_t padBytes = alignedBytes - bytesSoFar;
				const size_t padFloats = padBytes / sizeof(float);
				for (size_t i = 0; i < padFloats; ++i)
				{
					*ptr++ = 0.0f;
				}
			};

        // ===== VERTEX SHADER DATA =====

        // mat4 viewProj
        Matrix4 viewProjMatrix = projectionMatrix * viewMatrix;
        for (size_t i = 0; i < 16; ++i)
            *passBufferPtr++ = (float)viewProjMatrix[0][i];

        mPreparedPass.viewMatrix = viewMatrix;
        mPreparedPass.shadowMaps.clear();

        // mat4 view
        for (size_t i = 0; i < 16; ++i)
            *passBufferPtr++ = (float)viewMatrix[0][i];

        // Shadow map data
        size_t shadowMapTexIdx = 0;
        const auto& contiguousShadowMapTex = shadowNode ?
            shadowNode->getContiguousShadowMapTex() :
            TextureGpuVec();

        if (shadowNode)
        {
            for (int32 i = 0; i < numShadowMapLights; ++i)
            {
                while (!shadowNode->isShadowMapIdxActive(shadowMapTexIdx))
                    ++shadowMapTexIdx;

                // mat4 shadowRcv[i].texViewProj
                Matrix4 viewProjTex = shadowNode->getViewProjectionMatrix(shadowMapTexIdx);
                for (size_t j = 0; j < 16; ++j)
                    *passBufferPtr++ = (float)viewProjTex[0][j];

                // vec2 shadowDepthRange
                Real fNear, fFar;
                shadowNode->getMinMaxDepthRange(shadowMapTexIdx, fNear, fFar);
                const Real depthRange = fFar - fNear;
                *passBufferPtr++ = fNear;
                *passBufferPtr++ = 1.0f / depthRange;
                *passBufferPtr++ = 0.0f; // Padding
                *passBufferPtr++ = 0.0f;

                // vec2 invShadowMapSize
                size_t shadowMapTexContigIdx =
                    shadowNode->getIndexToContiguousShadowMapTex(shadowMapTexIdx);

                const uint32 texWidth = contiguousShadowMapTex[shadowMapTexContigIdx]->getWidth();
                const uint32 texHeight = contiguousShadowMapTex[shadowMapTexContigIdx]->getHeight();
                *passBufferPtr++ = 1.0f / texWidth;
                *passBufferPtr++ = 1.0f / texHeight;
                *passBufferPtr++ = static_cast<float>(texWidth);
                *passBufferPtr++ = static_cast<float>(texHeight);

                ++shadowMapTexIdx;
            }
        }

        // ===== PIXEL SHADER DATA =====

        // mat3 invViewMatCubemap
        Matrix3 viewMatrix3, invViewMatrixCubemap;
        viewMatrix.extract3x3Matrix(viewMatrix3);
        invViewMatrixCubemap = viewMatrix3;
        invViewMatrixCubemap[0][2] = -invViewMatrixCubemap[0][2];
        invViewMatrixCubemap[1][2] = -invViewMatrixCubemap[1][2];
        invViewMatrixCubemap[2][2] = -invViewMatrixCubemap[2][2];
        invViewMatrixCubemap = invViewMatrixCubemap.Inverse();

        for (size_t i = 0; i < 9; ++i)
        {
            *passBufferPtr++ = (float)invViewMatrixCubemap[0][i];
            if (!((i + 1) % 3))
                ++passBufferPtr; // Padding to vec4
        }

        // Ambient data
        if (ambientMode == AmbientFixed || ambientMode == AmbientHemisphere ||
            envMapScale != 1.0f)
        {
            *passBufferPtr++ = static_cast<float>(upperHemisphere.r);
            *passBufferPtr++ = static_cast<float>(upperHemisphere.g);
            *passBufferPtr++ = static_cast<float>(upperHemisphere.b);
            *passBufferPtr++ = envMapScale;
        }

        if (ambientMode == AmbientHemisphere)
        {
            *passBufferPtr++ = static_cast<float>(lowerHemisphere.r);
            *passBufferPtr++ = static_cast<float>(lowerHemisphere.g);
            *passBufferPtr++ = static_cast<float>(lowerHemisphere.b);
            *passBufferPtr++ = 1.0f;

            Vector3 hemisphereDir = viewMatrix3 *
                sceneManager->getAmbientLightHemisphereDir();
            hemisphereDir.normalise();
            *passBufferPtr++ = static_cast<float>(hemisphereDir.x);
            *passBufferPtr++ = static_cast<float>(hemisphereDir.y);
            *passBufferPtr++ = static_cast<float>(hemisphereDir.z);
            *passBufferPtr++ = 1.0f;
        }

        // PSSM splits
        for (int32 i = 0; i < numPssmSplits; ++i)
            *passBufferPtr++ = (*shadowNode->getPssmSplits(0))[i + 1];

        passBufferPtr += alignToNextMultiple(numPssmSplits, 4) - numPssmSplits;

        // Light data (directional and shadow-casting lights)
        if (numShadowMapLights > 0)
        {
            size_t shadowLightIdx = 0;
            size_t nonShadowLightIdx = 0;
            const LightListInfo& globalLightList = sceneManager->getGlobalLightList();
            const LightClosestArray& lights = shadowNode->getShadowCastingLights();
            const CompositorShadowNode::LightsBitSet& affectedLights =
                shadowNode->getAffectedLightsBitSet();

            int32 shadowCastingDirLights = getProperty(kNoTid,
                HlmsBaseProp::LightsDirectional);

            for (int32 i = 0; i < numLights; ++i)
            {
                Light const* light = nullptr;

                if (i >= shadowCastingDirLights && i < numDirectionalLights)
                {
                    while (affectedLights[nonShadowLightIdx])
                        ++nonShadowLightIdx;
                    light = globalLightList.lights[nonShadowLightIdx++];
                }
                else
                {
                    while (!lights[shadowLightIdx].light)
                        ++shadowLightIdx;
                    light = lights[shadowLightIdx++].light;
                }

                Vector4 lightPos4 = light->getAs4DVector();
                Vector3 lightPos;

                if (light->getType() == Light::LT_DIRECTIONAL)
                    lightPos = viewMatrix3 * Vector3(lightPos4.x, lightPos4.y, lightPos4.z);
                else
                    lightPos = viewMatrix * Vector3(lightPos4.x, lightPos4.y, lightPos4.z);

                // vec3 position + padding
                *passBufferPtr++ = lightPos.x;
                *passBufferPtr++ = lightPos.y;
                *passBufferPtr++ = lightPos.z;
                ++passBufferPtr;

                // vec3 diffuse + padding
                ColourValue color = light->getDiffuseColour() * light->getPowerScale();
                *passBufferPtr++ = color.r;
                *passBufferPtr++ = color.g;
                *passBufferPtr++ = color.b;
                ++passBufferPtr;

                // vec3 specular + padding
                color = light->getSpecularColour() * light->getPowerScale();
                *passBufferPtr++ = color.r;
                *passBufferPtr++ = color.g;
                *passBufferPtr++ = color.b;
                ++passBufferPtr;

                // vec3 attenuation + padding
                *passBufferPtr++ = light->getAttenuationRange();
                *passBufferPtr++ = light->getAttenuationLinear();
                *passBufferPtr++ = light->getAttenuationQuadric();
                ++passBufferPtr;

                // vec3 spotDirection + padding
                Vector3 spotDir = viewMatrix3 * light->getDerivedDirection();
                *passBufferPtr++ = spotDir.x;
                *passBufferPtr++ = spotDir.y;
                *passBufferPtr++ = spotDir.z;
                ++passBufferPtr;

                // vec3 spotParams + padding
                Radian innerAngle = light->getSpotlightInnerAngle();
                Radian outerAngle = light->getSpotlightOuterAngle();
                *passBufferPtr++ = 1.0f / (cosf(innerAngle.valueRadians() * 0.5f) -
                    cosf(outerAngle.valueRadians() * 0.5f));
                *passBufferPtr++ = cosf(outerAngle.valueRadians() * 0.5f);
                *passBufferPtr++ = light->getSpotlightFalloff();
                ++passBufferPtr;
            }
        }
        else
        {
            // Only directional lights (no shadow maps)
            const LightListInfo& globalLightList = sceneManager->getGlobalLightList();

            for (int32 i = 0; i < numDirectionalLights; ++i)
            {
                Vector4 lightPos4 = globalLightList.lights[i]->getAs4DVector();
                Vector3 lightPos = viewMatrix3 *
                    Vector3(lightPos4.x, lightPos4.y, lightPos4.z);

                *passBufferPtr++ = lightPos.x;
                *passBufferPtr++ = lightPos.y;
                *passBufferPtr++ = lightPos.z;
                ++passBufferPtr;

                ColourValue color = globalLightList.lights[i]->getDiffuseColour() *
                    globalLightList.lights[i]->getPowerScale();
                *passBufferPtr++ = color.r;
                *passBufferPtr++ = color.g;
                *passBufferPtr++ = color.b;
                ++passBufferPtr;

                color = globalLightList.lights[i]->getSpecularColour() *
                    globalLightList.lights[i]->getPowerScale();
                *passBufferPtr++ = color.r;
                *passBufferPtr++ = color.g;
                *passBufferPtr++ = color.b;
                ++passBufferPtr;
            }
        }

        // Store shadow maps
        if (shadowNode)
        {
            mPreparedPass.shadowMaps.reserve(contiguousShadowMapTex.size());
            for (auto* tex : contiguousShadowMapTex)
                mPreparedPass.shadowMaps.push_back(tex);
        }

        // Forward+ data
        if (forwardPlus)
        {
            RenderPassDescriptor* renderPassDesc =
                mRenderSystem->getCurrentPassDescriptor();
            const bool isInstancedStereo = false;
            forwardPlus->fillConstBufferData(
                sceneManager->getCurrentViewport0(),
                renderPassDesc->requiresTextureFlipping(),
                renderTarget->getHeight(),
                mShaderSyntax,
                isInstancedStereo,
                passBufferPtr);

            passBufferPtr += forwardPlus->getConstBufferSize() >> 2;
        }

        // Align before custom listener data
        alignPassPtr16(passBufferPtr);

        // Let listener fill its data
        if (mListener)
        {
            passBufferPtr = mListener->preparePassBuffer(shadowNode, casterPass, dualParaboloid, sceneManager, passBufferPtr);
        }

		// Final alignment
		alignPassPtr16(passBufferPtr);

        // Verify we didn't overflow
        const size_t bytesWritten = (passBufferPtr - startupPtr) * sizeof(float);
        if (bytesWritten > mapSize)
        {
            OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR,
                "Pass buffer overflow! Written: " +
                StringConverter::toString(bytesWritten) +
                " bytes, allocated: " + StringConverter::toString(mapSize),
                "HlmsOcean::preparePassHash");
        }

        // Unmap the buffer
        passBuffer->unmap(UO_KEEP_PERSISTENT);

        // Reset state
        mLastTextureHash = 0;
        mLastMovableObject = nullptr;
        mLastBoundPool = nullptr;

        if (mShadowmapSamplerblock &&
            !getProperty(kNoTid, HlmsBaseProp::ShadowUsesDepthTexture))
            mCurrentShadowmapSamplerblock = mShadowmapSamplerblock;
        else
            mCurrentShadowmapSamplerblock = mShadowmapCmpSamplerblock;

        uploadDirtyDatablocks();

        return retVal;
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
        return fillBuffersFor(cache, queuedRenderable, casterPass,
            lastCacheHash, commandBuffer, false);
    }
    //-----------------------------------------------------------------------------------
    // ============================================================================
    // Bind Ocean textures EVERY frame, not just on movable change
    // ============================================================================
    uint32 HlmsOcean::fillBuffersFor(const HlmsCache* cache, const QueuedRenderable& queuedRenderable, bool casterPass, uint32 lastCacheHash, CommandBuffer* commandBuffer, bool isV1)
    {
        OGRE_UNUSED(cache);
        OGRE_UNUSED(isV1);

        const HlmsOceanDatablock* datablock =
            static_cast<const HlmsOceanDatablock*>(queuedRenderable.renderable->getDatablock());

        if (mCurrentPassBuffer == 0u)
            return 0u;

        // ----------------------------
        // Bind pass buffer (VS/PS slot 0)
        // ----------------------------
        ConstBufferPacked* passBuffer = mPassBuffers[mCurrentPassBuffer - 1u];
        *commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer(VertexShader, 0u, passBuffer, 0u, passBuffer->getTotalSizeBytes());
        *commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer(PixelShader, 0u, passBuffer, 0u, passBuffer->getTotalSizeBytes());

        // If HLMS type changes, we must (re)bind all shared resources.
        if (OGRE_EXTRACT_HLMS_TYPE_FROM_CACHE_HASH(lastCacheHash) != mType)
        {
            // ----------------------------
           // Atmosphere const buffers (Terra/PBS-compatible binding)
           // Must be bound when HLMS type changes, AFTER shared resources
           // ----------------------------
            if (!casterPass && mAtmosphere && mAtmosphere->providesHlmsCode())
            {
                // PBS layout:
                // 0 = pass buffer
                // 1 = material buffer
                // 2 = instance buffer
                // 3+ = lighting / atmosphere
                uint32 constBufferSlot = 3u;
                mAtmosphere->bindConstBuffers(commandBuffer, constBufferSlot);
            }

            // ----------------------------
            // Ocean baked textures (bind fixed)
            // Your HLSL setup effectively expects them at t0/t1 with samplers s0/s1.
            // ----------------------------
            ensureOceanTexturesLoaded();

            if (mOceanTexReady && mOceanDataTex && mWeightTex)
            {
                // Use the samplerblocks created in _changeRenderSystem (stable & cached)
                // IMPORTANT: bind to slots 0 and 1 (do NOT use getProperty("terrainData") for HLSL)
                *commandBuffer->addCommand<CbTexture>() = CbTexture(0u, mOceanDataTex, mOceanDataSamplerblock);
                *commandBuffer->addCommand<CbTexture>() = CbTexture(1u, mWeightTex, mWeightSamplerblock);
            }

            // ----------------------------
            // Forward+ buffers (if active)
            // NOTE: these are buffers, not textures; keep your existing slots.
            // ----------------------------
            size_t texUnit = 2u;

            if (mGridBuffer)
            {
                // Use slots 10 and 11 for lighting buffers to stay away from textures
                *commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer(PixelShader, 10u, mGridBuffer, 0u, 0u);
                *commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer(PixelShader, 11u, mGlobalLightListBuffer, 0u, 0u);
                texUnit = 5u;
            }

            // ----------------------------
            // Shadow maps start AFTER ocean textures (t2+)
            // ----------------------------
            for (TextureGpu* shadowTex : mPreparedPass.shadowMaps)
            {
                *commandBuffer->addCommand<CbTexture>() = CbTexture(static_cast<uint16>(texUnit++), shadowTex, mCurrentShadowmapSamplerblock);
            }

            // ----------------------------
            // Env probe (you hardcoded 15; keep it consistent with shader/pieces)
            // ----------------------------
            if (mProbe)
            {
                *commandBuffer->addCommand<CbTexture>() = CbTexture(15u, mProbe, mOceanSamplerblock);
            }

            // ----------------------------
            // Bind instance buffer (VS/PS slot 2)
            // ----------------------------
            if (mCurrentConstBuffer < mConstBuffers.size())
            {
                *commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer(VertexShader, 2u, mConstBuffers[mCurrentConstBuffer], 0u, 0u);
                *commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer(PixelShader, 2u, mConstBuffers[mCurrentConstBuffer], 0u, 0u);
            }

            // HLMS callback
            // mListener->hlmsTypeChanged(casterPass, commandBuffer, datablock, 0u);

            uint16 listenerTexUnit = 16u; // because we hard-bind env probe at t15
            mListener->hlmsTypeChanged(casterPass, commandBuffer, datablock, listenerTexUnit);

            // Reset caches (same spirit as Terra/Pbs)
            mLastTextureHash = 0u;
            mLastMovableObject = 0u;
            mLastBoundPool = 0u;
        }

        // ----------------------------
        // Bind material buffer if pool changed (PS slot 1)
        // ----------------------------
        if (mLastBoundPool != datablock->getAssignedPool())
        {
            const ConstBufferPool::BufferPool* newPool = datablock->getAssignedPool();
            const uint32 materialSlot = datablock->getAssignedSlot();
            const uint32 materialOffset = materialSlot * HlmsOceanDatablock::MaterialSizeInGpuAligned;

            const uint32 fullSize = newPool->materialBuffer->getTotalSizeBytes();

            *commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer(PixelShader, 1u, newPool->materialBuffer, materialOffset, fullSize - materialOffset);

            mLastBoundPool = newPool;
        }

        // ----------------------------
        // Per-draw data upload into instance const buffer
        // ----------------------------
        static constexpr uint32 kU32PerCell = 20u; // 16 (Terra) + 4 (oceanTime float4)

        uint32* RESTRICT_ALIAS currentMappedConstBuffer = mCurrentMappedConstBuffer;

        const size_t used = static_cast<size_t>(currentMappedConstBuffer - mStartMappedConstBuffer);
        if (used + kU32PerCell > mCurrentConstBufferSize)
        {
            // Switch to the next mapped const buffer
            currentMappedConstBuffer = mapNextConstBuffer(commandBuffer);

            // After switching buffers, rebind instance buffer (VS/PS slot 2),
            // otherwise GPU still reads the old buffer -> holes.
            if (mCurrentConstBuffer < mConstBuffers.size())
            {
                *commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer(VertexShader, 2u, mConstBuffers[mCurrentConstBuffer], 0u, 0u);
                *commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer(PixelShader, 2u, mConstBuffers[mCurrentConstBuffer], 0u, 0u);
            }
        }

        // Compute drawId BEFORE we advance the pointer
        const uint32 drawId =
            static_cast<uint32>((currentMappedConstBuffer - mStartMappedConstBuffer) / kU32PerCell);

        const OceanCell* oceanCell = static_cast<const OceanCell*>(queuedRenderable.renderable);
        oceanCell->uploadToGpu(currentMappedConstBuffer);
        currentMappedConstBuffer += kU32PerCell;

        mCurrentMappedConstBuffer = currentMappedConstBuffer;
        mLastMovableObject = queuedRenderable.movableObject;

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
            mOceanDataTex = texMgr->createOrRetrieveTexture(mOceanDataTextureName, GpuPageOutStrategy::Discard, static_cast<TextureFlags::TextureFlags>(0),
                TextureTypes::Type3D, ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);

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
    void HlmsOcean::destroyAllBuffers(void)
    {
        mCurrentPassBuffer = 0;

        {
            ConstBufferPackedVec::const_iterator itor = mPassBuffers.begin();
            ConstBufferPackedVec::const_iterator end = mPassBuffers.end();

            while (itor != end)
            {
                if ((*itor)->getMappingState() != MS_UNMAPPED)
                    (*itor)->unmap(UO_UNMAP_ALL);
                mVaoManager->destroyConstBuffer(*itor);
                ++itor;
            }

            mPassBuffers.clear();
        }
    }
	//-----------------------------------------------------------------------------------
    Ogre::Hlms::PropertiesMergeStatus HlmsOcean::notifyPropertiesMergedPreGenerationStep(size_t tid, PiecesMap* inOutPieces)
    {
        PropertiesMergeStatus status =
            HlmsPbs::notifyPropertiesMergedPreGenerationStep(tid, inOutPieces);

        if (status == PropertiesMergeStatusError)
            return status;

        int32 texSlotsStart = 0;

        // EXACTLY like Terra: Forward+ consumes texture slots (TexBuffer),
        // so the "first free texture slot" is after f3dGrid.
        if (getProperty(tid, HlmsBaseProp::ForwardPlus))
            texSlotsStart = getProperty(tid, "f3dGrid") + 1;

        // ---- Ocean textures MUST be the FIRST ones ----
        // So they become 0/1 when ForwardPlus is off, and shift when it's on (like Terra).
        setTextureReg(tid, VertexShader, "terrainData", texSlotsStart + 0);
        setTextureReg(tid, VertexShader, "blendMap", texSlotsStart + 1);

        // Ocean PS also samples them (your pixel shader declares them too)
        setTextureReg(tid, PixelShader, "terrainData", texSlotsStart + 0);
        setTextureReg(tid, PixelShader, "blendMap", texSlotsStart + 1);

        // ---- Shadows: start AFTER Ocean textures (if used by your pieces) ----
        // Terra does terrainNormals/terrainShadows here; you need the equivalent
        // if your ocean shader pieces declare shadow textures by name.
        // If you use the standard "texShadowMap0..n" path, ensure those regs start at texSlotsStart+2.
        if (!getProperty(tid, HlmsBaseProp::ShadowCaster))
        {
            // This loop only works if your shadow decl uses texShadowMap0..N.
            // If your pieces use different names, rename accordingly.
            const int32 numShadowMapLights = getProperty(tid, HlmsBaseProp::NumShadowMapLights);
            const int32 numPssmSplits = std::max<int32>(1, getProperty(tid, HlmsBaseProp::PssmSplits));
            const int32 numShadowMaps = std::max<int32>(0, numShadowMapLights * numPssmSplits);

            int32 baseShadow = texSlotsStart + 2;
            for (int32 i = 0; i < numShadowMaps; ++i)
            {
                Ogre::String name = "texShadowMap" + Ogre::StringConverter::toString(i);
                setTextureReg(tid, PixelShader, name.c_str(), baseShadow + i);
            }
        }

        // Env probe - ALWAYS at a fixed slot to avoid conflicts
        if (getProperty(tid, OceanProperty::EnvProbeMap))
        {
            // Use fixed slot 15 for ocean env probe
            setTextureReg(tid, PixelShader, "texEnvProbeMap", 15);
            setProperty(tid, "envMapRegSampler", 15);
        }

        // Samplers start at 2 (after ocean's s0/s1)
        setProperty(tid, "samplerStateStart", 2);

        const int32 numShadowMapLights = getProperty(tid, HlmsBaseProp::NumShadowMapLights);
        const int32 numPssmSplits = std::max<int32>(1, getProperty(tid, HlmsBaseProp::PssmSplits));
        const int32 numShadowMaps = std::max<int32>(0, numShadowMapLights * numPssmSplits);

        int32 totalSamplers = numShadowMaps;
        if (getProperty(tid, OceanProperty::EnvProbeMap))
            totalSamplers += 1;

        setProperty(tid, "num_samplers", totalSamplers);

        return status;
    }
    //-----------------------------------------------------------------------------------
    void HlmsOcean::setupRootLayout(RootLayout& rootLayout, size_t tid)
    {
        DescBindingRange* set0 = rootLayout.mDescBindingRanges[0];

        // Const buffers: 0 = pass, 1 = material, 2 = instance, 3+ = atmosphere
        set0[DescBindingTypes::ConstBuffer].start = 0u;
        const uint32 atmoCbCount = static_cast<uint32>(
            std::max<int32>(0, getProperty(tid, OceanProperty::AtmosphereNumConstBuffers)));
        set0[DescBindingTypes::ConstBuffer].end = 3u + atmoCbCount;

        // Forward+ buffers (if active)
        if (getProperty(tid, HlmsBaseProp::ForwardPlus))
        {
            set0[DescBindingTypes::ReadOnlyBuffer].start = 0u;
            set0[DescBindingTypes::ReadOnlyBuffer].end =
                static_cast<uint16>(getProperty(tid, "f3dLightList") + 1u);

            set0[DescBindingTypes::TexBuffer].start =
                static_cast<uint16>(getProperty(tid, "f3dGrid"));
            set0[DescBindingTypes::TexBuffer].end =
                static_cast<uint16>(set0[DescBindingTypes::TexBuffer].start + 1u);
        }
        else
        {
            set0[DescBindingTypes::ReadOnlyBuffer].start = 0u;
            set0[DescBindingTypes::ReadOnlyBuffer].end = 0u;
            set0[DescBindingTypes::TexBuffer].start = 0u;
            set0[DescBindingTypes::TexBuffer].end = 0u;
        }

        // Texture slots: terrainData (0), blendMap (1), shadows (2+), env probe (15)
        const uint16 ocean0 = (uint16)getProperty(tid, "terrainData");
        const uint16 ocean1 = (uint16)getProperty(tid, "blendMap");

        uint16 start = std::min(ocean0, ocean1);
        uint16 end = std::max(ocean0, ocean1) + 1;

        // Include shadow maps
        const int32 numShadowMapLights = getProperty(tid, HlmsBaseProp::NumShadowMapLights);
        const int32 numPssmSplits = std::max<int32>(1, getProperty(tid, HlmsBaseProp::PssmSplits));
        const uint16 numShadowMaps = (uint16)std::max<int32>(0, numShadowMapLights * numPssmSplits);

        end = std::max<uint16>(end, (uint16)(start + 2 + numShadowMaps));

        // Include env probe at slot 15
        if (getProperty(tid, OceanProperty::EnvProbeMap))
        {
            const uint16 probe = 15;
            end = std::max<uint16>(end, (uint16)(probe + 1));
        }

        set0[DescBindingTypes::Texture].start = start;
        set0[DescBindingTypes::Texture].end = end;

        set0[DescBindingTypes::Sampler].start = start;
        set0[DescBindingTypes::Sampler].end = end;

        rootLayout.mBaked[1] = false;
    }
    //-----------------------------------------------------------------------------------
    void HlmsOcean::postCommandBufferExecution(CommandBuffer* commandBuffer)
    {
        // Ocean doesn't need special command buffer post-processing like Terra does
        // (Terra uploads heightmap data here). Just ensure we call the base class.

        // If your base class HlmsPbs has postCommandBufferExecution, call it:
        // HlmsPbs::postCommandBufferExecution(commandBuffer);

        // Otherwise, leave empty - the crash is likely from buffer size issues,
        // not from this function itself
    }
    void HlmsOcean::frameEnded(void)
    {
        HlmsPbs::frameEnded();

        // validatePassBufferState();

        // Reset counter
        mCurrentPassBuffer = 0;

        // Ensure all buffers are properly unmapped
        for (size_t i = 0; i < mPassBuffers.size(); ++i)
        {
            ConstBufferPacked* buffer = mPassBuffers[i];
            if (buffer && buffer->getMappingState() != MS_UNMAPPED)
            {
                try
                {
                    buffer->unmap(UO_UNMAP_ALL);
                }
                catch (const Exception& e)
                {
                    LogManager::getSingleton().logMessage("[HlmsOcean] Exception unmapping buffer: " + e.getDescription(), LML_CRITICAL);
                }
            }
        }

        // Reset diagnostics
        mLastAllocatedPassBufferSize = 0;
        mLastWrittenPassBufferSize = 0;
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
    void HlmsOcean::setEnvProbe(TextureGpu* probe)
    {
        mProbe = probe;
    }
	//-----------------------------------------------------------------------------------
    void HlmsOcean::setEnvProbe(const Ogre::String& textureName)
    {
        Ogre::TextureGpuManager* textureManager = Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();

        if (true == textureName.empty())
        {
            mProbe = nullptr;
            return;
        }

        // --- Flags (PBS-style reflection setup, simplified for Ocean) ---
        // IMPORTANT: disable AutomaticBatching for cubemaps
        if (false == Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(textureName))
        {
            Ogre::LogManager::getSingletonPtr()->logMessage( Ogre::LML_CRITICAL, "[HlmsOcean] Cannot set env probe: '" + textureName + "', because it does not exist in any resource group!");
            mProbe = nullptr;
            return;
        }

        Ogre::uint32 textureFlags = 0;
        textureFlags |= Ogre::TextureFlags::PrefersLoadingFromFileAsSRGB;
        textureFlags &= ~Ogre::TextureFlags::AutomaticBatching;

        Ogre::uint32 filters = Ogre::TextureFilter::FilterTypes::TypeGenerateDefaultMipmaps;

        Ogre::TextureGpu* envTexture = textureManager->createOrRetrieveTexture(
            textureName,
            Ogre::GpuPageOutStrategy::SaveToSystemRam,
            textureFlags,
            Ogre::TextureTypes::TypeCube,
            Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
            filters,
            0u);

        if (nullptr == envTexture)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[HlmsOcean] Failed to create env probe texture: '" + textureName + "'");
            return;
        }

        try
        {
            envTexture->scheduleTransitionTo(Ogre::GpuResidency::Resident);
            textureManager->waitForStreamingCompletion();
        }
        catch (const Ogre::Exception& e)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, e.getFullDescription());
            return;
        }

		this->setEnvProbe(envTexture);
    }
    //-----------------------------------------------------------------------------------
    HlmsDatablock* HlmsOcean::createDatablockImpl(IdString datablockName, const HlmsMacroblock* macroblock, const HlmsBlendblock* blendblock, const HlmsParamVec& paramVec)
    {
        return OGRE_NEW HlmsOceanDatablock(datablockName, this, macroblock, blendblock, paramVec);
    }
}

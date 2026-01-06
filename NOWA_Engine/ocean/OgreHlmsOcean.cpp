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
#include "OgreTimer.h"

#include <iostream>
#include <algorithm>

namespace Ogre
{
    const IdString OceanProperty::HwGammaRead = IdString("hw_gamma_read");
    const IdString OceanProperty::HwGammaWrite = IdString("hw_gamma_write");
    const IdString OceanProperty::SignedIntTex = IdString("signed_int_textures");
    const IdString OceanProperty::MaterialsPerBuffer = IdString("materials_per_buffer");
    const IdString OceanProperty::DebugPssmSplits = IdString("debug_pssm_splits");

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
        HlmsBufferManager(HLMS_USER1, "Ocean", dataFolder, libraryFolders),
        ConstBufferPool(HlmsOceanDatablock::MaterialSizeInGpuAligned, ConstBufferPool::ExtraBufferParams()),
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
        //Override defaults
        mLightGatheringMode = LightGatherForwardPlus;
        mProbe = 0;
	    mOceanDataTextureName = "oceanData.dds";
	    mWeightTextureName = "weight.dds";
    }

    //-----------------------------------------------------------------------------------
    void HlmsOcean::analyzeBarriers(BarrierSolver& barrierSolver,
        ResourceTransitionArray& resourceTransitions,
        Camera* renderingCamera, const bool bCasterPass)
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

    //-----------------------------------------------------------------------------------
    HlmsOcean::~HlmsOcean()
    {
        destroyAllBuffers();
    }
    //-----------------------------------------------------------------------------------
    void HlmsOcean::_changeRenderSystem(RenderSystem* newRs)
    {
        ConstBufferPool::_changeRenderSystem(newRs);

        // Force the pool to contain only 1 entry.
        mSlotsPerPool = 1u;
        mBufferSize = HlmsOceanDatablock::MaterialSizeInGpuAligned;

        HlmsBufferManager::_changeRenderSystem(newRs);

        // Reset cached GPU textures when RS changes
        mOceanDataTex = nullptr;
        mWeightTex = nullptr;

        if (!newRs)
            return;

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
        samplerblock.mBorderColour = ColourValue(std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max());

        if (mShaderProfile != "hlsl")
        {
            samplerblock.mMinFilter = FO_POINT;
            samplerblock.mMagFilter = FO_POINT;
            samplerblock.mMipFilter = FO_NONE;

            if (!mShadowmapSamplerblock)
                mShadowmapSamplerblock = mHlmsManager->getSamplerblock(samplerblock);
        }

        samplerblock.mMinFilter = FO_LINEAR;
        samplerblock.mMagFilter = FO_LINEAR;
        samplerblock.mMipFilter = FO_NONE;
        samplerblock.mCompareFunction = CMPF_LESS_EQUAL;

        if (!mShadowmapCmpSamplerblock)
            mShadowmapCmpSamplerblock = mHlmsManager->getSamplerblock(samplerblock);

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
			sb.mMipFilter = FO_LINEAR;
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
            mOceanSamplerblock = mHlmsManager->getSamplerblock(HlmsSamplerblock());
    }
    //-----------------------------------------------------------------------------------
    const HlmsCache* HlmsOcean::createShaderCacheEntry(uint32 renderableHash,
        const HlmsCache& passCache,
        uint32 finalHash,
        const QueuedRenderable& queuedRenderable,
        HlmsCache* reservedStubEntry,
        const size_t tid)
    {
        const HlmsCache* retVal = Hlms::createShaderCacheEntry(renderableHash, passCache, finalHash,
            queuedRenderable, reservedStubEntry,
            tid);

        if (mShaderProfile == "hlsl")
        {
            mListener->shaderCacheEntryCreated(mShaderProfile, retVal, passCache,
                mT[tid].setProperties, queuedRenderable, tid);
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

        mListener->shaderCacheEntryCreated(mShaderProfile, retVal, passCache,
            mT[tid].setProperties, queuedRenderable, tid);

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
            "This Hlms can only be used on a Ocean object!");

        Ogre::OceanCell* oceanCell = static_cast<Ogre::OceanCell*>(renderable);
        setProperty(kNoTid, OceanProperty::UseSkirts, oceanCell->getUseSkirts());

        assert(dynamic_cast<HlmsOceanDatablock*>(renderable->getDatablock()));
        HlmsOceanDatablock* datablock =
            static_cast<HlmsOceanDatablock*>(renderable->getDatablock());

        setProperty(kNoTid, OceanProperty::ReceiveShadows, 1);
        setProperty(kNoTid, "terra_enabled", 0);
        setProperty(kNoTid, "hlms_vpos", 1);

        // IMPORTANT:
        // Do NOT set terrainData/blendMap slots here.
        // They are assigned dynamically in notifyPropertiesMergedPreGenerationStep via setTextureReg().

        uint32 brdf = datablock->getBrdf();
        if ((brdf & OceanBrdf::BRDF_MASK) == OceanBrdf::Default)
        {
            setProperty(kNoTid, OceanProperty::BrdfDefault, 1);
            if (!(brdf & OceanBrdf::FLAG_UNCORRELATED))
                setProperty(kNoTid, OceanProperty::GgxHeightCorrelated, 1);
        }
        else if ((brdf & OceanBrdf::BRDF_MASK) == OceanBrdf::CookTorrance)
            setProperty(kNoTid, OceanProperty::BrdfCookTorrance, 1);
        else if ((brdf & OceanBrdf::BRDF_MASK) == OceanBrdf::BlinnPhong)
            setProperty(kNoTid, OceanProperty::BrdfBlinnPhong, 1);

        if (brdf & OceanBrdf::FLAG_SPERATE_DIFFUSE_FRESNEL)
            setProperty(kNoTid, OceanProperty::FresnelSeparateDiffuse, 1);

        OGRE_UNUSED(inOutPieces);
    }
    //-----------------------------------------------------------------------------------
    HlmsCache HlmsOcean::preparePassHash(const CompositorShadowNode* shadowNode, bool casterPass,
        bool dualParaboloid, SceneManager* sceneManager)
    {
        mT[kNoTid].setProperties.clear();

        //setProperty(kNoTid, "use_planar_reflections", 0);
        //setProperty(kNoTid, PbsProperty::ParallaxCorrectCubemaps, 0);
        //setProperty(kNoTid, PbsProperty::EnableCubemapsAuto, 1); // Make the && condition false
        //setProperty(kNoTid, "vct_num_probes", 0);
        //setProperty(kNoTid, PbsProperty::IrradianceVolumes, 0);

        if (casterPass)
        {
            HlmsCache retVal = Hlms::preparePassHashBase(shadowNode, casterPass, dualParaboloid, sceneManager);
            return retVal;
        }


        // --- Env probe (reflection cubemap) ---
        // In D3D11/HLSL the texture registers are compiled into the shader.
        // To avoid overlaps with shadow maps / Forward+ buffers, we use a fixed, high slot.
        // (Shadow maps are typically in the low single digits; slot 70 is safely out of the way.)
        const bool hasProbe = (mProbe != nullptr);
        setProperty(kNoTid, OceanProperty::EnvProbeMap, hasProbe ? 1 : 0);


        //The properties need to be set before preparePassHash so that
        //they are considered when building the HlmsCache's hash.
        if (shadowNode && !casterPass)
        {
            //Shadow receiving can be improved in performance by using gather sampling.
            //(it's the only feature so far that uses gather)
            const RenderSystemCapabilities* capabilities = mRenderSystem->getCapabilities();
            if (capabilities->hasCapability(RSC_TEXTURE_GATHER))
                setProperty(kNoTid, HlmsBaseProp::TexGather, 1);

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
                        setProperty(kNoTid, OceanProperty::DebugPssmSplits, 1);
                }
            }
        }

        AmbientLightMode ambientMode = mAmbientLightMode;
        ColourValue upperHemisphere = sceneManager->getAmbientLightUpperHemisphere();
        ColourValue lowerHemisphere = sceneManager->getAmbientLightLowerHemisphere();

        const float envMapScale = upperHemisphere.a;
        //Ignore alpha channel
        upperHemisphere.a = lowerHemisphere.a = 1.0;

        if (!casterPass)
        {
            if (mAmbientLightMode == AmbientAuto)
            {
                if (upperHemisphere == lowerHemisphere)
                {
                    if (upperHemisphere == ColourValue::Black)
                        ambientMode = AmbientNone;
                    else
                        ambientMode = AmbientFixed;
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

        HlmsCache retVal = Hlms::preparePassHashBase(shadowNode, casterPass,
            dualParaboloid, sceneManager);

        if (getProperty(kNoTid, HlmsBaseProp::LightsDirNonCaster) > 0)
        {
            //First directional light always cast shadows thanks to our Ocean shadows.
            int32 shadowCasterDirectional = getProperty(kNoTid, HlmsBaseProp::LightsDirectional);
            shadowCasterDirectional = std::max(shadowCasterDirectional, 1);
            setProperty(kNoTid, HlmsBaseProp::LightsDirectional, shadowCasterDirectional);
        }

        Viewport* viewport = mRenderSystem->getCurrentRenderViewports();
        TextureGpu* renderTarget = viewport->getCurrentTarget();
        OGRE_UNUSED(renderTarget);

        const RenderSystemCapabilities* capabilities = mRenderSystem->getCapabilities();
        setProperty(kNoTid, OceanProperty::HwGammaRead, capabilities->hasCapability(RSC_HW_GAMMA));
        setProperty(kNoTid, OceanProperty::HwGammaWrite, capabilities->hasCapability(RSC_HW_GAMMA));
        setProperty(kNoTid, OceanProperty::SignedIntTex,
            capabilities->hasCapability(RSC_TEXTURE_SIGNED_INT));
        retVal.setProperties = mT[kNoTid].setProperties;

        auto camerasInProgress = sceneManager->getCamerasInProgress();
        const Camera* camera = camerasInProgress.renderingCamera;
        Matrix4 viewMatrix = camera->getViewMatrix(true);

        Matrix4 projectionMatrix = camera->getProjectionMatrixWithRSDepth();

        if (renderTarget->requiresTextureFlipping())
        {
            projectionMatrix[1][0] = -projectionMatrix[1][0];
            projectionMatrix[1][1] = -projectionMatrix[1][1];
            projectionMatrix[1][2] = -projectionMatrix[1][2];
            projectionMatrix[1][3] = -projectionMatrix[1][3];
        }

        int32 numLights = getProperty(kNoTid, HlmsBaseProp::LightsSpot);
        int32 numDirectionalLights = getProperty(kNoTid, HlmsBaseProp::LightsDirNonCaster);
        int32 numShadowMapLights = getProperty(kNoTid, HlmsBaseProp::NumShadowMapLights);
        int32 numPssmSplits = getProperty(kNoTid, HlmsBaseProp::PssmSplits);

        //mat4 viewProj + mat4 view;
        size_t mapSize = (16 + 16) * 4;

        mGridBuffer = 0;
        mGlobalLightListBuffer = 0;

        ForwardPlusBase* forwardPlus = sceneManager->_getActivePassForwardPlus();
        if (forwardPlus)
        {
            mapSize += forwardPlus->getConstBufferSize();
            mGridBuffer = forwardPlus->getGridBuffer(camera);
            mGlobalLightListBuffer = forwardPlus->getGlobalLightListBuffer(camera);
        }

        //mat4 shadowRcv[numShadowMapLights].texViewProj +
        //              vec2 shadowRcv[numShadowMapLights].shadowDepthRange +
        //              vec2 padding +
        //              vec4 shadowRcv[numShadowMapLights].invShadowMapSize +
        //mat3 invViewMatCubemap (upgraded to three vec4)
        mapSize += ((16 + 2 + 2 + 4) * numShadowMapLights + 4 * 3) * 4;

        //vec3 ambientUpperHemi + float envMapScale
        if (ambientMode == AmbientFixed || ambientMode == AmbientHemisphere || envMapScale != 1.0f)
            mapSize += 4 * 4;

        //vec3 ambientLowerHemi + padding + vec3 ambientHemisphereDir + padding
        if (ambientMode == AmbientHemisphere)
            mapSize += 8 * 4;

        //float pssmSplitPoints N times.
        mapSize += numPssmSplits * 4;
        mapSize = alignToNextMultiple<uint32>(mapSize, 16);

        if (shadowNode)
        {
            //Six variables * 4 (padded vec3) * 4 (bytes) * numLights
            mapSize += (6 * 4 * 4) * numLights;
        }
        else
        {
            //Three variables * 4 (padded vec3) * 4 (bytes) * numDirectionalLights
            mapSize += (3 * 4 * 4) * numDirectionalLights;
        }

        // Existing listener data (can be multiple listeners)
        mapSize = alignToNextMultiple<uint32>(mapSize, 16);
        mapSize += mListener->getPassBufferSize(shadowNode, casterPass, dualParaboloid, sceneManager);
        // mapSize += sizeof(float) * 4; // timer
        mapSize = alignToNextMultiple<uint32>(mapSize, 16);


        //Arbitrary 8kb, should be enough.
        const size_t maxBufferSize = 8 * 1024;

        assert(mapSize <= maxBufferSize);

        if (mCurrentPassBuffer >= mPassBuffers.size())
        {
            mPassBuffers.push_back(mVaoManager->createConstBuffer(maxBufferSize, BT_DYNAMIC_PERSISTENT, 0, false));
        }

        ConstBufferPacked* passBuffer = mPassBuffers[mCurrentPassBuffer++];
        float* passBufferPtr = reinterpret_cast<float*>(passBuffer->map(0, mapSize));


        // IMPORTANT: We use persistent buffers; padding holes would keep stale data.
        // Clear the mapped region so debug scans and shader reads are deterministic.
        const size_t numFloatsToClear = mapSize / sizeof(float);
        std::fill_n(passBufferPtr, numFloatsToClear, 0.0f);
        const float* startupPtr = passBufferPtr;

        auto alignPassPtr16 = [&](float*& ptr)
            {
                const size_t bytesSoFar = size_t(ptr - startupPtr) * sizeof(float);
                const size_t alignedBytes = alignToNextMultiple<uint32>(static_cast<uint32>(bytesSoFar), 16u);
                const size_t padBytes = alignedBytes - bytesSoFar;
                const size_t padFloats = padBytes / sizeof(float);
                for (size_t i = 0; i < padFloats; ++i)
                    *ptr++ = 0.0f; // keep padding deterministic
            };

        //---------------------------------------------------------------------------
        //                          ---- VERTEX SHADER ----
        //---------------------------------------------------------------------------

        //mat4 viewProj;
        Matrix4 viewProjMatrix = projectionMatrix * viewMatrix;
        for (size_t i = 0; i < 16; ++i)
            *passBufferPtr++ = (float)viewProjMatrix[0][i];

        mPreparedPass.viewMatrix = viewMatrix;

        mPreparedPass.shadowMaps.clear();

        //mat4 view;
        for (size_t i = 0; i < 16; ++i)
        {
            *passBufferPtr++ = (float)viewMatrix[0][i];
        }

        size_t shadowMapTexIdx = 0;
        // Ogre-Next uses TextureGpu for shadow maps.
        const auto& contiguousShadowMapTex = shadowNode->getContiguousShadowMapTex();

        if (nullptr != shadowNode)
        {

            for (int32 i = 0; i < numShadowMapLights; ++i)
            {
                //Skip inactive lights (e.g. no directional lights are available
                //and there's a shadow map that only accepts dir lights)
                while (!shadowNode->isShadowMapIdxActive(shadowMapTexIdx))
                    ++shadowMapTexIdx;

                //mat4 shadowRcv[numShadowMapLights].texViewProj
                Matrix4 viewProjTex = shadowNode->getViewProjectionMatrix(shadowMapTexIdx);
                for (size_t j = 0; j < 16; ++j)
                    *passBufferPtr++ = (float)viewProjTex[0][j];

                //vec2 shadowRcv[numShadowMapLights].shadowDepthRange
                Real fNear, fFar;
                shadowNode->getMinMaxDepthRange(shadowMapTexIdx, fNear, fFar);
                const Real depthRange = fFar - fNear;
                *passBufferPtr++ = fNear;
                *passBufferPtr++ = 1.0f / depthRange;
                *passBufferPtr++ = 0.0f; //Padding
                *passBufferPtr++ = 0.0f; //Padding


                //vec2 shadowRcv[numShadowMapLights].invShadowMapSize
                size_t shadowMapTexContigIdx = shadowNode->getIndexToContiguousShadowMapTex((size_t)shadowMapTexIdx);

                const uint32 texWidth = contiguousShadowMapTex[shadowMapTexContigIdx]->getWidth();
                const uint32 texHeight = contiguousShadowMapTex[shadowMapTexContigIdx]->getHeight();
                *passBufferPtr++ = 1.0f / texWidth;
                *passBufferPtr++ = 1.0f / texHeight;
                *passBufferPtr++ = static_cast<float>(texWidth);
                *passBufferPtr++ = static_cast<float>(texHeight);

                ++shadowMapTexIdx;
            }
        }

        //---------------------------------------------------------------------------
        //                          ---- PIXEL SHADER ----
        //---------------------------------------------------------------------------

        Matrix3 viewMatrix3, invViewMatrixCubemap;
        viewMatrix.extract3x3Matrix(viewMatrix3);
        invViewMatrixCubemap = viewMatrix3.Inverse();
        //Cubemaps are left-handed.
        invViewMatrixCubemap = viewMatrix3;
        invViewMatrixCubemap[0][2] = -invViewMatrixCubemap[0][2];
        invViewMatrixCubemap[1][2] = -invViewMatrixCubemap[1][2];
        invViewMatrixCubemap[2][2] = -invViewMatrixCubemap[2][2];
        invViewMatrixCubemap = invViewMatrixCubemap.Inverse();

        //mat3 invViewMatCubemap
        for (size_t i = 0; i < 9; ++i)
        {
#ifdef OGRE_GLES2_WORKAROUND_2
            Matrix3 xRot(1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, -1.0f,
                0.0f, 1.0f, 0.0f);
            xRot = xRot * invViewMatrixCubemap;
            *passBufferPtr++ = (float)xRot[0][i];
#else
            * passBufferPtr++ = (float)invViewMatrixCubemap[0][i];
#endif

            //Alignment: each row/column is one vec4, despite being 3x3
            if (!((i + 1) % 3))
                ++passBufferPtr;
        }

        //vec3 ambientUpperHemi + padding
        if (ambientMode == AmbientFixed || ambientMode == AmbientHemisphere || envMapScale != 1.0f)
        {
            *passBufferPtr++ = static_cast<float>(upperHemisphere.r);
            *passBufferPtr++ = static_cast<float>(upperHemisphere.g);
            *passBufferPtr++ = static_cast<float>(upperHemisphere.b);
            *passBufferPtr++ = envMapScale;
        }

        //vec3 ambientLowerHemi + padding + vec3 ambientHemisphereDir + padding
        if (ambientMode == AmbientHemisphere)
        {
            *passBufferPtr++ = static_cast<float>(lowerHemisphere.r);
            *passBufferPtr++ = static_cast<float>(lowerHemisphere.g);
            *passBufferPtr++ = static_cast<float>(lowerHemisphere.b);
            *passBufferPtr++ = 1.0f;

            Vector3 hemisphereDir = viewMatrix3 * sceneManager->getAmbientLightHemisphereDir();
            hemisphereDir.normalise();
            *passBufferPtr++ = static_cast<float>(hemisphereDir.x);
            *passBufferPtr++ = static_cast<float>(hemisphereDir.y);
            *passBufferPtr++ = static_cast<float>(hemisphereDir.z);
            *passBufferPtr++ = 1.0f;
        }

        //float pssmSplitPoints
        for (int32 i = 0; i < numPssmSplits; ++i)
            *passBufferPtr++ = (*shadowNode->getPssmSplits(0))[i + 1];

        passBufferPtr += alignToNextMultiple(numPssmSplits, 4) - numPssmSplits;

        if (numShadowMapLights > 0)
        {
            //All directional lights (caster and non-caster) are sent.
            //Then non-directional shadow-casting shadow lights are sent.
            size_t shadowLightIdx = 0;
            size_t nonShadowLightIdx = 0;
            const LightListInfo& globalLightList = sceneManager->getGlobalLightList();
            const LightClosestArray& lights = shadowNode->getShadowCastingLights();

            const CompositorShadowNode::LightsBitSet& affectedLights =
                shadowNode->getAffectedLightsBitSet();

            int32 shadowCastingDirLights = getProperty(kNoTid, HlmsBaseProp::LightsDirectional);

            for (int32 i = 0; i < numLights; ++i)
            {
                Light const* light = 0;

                if (i >= shadowCastingDirLights && i < numDirectionalLights)
                {
                    while (affectedLights[nonShadowLightIdx])
                        ++nonShadowLightIdx;

                    light = globalLightList.lights[nonShadowLightIdx++];

                    assert(light->getType() == Light::LT_DIRECTIONAL);
                }
                else
                {
                    //Skip inactive lights (e.g. no directional lights are available
                    //and there's a shadow map that only accepts dir lights)
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

                //vec3 lights[numLights].position
                *passBufferPtr++ = lightPos.x;
                *passBufferPtr++ = lightPos.y;
                *passBufferPtr++ = lightPos.z;
                ++passBufferPtr;

                //vec3 lights[numLights].diffuse
                ColourValue color = light->getDiffuseColour() *
                    light->getPowerScale();
                *passBufferPtr++ = color.r;
                *passBufferPtr++ = color.g;
                *passBufferPtr++ = color.b;
                ++passBufferPtr;

                //vec3 lights[numLights].specular
                color = light->getSpecularColour() * light->getPowerScale();
                *passBufferPtr++ = color.r;
                *passBufferPtr++ = color.g;
                *passBufferPtr++ = color.b;
                ++passBufferPtr;

                //vec3 lights[numLights].attenuation;
                Real attenRange = light->getAttenuationRange();
                Real attenLinear = light->getAttenuationLinear();
                Real attenQuadratic = light->getAttenuationQuadric();
                *passBufferPtr++ = attenRange;
                *passBufferPtr++ = attenLinear;
                *passBufferPtr++ = attenQuadratic;
                ++passBufferPtr;

                //vec3 lights[numLights].spotDirection;
                Vector3 spotDir = viewMatrix3 * light->getDerivedDirection();
                *passBufferPtr++ = spotDir.x;
                *passBufferPtr++ = spotDir.y;
                *passBufferPtr++ = spotDir.z;
                ++passBufferPtr;

                //vec3 lights[numLights].spotParams;
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
            //No shadow maps, only send directional lights
            const LightListInfo& globalLightList = sceneManager->getGlobalLightList();

            for (int32 i = 0; i < numDirectionalLights; ++i)
            {
                assert(globalLightList.lights[i]->getType() == Light::LT_DIRECTIONAL);

                Vector4 lightPos4 = globalLightList.lights[i]->getAs4DVector();
                Vector3 lightPos = viewMatrix3 * Vector3(lightPos4.x, lightPos4.y, lightPos4.z);

                //vec3 lights[numLights].position
                *passBufferPtr++ = lightPos.x;
                *passBufferPtr++ = lightPos.y;
                *passBufferPtr++ = lightPos.z;
                ++passBufferPtr;

                //vec3 lights[numLights].diffuse
                ColourValue color = globalLightList.lights[i]->getDiffuseColour() *
                    globalLightList.lights[i]->getPowerScale();
                *passBufferPtr++ = color.r;
                *passBufferPtr++ = color.g;
                *passBufferPtr++ = color.b;
                ++passBufferPtr;

                //vec3 lights[numLights].specular
                color = globalLightList.lights[i]->getSpecularColour() * globalLightList.lights[i]->getPowerScale();
                *passBufferPtr++ = color.r;
                *passBufferPtr++ = color.g;
                *passBufferPtr++ = color.b;
                ++passBufferPtr;
            }
        }

        if (shadowNode)
        {
            mPreparedPass.shadowMaps.reserve(contiguousShadowMapTex.size());

            for (auto* tex : contiguousShadowMapTex)
                mPreparedPass.shadowMaps.push_back(tex);
        }

        if (forwardPlus)
        {
            RenderPassDescriptor* renderPassDesc = mRenderSystem->getCurrentPassDescriptor();
            const bool isInstancedStereo = false;
            forwardPlus->fillConstBufferData(sceneManager->getCurrentViewport0(), renderPassDesc->requiresTextureFlipping(), renderTarget->getHeight(),
                mShaderSyntax, isInstancedStereo, passBufferPtr);

            passBufferPtr += forwardPlus->getConstBufferSize() >> 2;
        }

        // ===================================================================
        // CRITICAL: These pieces are inserted BEFORE custom_passBuffer in the shader!
        // We must either write the data OR skip if not present.
        // ===================================================================

        // 1. DeclPlanarReflUniforms
        // If you're not using planar reflections, this should be empty, but check properties
        if (getProperty(kNoTid, "use_planar_reflections"))
        {
            // If enabled, you need to write planar reflection data here
            // For now, let's assume it's disabled
            OGRE_EXCEPT(Exception::ERR_NOT_IMPLEMENTED, "Planar reflections not implemented in Ocean", "HlmsOcean::preparePassHash");
        }

        // 2. CubemapProbe autoProbe
        if (getProperty(kNoTid, PbsProperty::ParallaxCorrectCubemaps) &&
            !getProperty(kNoTid, PbsProperty::EnableCubemapsAuto))
        {
            // CubemapProbe struct needs to be written here
            // For now, let's assume it's disabled
            OGRE_EXCEPT(Exception::ERR_NOT_IMPLEMENTED,  "Parallax correct cubemaps not implemented in Ocean",  "HlmsOcean::preparePassHash");
        }

        // 3. DeclVctUniform
        if (getProperty(kNoTid, "vct_num_probes"))
        {
            // VCT data needs to be written here
            OGRE_EXCEPT(Exception::ERR_NOT_IMPLEMENTED, "VCT not implemented in Ocean", "HlmsOcean::preparePassHash");
        }

        // 4. DeclIrradianceFieldUniform
        if (getProperty(kNoTid, PbsProperty::IrradianceVolumes))
        {
            // Irradiance volumes data needs to be written here
            OGRE_EXCEPT(Exception::ERR_NOT_IMPLEMENTED, "Irradiance volumes not implemented in Ocean", "HlmsOcean::preparePassHash");
        }

        // Align exactly like Terra expects (custom_passBuffer is vec4-aligned)
        alignPassPtr16(passBufferPtr);

        if (mListener)
            passBufferPtr = mListener->preparePassBuffer(shadowNode, casterPass, dualParaboloid, sceneManager, passBufferPtr);

        // Align to match final mapSize alignment too
        alignPassPtr16(passBufferPtr);

#if 0
        const size_t idx = (size_t)(passBufferPtr - startupPtr); // you need startupPtr passed in or stored
        LogManager::getSingleton().logMessage(
            "OceanListener write at float idx=" + StringConverter::toString(idx) +
            " casterPass=" + StringConverter::toString(casterPass) +
            " dualParaboloid=" + StringConverter::toString(dualParaboloid) +
            " shadowNode=" + StringConverter::toString((size_t)shadowNode));

        Ogre::LogManager::getSingleton().logMessage(
            "Ocean pass: shadowNode=" + StringConverter::toString((size_t)shadowNode) +
            " listenerEndIdx=" + StringConverter::toString(idx) +
            " cacheHash=" + StringConverter::toString(retVal.hash) +   // or whatever you have there
            " casterPass=" + StringConverter::toString(casterPass));

        // Final alignment check
        assert((size_t)(passBufferPtr - startupPtr) * sizeof(float) == mapSize);

        {
            const size_t numFloats = mapSize / sizeof(float);
            const float* dbg = startupPtr;


            for (size_t i = 0; i < numFloats; ++i)
            {
                if (dbg[i] == 42.0f || dbg[i] == 43.0f ||
                    dbg[i] == 44.0f || dbg[i] == 45.0f)
                {
                    Ogre::LogManager::getSingleton().logMessage(
                        "---->MARKER FOUND at index " +
                        Ogre::StringConverter::toString(i) +
                        " value = " +
                        Ogre::StringConverter::toString(dbg[i]));
                }
            }
        }
#endif

        passBuffer->unmap(UO_KEEP_PERSISTENT);

        mLastTextureHash = 0;
        mLastMovableObject = 0;

        mLastBoundPool = 0;

        if (mShadowmapSamplerblock && !getProperty(kNoTid, HlmsBaseProp::ShadowUsesDepthTexture))
            mCurrentShadowmapSamplerblock = mShadowmapSamplerblock;
        else
            mCurrentShadowmapSamplerblock = mShadowmapCmpSamplerblock;

        uploadDirtyDatablocks();

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    uint32 HlmsOcean::fillBuffersFor(const HlmsCache* cache, const QueuedRenderable& queuedRenderable,
        bool casterPass, uint32 lastCacheHash,
        uint32 lastTextureHash)
    {
        OGRE_EXCEPT(Exception::ERR_NOT_IMPLEMENTED,
            "Trying to use slow-path on a desktop implementation. "
            "Change the RenderQueue settings.",
            "HlmsOcean::fillBuffersFor");
    }
    //-----------------------------------------------------------------------------------
    uint32 HlmsOcean::fillBuffersForV1(const HlmsCache* cache,
        const QueuedRenderable& queuedRenderable,
        bool casterPass, uint32 lastCacheHash,
        CommandBuffer* commandBuffer)
    {
        return fillBuffersFor(cache, queuedRenderable, casterPass,
            lastCacheHash, commandBuffer, true);
    }
    //-----------------------------------------------------------------------------------
    uint32 HlmsOcean::fillBuffersForV2(const HlmsCache* cache,
        const QueuedRenderable& queuedRenderable,
        bool casterPass, uint32 lastCacheHash,
        CommandBuffer* commandBuffer)
    {
        return fillBuffersFor(cache, queuedRenderable, casterPass,
            lastCacheHash, commandBuffer, false);
    }
    //-----------------------------------------------------------------------------------
    // ============================================================================
    // CRITICAL FIX: Bind Ocean textures EVERY frame, not just on movable change
    // ============================================================================
    uint32 HlmsOcean::fillBuffersFor(const HlmsCache* cache,
        const QueuedRenderable& queuedRenderable,
        bool casterPass,
        uint32 lastCacheHash,
        CommandBuffer* commandBuffer,
        bool isV1)
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
        *commandBuffer->addCommand<CbShaderBuffer>() =
            CbShaderBuffer(VertexShader, 0u, passBuffer, 0u, passBuffer->getTotalSizeBytes());
        *commandBuffer->addCommand<CbShaderBuffer>() =
            CbShaderBuffer(PixelShader, 0u, passBuffer, 0u, passBuffer->getTotalSizeBytes());

        // If HLMS type changes, we must (re)bind all shared resources.
        if (OGRE_EXTRACT_HLMS_TYPE_FROM_CACHE_HASH(lastCacheHash) != mType)
        {
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
                *commandBuffer->addCommand<CbTexture>() =
                    CbTexture(static_cast<uint16>(texUnit++), shadowTex, mCurrentShadowmapSamplerblock);
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
                *commandBuffer->addCommand<CbShaderBuffer>() =
                    CbShaderBuffer(VertexShader, 2u, mConstBuffers[mCurrentConstBuffer], 0u, 0u);
                *commandBuffer->addCommand<CbShaderBuffer>() =
                    CbShaderBuffer(PixelShader, 2u, mConstBuffers[mCurrentConstBuffer], 0u, 0u);
            }

            // HLMS callback
            mListener->hlmsTypeChanged(casterPass, commandBuffer, datablock, 0u);

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
            *commandBuffer->addCommand<CbShaderBuffer>() =
                CbShaderBuffer(PixelShader, 1u, newPool->materialBuffer, 0u,
                    newPool->materialBuffer->getTotalSizeBytes());
            mLastBoundPool = newPool;
        }

        // ============================================================================
        // CRITICAL: Bind material buffer with OFFSET for the specific datablock
        // ============================================================================
        if (mLastBoundPool != datablock->getAssignedPool())
        {
            const ConstBufferPool::BufferPool* newPool = datablock->getAssignedPool();
            const uint32 materialSlot = datablock->getAssignedSlot();
            const uint32 materialOffset = materialSlot * HlmsOceanDatablock::MaterialSizeInGpuAligned;

            // Use the remaining buffer size from the offset
            uint32 fullSize = newPool->materialBuffer->getTotalSizeBytes();

            *commandBuffer->addCommand<CbShaderBuffer>() =
                CbShaderBuffer(PixelShader, 1u, newPool->materialBuffer,
                    materialOffset,
                    fullSize - materialOffset);

            mLastBoundPool = newPool;

            // Debug logging (remove after confirming it works)
            LogManager::getSingleton().logMessage(
                "[HlmsOcean] Bound material buffer - Slot: " +
                StringConverter::toString(materialSlot) +
                ", Offset: " + StringConverter::toString(materialOffset) +
                ", Size: " + StringConverter::toString(fullSize - materialOffset));
        }

        // ----------------------------
        // Per-draw data upload into instance const buffer
        // ----------------------------
        uint32* RESTRICT_ALIAS currentMappedConstBuffer = mCurrentMappedConstBuffer;

        // 16 uints written by oceanCell->uploadToGpu()
        const size_t needed = 16u;
        const size_t used = static_cast<size_t>(currentMappedConstBuffer - mStartMappedConstBuffer);

        if (used + needed > mCurrentConstBufferSize)
            currentMappedConstBuffer = mapNextConstBuffer(commandBuffer);

        const OceanCell* oceanCell = static_cast<const OceanCell*>(queuedRenderable.renderable);
        oceanCell->uploadToGpu(currentMappedConstBuffer);
        currentMappedConstBuffer += 16u;

        mCurrentMappedConstBuffer = currentMappedConstBuffer;
        mLastMovableObject = queuedRenderable.movableObject;

        return static_cast<uint32>(((mCurrentMappedConstBuffer - mStartMappedConstBuffer) >> 4) - 1u);
    }


    void HlmsOcean::ensureOceanTexturesLoaded()
    {
        if (mOceanTexReady)
            return;

        TextureGpuManager* texMgr = Root::getSingleton().getRenderSystem()->getTextureGpuManager();

        // Load 3D ocean data
        try
        {
            mOceanDataTex = texMgr->createOrRetrieveTexture(
                mOceanDataTextureName,
                GpuPageOutStrategy::Discard,
                TextureFlags::PrefersLoadingFromFileAsSRGB,
                TextureTypes::Type3D,
                ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);

            if (!mOceanDataTex)
            {
                LogManager::getSingleton().logMessage(
                    "[HlmsOcean ERROR] Failed to create ocean data texture", LML_CRITICAL);
                return;
            }

            mOceanDataTex->scheduleTransitionTo(GpuResidency::Resident);

            LogManager::getSingleton().logMessage(
                "[HlmsOcean] Ocean data texture created: " + mOceanDataTextureName);
        }
        catch (const Exception& e)
        {
            LogManager::getSingleton().logMessage(
                "[HlmsOcean ERROR] Exception loading ocean data: " + e.getDescription(), LML_CRITICAL);
            return;
        }

        // Load 2D weight map
        try
        {
            mWeightTex = texMgr->createOrRetrieveTexture(
                mWeightTextureName,
                GpuPageOutStrategy::Discard,
                TextureFlags::PrefersLoadingFromFileAsSRGB,
                TextureTypes::Type2D,
                ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);

            if (!mWeightTex)
            {
                LogManager::getSingleton().logMessage(
                    "[HlmsOcean ERROR] Failed to create weight texture", LML_CRITICAL);
                return;
            }

            mWeightTex->scheduleTransitionTo(GpuResidency::Resident);

            LogManager::getSingleton().logMessage(
                "[HlmsOcean] Weight texture created: " + mWeightTextureName);
        }
        catch (const Exception& e)
        {
            LogManager::getSingleton().logMessage(
                "[HlmsOcean ERROR] Exception loading weight texture: " + e.getDescription(), LML_CRITICAL);
            return;
        }

        // Wait for loading
        if (mOceanDataTex && mWeightTex)
        {
            texMgr->waitForStreamingCompletion();

            // Detailed texture info
            LogManager::getSingleton().logMessage(
                "[HlmsOcean] Ocean data dimensions: " +
                StringConverter::toString(mOceanDataTex->getWidth()) + "x" +
                StringConverter::toString(mOceanDataTex->getHeight()) + "x" +
                StringConverter::toString(mOceanDataTex->getDepth()));

            LogManager::getSingleton().logMessage(
                "[HlmsOcean] Weight texture dimensions: " +
                StringConverter::toString(mWeightTex->getWidth()) + "x" +
                StringConverter::toString(mWeightTex->getHeight()));

            if (mOceanDataTex->getWidth() > 0u && mWeightTex->getWidth() > 0u)
            {
                mOceanTexReady = true;
                LogManager::getSingleton().logMessage(
                    "[HlmsOcean] Textures successfully loaded and ready!");
            }
            else
            {
                LogManager::getSingleton().logMessage(
                    "[HlmsOcean ERROR] Textures have zero dimensions!", LML_CRITICAL);
            }
        }
    }

    //-----------------------------------------------------------------------------------
    void HlmsOcean::destroyAllBuffers(void)
    {
        HlmsBufferManager::destroyAllBuffers();

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
    void HlmsOcean::notifyPropertiesMergedPreGenerationStep(const size_t tid)
    {
        // IMPORTANT: Let PBS/Terra glue decide its own registers first.
        // If your HlmsOcean cannot call HlmsPbs directly (no inheritance),
        // call the base Hlms implementation:
        Hlms::notifyPropertiesMergedPreGenerationStep(tid);

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

        // ---- Env probe ----
        // If your shader uses texEnvProbeMap, set it AFTER everything else.
        // (Don’t hardcode 15 unless you also guarantee nothing else uses it.)
        if (getProperty(tid, OceanProperty::EnvProbeMap))
        {
            // put probe after shadows
            const int32 numShadowMapLights = getProperty(tid, HlmsBaseProp::NumShadowMapLights);
            const int32 numPssmSplits = std::max<int32>(1, getProperty(tid, HlmsBaseProp::PssmSplits));
            const int32 numShadowMaps = std::max<int32>(0, numShadowMapLights * numPssmSplits);

            const int32 probeSlot = texSlotsStart + 2 + numShadowMaps;
            setTextureReg(tid, PixelShader, "texEnvProbeMap", probeSlot);
            
            setProperty(tid, "envMapRegSampler", probeSlot);
        }

        // ---- Samplers ----
        // This fixes your 's0 overlap': reserve s0/s1 for Ocean, so HLMS auto samplers start at s2.
        setProperty(tid, "samplerStateStart", 2);
        
        const int32 numShadowMapLights = getProperty(tid, HlmsBaseProp::NumShadowMapLights);
        const int32 numPssmSplits = std::max<int32>(1, getProperty(tid, HlmsBaseProp::PssmSplits));
        const int32 numShadowMaps = std::max<int32>(0, numShadowMapLights * numPssmSplits);
        
        int32 totalSamplers = numShadowMaps;
        if (getProperty(tid, OceanProperty::EnvProbeMap))
            totalSamplers += 1;
            
        setProperty(tid, "num_samplers", totalSamplers);
    }
    //-----------------------------------------------------------------------------------
    void HlmsOcean::postCommandBufferExecution(CommandBuffer* commandBuffer)
    {
        // Keep same behaviour as base (just reset state after command buffer is executed).
        HlmsBufferManager::postCommandBufferExecution(commandBuffer);
    }

    // ============================================================================
// CRITICAL FIX: setupRootLayout must include slots 0 and 1
// ============================================================================
    void HlmsOcean::setupRootLayout(RootLayout& rootLayout, size_t tid)
    {
        DescBindingRange* set0 = rootLayout.mDescBindingRanges[0];

        // Const buffers: 0 = pass, 1 = material, 2 = instance
        set0[DescBindingTypes::ConstBuffer].start = 0u;
        set0[DescBindingTypes::ConstBuffer].end = 3u;

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

        const uint16 ocean0 = (uint16)getProperty(tid, "terrainData");
        const uint16 ocean1 = (uint16)getProperty(tid, "blendMap");

        uint16 start = std::min(ocean0, ocean1);
        uint16 end = std::max(ocean0, ocean1) + 1; // end is exclusive

        // include shadow maps and env probe regs if present:
        const int32 numShadowMapLights = getProperty(tid, HlmsBaseProp::NumShadowMapLights);
        const int32 numPssmSplits = std::max<int32>(1, getProperty(tid, HlmsBaseProp::PssmSplits));
        const uint16 numShadowMaps = (uint16)std::max<int32>(0, numShadowMapLights * numPssmSplits);

        end = std::max<uint16>(end, (uint16)(start + 2 + numShadowMaps)); // shadows start after ocean

        if (getProperty(tid, OceanProperty::EnvProbeMap))
        {
            const uint16 probe = (uint16)getProperty(tid, "texEnvProbeMap");
            end = std::max<uint16>(end, (uint16)(probe + 1));
            start = std::min<uint16>(start, probe); // (only if probe reg can be lower; usually it isn't)
        }

        // MUST start at 0 for Ocean textures
        set0[DescBindingTypes::Texture].start = start;
        set0[DescBindingTypes::Texture].end = end;

        set0[DescBindingTypes::Sampler].start = start;
        set0[DescBindingTypes::Sampler].end = end;

        rootLayout.mBaked[1] = false;
    }
    //-----------------------------------------------------------------------------------
    void HlmsOcean::frameEnded(void)
    {
        HlmsBufferManager::frameEnded();
        mCurrentPassBuffer = 0;
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
    HlmsDatablock* HlmsOcean::createDatablockImpl(IdString datablockName,
        const HlmsMacroblock* macroblock,
        const HlmsBlendblock* blendblock,
        const HlmsParamVec& paramVec)
    {
        return OGRE_NEW HlmsOceanDatablock(datablockName, this, macroblock, blendblock, paramVec);
    }
}

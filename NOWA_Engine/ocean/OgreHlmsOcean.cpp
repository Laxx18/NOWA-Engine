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
        ConstBufferPool(HlmsOceanDatablock::MaterialSizeInGpuAligned,
            ConstBufferPool::ExtraBufferParams()),
        mShadowmapSamplerblock(0),
        mShadowmapCmpSamplerblock(0),
        mCurrentShadowmapSamplerblock(0),
        mOceanSamplerblock(0),
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
        // We need to know what RenderSystem is currently in use, as the
        // name of the compatible shading language is part of the path
        Ogre::RenderSystem* renderSystem = Ogre::Root::getSingleton().getRenderSystem();
        Ogre::String shaderSyntax = "GLSL";
        if (renderSystem->getName() == "Direct3D11 Rendering Subsystem")
            shaderSyntax = "HLSL";
        else if (renderSystem->getName() == "Metal Rendering Subsystem")
            shaderSyntax = "Metal";

        outLibraryFoldersPaths.clear();

        outLibraryFoldersPaths.push_back("Hlms/Common/" + shaderSyntax);
        outLibraryFoldersPaths.push_back("Hlms/Common/Any");
        outLibraryFoldersPaths.push_back("Hlms/Common/Any/Main");

        outLibraryFoldersPaths.push_back("Hlms/Pbs/Any");
        outLibraryFoldersPaths.push_back("Hlms/Pbs/Any/Main");
        outLibraryFoldersPaths.push_back("Hlms/Pbs/" + shaderSyntax);

        // Terra
        outLibraryFoldersPaths.push_back("Hlms/Terra/" + shaderSyntax);
        outLibraryFoldersPaths.push_back("Hlms/Terra/Any");
        outLibraryFoldersPaths.push_back("Hlms/Terra/" + shaderSyntax + "/PbsTerraShadows");
        outLibraryFoldersPaths.push_back("Hlms/Terra/Any/PbsTerraShadows");

        // Ocean hooks
        outLibraryFoldersPaths.push_back("Hlms/Ocean/Any");
        outLibraryFoldersPaths.push_back("Hlms/Ocean/Any/Main");

        // Custom last
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

        //Force the pool to contain only 1 entry.
        mSlotsPerPool = 1u;
        mBufferSize = HlmsOceanDatablock::MaterialSizeInGpuAligned;

        HlmsBufferManager::_changeRenderSystem(newRs);

        if (newRs)
        {
            HlmsDatablockMap::const_iterator itor = mDatablocks.begin();
            HlmsDatablockMap::const_iterator end = mDatablocks.end();

            while (itor != end)
            {
                assert(dynamic_cast<HlmsOceanDatablock*>(itor->second.datablock));
                HlmsOceanDatablock* datablock = static_cast<HlmsOceanDatablock*>(itor->second.datablock);

                requestSlot(datablock->mTextureHash, datablock, false);
                ++itor;
            }

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

            if (!mOceanSamplerblock)
                mOceanSamplerblock = mHlmsManager->getSamplerblock(HlmsSamplerblock());
        }
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

        Ogre::OceanCell* OceanCell = static_cast<Ogre::OceanCell*>(renderable);
        setProperty(kNoTid, OceanProperty::UseSkirts, OceanCell->getUseSkirts());

        assert(dynamic_cast<HlmsOceanDatablock*>(renderable->getDatablock()));
        HlmsOceanDatablock* datablock = static_cast<HlmsOceanDatablock*>(
            renderable->getDatablock());

        setProperty(kNoTid, OceanProperty::ReceiveShadows, 1);

        setProperty(kNoTid, "terra_enabled", 0);
        setProperty(kNoTid, OceanProperty::EnvProbeMap, 1);

        // setProperty( kNoTid, "terrainData", 0);
        // setProperty( kNoTid, "blendMap", 1);

        // TODO:
        // Only if you actually bind a shadow texture:
        // setProperty( kNoTid, "terrainShadows", 2);

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
    }
    //-----------------------------------------------------------------------------------
    HlmsCache HlmsOcean::preparePassHash(const CompositorShadowNode* shadowNode, bool casterPass,
        bool dualParaboloid, SceneManager* sceneManager)
    {
        mT[kNoTid].setProperties.clear();

        if (casterPass)
        {
            HlmsCache retVal = Hlms::preparePassHashBase(shadowNode, casterPass,
                dualParaboloid, sceneManager);
            return retVal;
        }

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

        mapSize += mListener->getPassBufferSize(shadowNode, casterPass, dualParaboloid,
            sceneManager);

        //Arbitrary 8kb, should be enough.
        const size_t maxBufferSize = 8 * 1024;

        assert(mapSize <= maxBufferSize);

        if (mCurrentPassBuffer >= mPassBuffers.size())
        {
            mPassBuffers.push_back(mVaoManager->createConstBuffer(maxBufferSize, BT_DYNAMIC_PERSISTENT,
                0, false));
        }

        ConstBufferPacked* passBuffer = mPassBuffers[mCurrentPassBuffer++];
        float* passBufferPtr = reinterpret_cast<float*>(passBuffer->map(0, mapSize));

#ifndef NDEBUG
        const float* startupPtr = passBufferPtr;
#endif

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
            *passBufferPtr++ = (float)viewMatrix[0][i];

        size_t shadowMapTexIdx = 0;
        // Ogre-Next uses TextureGpu for shadow maps.
        const auto& contiguousShadowMapTex = shadowNode->getContiguousShadowMapTex();

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
            ++passBufferPtr; //Padding
            ++passBufferPtr; //Padding


            //vec2 shadowRcv[numShadowMapLights].invShadowMapSize
            size_t shadowMapTexContigIdx =
                shadowNode->getIndexToContiguousShadowMapTex((size_t)shadowMapTexIdx);
            const uint32 texWidth = contiguousShadowMapTex[shadowMapTexContigIdx]->getWidth();
            const uint32 texHeight = contiguousShadowMapTex[shadowMapTexContigIdx]->getHeight();
            *passBufferPtr++ = 1.0f / texWidth;
            *passBufferPtr++ = 1.0f / texHeight;
            *passBufferPtr++ = static_cast<float>(texWidth);
            *passBufferPtr++ = static_cast<float>(texHeight);

            ++shadowMapTexIdx;
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
            forwardPlus->fillConstBufferData(sceneManager->getCurrentViewport0(),
                renderPassDesc->requiresTextureFlipping(),
                renderTarget->getHeight(),
                mShaderSyntax, isInstancedStereo, passBufferPtr);

            passBufferPtr += forwardPlus->getConstBufferSize() >> 2;
        }

        //Timer
        *passBufferPtr++ = Ogre::Root::getSingletonPtr()->getTimer()->getMilliseconds() * 0.0005;
        *passBufferPtr++ = 0;
        *passBufferPtr++ = 0;
        *passBufferPtr++ = 0;


        passBufferPtr = mListener->preparePassBuffer(shadowNode, casterPass, dualParaboloid,
            sceneManager, passBufferPtr);

        // assert( (size_t)(passBufferPtr - startupPtr) * 4u == mapSize );

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
    uint32 HlmsOcean::fillBuffersFor(const HlmsCache* cache, const QueuedRenderable& queuedRenderable,
        bool casterPass, uint32 lastCacheHash,
        CommandBuffer* commandBuffer, bool isV1)
    {
        assert(dynamic_cast<const HlmsOceanDatablock*>(queuedRenderable.renderable->getDatablock()));
        const HlmsOceanDatablock* datablock = static_cast<const HlmsOceanDatablock*>(
            queuedRenderable.renderable->getDatablock());

        if (OGRE_EXTRACT_HLMS_TYPE_FROM_CACHE_HASH(lastCacheHash) != mType)
        {
            //layout(binding = 0) uniform PassBuffer {} pass
            ConstBufferPacked* passBuffer = mPassBuffers[mCurrentPassBuffer - 1];
            *commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer(VertexShader,
                0, passBuffer, 0,
                passBuffer->
                getTotalSizeBytes());
            *commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer(PixelShader,
                0, passBuffer, 0,
                passBuffer->
                getTotalSizeBytes());

            size_t texUnit = 3;

            if (mGridBuffer)
            {
                texUnit = 5;
                *commandBuffer->addCommand<CbShaderBuffer>() =
                    CbShaderBuffer(PixelShader, 3, mGridBuffer, 0, 0);
                *commandBuffer->addCommand<CbShaderBuffer>() =
                    CbShaderBuffer(PixelShader, 4, mGlobalLightListBuffer, 0, 0);
            }

            //We changed HlmsType, rebind the shared textures.
            FastArray<TextureGpu*>::const_iterator itor = mPreparedPass.shadowMaps.begin();
            FastArray<TextureGpu*>::const_iterator end = mPreparedPass.shadowMaps.end();
            while (itor != end)
            {
                *commandBuffer->addCommand<CbTexture>() =
                    CbTexture((uint16)texUnit, *itor, mCurrentShadowmapSamplerblock);
                ++texUnit;
                ++itor;
            }

            mLastTextureHash = 0;
            mLastMovableObject = 0;
            mLastBoundPool = 0;

            //layout(binding = 2) uniform InstanceBuffer {} instance
            if (mCurrentConstBuffer < mConstBuffers.size() &&
                (size_t)((mCurrentMappedConstBuffer - mStartMappedConstBuffer) + 16) <=
                mCurrentConstBufferSize)
            {
                *commandBuffer->addCommand<CbShaderBuffer>() =
                    CbShaderBuffer(VertexShader, 2, mConstBuffers[mCurrentConstBuffer], 0, 0);
                *commandBuffer->addCommand<CbShaderBuffer>() =
                    CbShaderBuffer(PixelShader, 2, mConstBuffers[mCurrentConstBuffer], 0, 0);
            }

            mListener->hlmsTypeChanged(casterPass, commandBuffer, datablock, 0u);
        }

        //Don't bind the material buffer on caster passes (important to keep
        //MDI & auto-instancing running on shadow map passes)
        if (mLastBoundPool != datablock->getAssignedPool() &&
            (!casterPass || datablock->getAlphaTest() != CMPF_ALWAYS_PASS))
        {
            //layout(binding = 1) uniform MaterialBuf {} materialArray
            const ConstBufferPool::BufferPool* newPool = datablock->getAssignedPool();
            *commandBuffer->addCommand<CbShaderBuffer>() = CbShaderBuffer(PixelShader,
                1, newPool->materialBuffer, 0,
                newPool->materialBuffer->
                getTotalSizeBytes());
            mLastBoundPool = newPool;
        }

        if (mLastMovableObject != queuedRenderable.movableObject)
        {
            Ogre::HlmsManager* hlmsManager = Ogre::Root::getSingleton().getHlmsManager();
            Ogre::TextureGpuManager* hlmsTextureManager = Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();

            auto ensureTextureGpu = [&](const Ogre::String& filename,
                Ogre::TextureTypes::TextureTypes texType,
                Ogre::TextureFlags::TextureFlags flags) -> Ogre::TextureGpu*
                {
                    Ogre::TextureGpu* tex = nullptr;

                    // Optional: avoid creating if not present in any resource location
                    const Ogre::String* nameStr = hlmsTextureManager->findResourceNameStr(filename);
                    if (!nameStr)
                        return nullptr;

                    tex = hlmsTextureManager->createOrRetrieveTexture(
                        filename,
                        Ogre::GpuPageOutStrategy::Discard,
                        flags,
                        texType,
                        Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);

                    if (!tex)
                        return nullptr;

                    try
                    {
                        tex->scheduleTransitionTo(Ogre::GpuResidency::Resident);
                    }
                    catch (const Ogre::Exception& e)
                    {
                        Ogre::LogManager::getSingleton().logMessage(e.getFullDescription(), Ogre::LML_CRITICAL);
                        return nullptr;
                    }

                    // Note: width can be 0 until streaming finished.
                    if (tex->getWidth() == 0u)
                        hlmsTextureManager->waitForStreamingCompletion();

                    if (tex->getWidth() == 0u)
                        return nullptr; // still invalid

                    return tex;
                };

            // Build samplerblocks once (or cache them as members if you want)
            Ogre::HlmsSamplerblock sb3D;
            sb3D.mU = Ogre::TextureAddressingMode::TAM_WRAP;
            sb3D.mV = Ogre::TextureAddressingMode::TAM_WRAP;
            sb3D.mW = Ogre::TextureAddressingMode::TAM_WRAP;
            const Ogre::HlmsSamplerblock* finalSb3D = hlmsManager->getSamplerblock(sb3D);

            Ogre::HlmsSamplerblock sb2D;
            sb2D.mU = Ogre::TextureAddressingMode::TAM_WRAP;
            sb2D.mV = Ogre::TextureAddressingMode::TAM_WRAP;
            const Ogre::HlmsSamplerblock* finalSb2D = hlmsManager->getSamplerblock(sb2D);

	            // Ocean data (3D)
            {
                // If oceanData.dds is authored as linear data (likely), do NOT force SRGB.
                // If your asset is actually sRGB, keep PrefersLoadingFromFileAsSRGB.
	                Ogre::TextureGpu* tex3D = ensureTextureGpu(
	                    mOceanDataTextureName,
                    Ogre::TextureTypes::Type3D,
                    static_cast<Ogre::TextureFlags::TextureFlags>(0u)
                    // or: Ogre::TextureFlags::PrefersLoadingFromFileAsSRGB
                );

                if (tex3D)
                    *commandBuffer->addCommand<Ogre::CbTexture>() = Ogre::CbTexture(0u, tex3D, finalSb3D);
            }

	            // Weight (2D)
            {
                Ogre::TextureGpu* tex2D = ensureTextureGpu(
	                    mWeightTextureName,
                    Ogre::TextureTypes::Type2D,
                    static_cast<Ogre::TextureFlags::TextureFlags>(0u)
                );

                if (tex2D)
                    *commandBuffer->addCommand<Ogre::CbTexture>() = Ogre::CbTexture(1u, tex2D, finalSb2D);
            }

            // Probe texture (already TextureGpu*)
            if (mProbe)
            {
                const size_t texUnit = mPreparedPass.shadowMaps.size() + (!mGridBuffer ? 3u : 5u);
                *commandBuffer->addCommand<Ogre::CbTexture>() =
                    Ogre::CbTexture(static_cast<uint16>(texUnit), mProbe, mOceanSamplerblock);
            }

            mLastMovableObject = queuedRenderable.movableObject;
        }

        uint32* RESTRICT_ALIAS currentMappedConstBuffer = mCurrentMappedConstBuffer;

        //---------------------------------------------------------------------------
        //                          ---- VERTEX SHADER ----
        //---------------------------------------------------------------------------
        //We need to correct currentMappedConstBuffer to point to the right texture buffer's
        //offset, which may not be in sync if the previous draw had skeletal animation.
        bool exceedsConstBuffer = (size_t)((currentMappedConstBuffer - mStartMappedConstBuffer) + 12)
            > mCurrentConstBufferSize;

            if (exceedsConstBuffer)
                currentMappedConstBuffer = mapNextConstBuffer(commandBuffer);

            const Ogre::OceanCell* OceanCell = static_cast<const Ogre::OceanCell*>(queuedRenderable.renderable);

            OceanCell->uploadToGpu(currentMappedConstBuffer);
            currentMappedConstBuffer += 16u;

            //---------------------------------------------------------------------------
            //                          ---- PIXEL SHADER ----
            //---------------------------------------------------------------------------

            mCurrentMappedConstBuffer = currentMappedConstBuffer;

            return ((mCurrentMappedConstBuffer - mStartMappedConstBuffer) >> 4) - 1;
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
    //-----------------------------------------------------------------------------------
    void HlmsOcean::postCommandBufferExecution(CommandBuffer* commandBuffer)
    {
        // Keep same behaviour as base (just reset state after command buffer is executed).
        HlmsBufferManager::postCommandBufferExecution(commandBuffer);
    }

    void HlmsOcean::setupRootLayout(RootLayout& rootLayout, size_t tid)
    {
        // This matches the modern HLMS model (see HlmsPbs::setupRootLayout)
        // and gives Ogre enough info to create descriptor layouts.
        DescBindingRange* set0 = rootLayout.mDescBindingRanges[0];

        // --- Constant buffers ---
        // Binding 0: pass buffer (VS+PS)
        // Binding 1: material buffer (PS) - still a const buffer range requirement
        set0[DescBindingTypes::ConstBuffer].start = 0u;
        set0[DescBindingTypes::ConstBuffer].end = 2u;

        // --- Forward+ (if enabled) ---
        // Your Ocean code uses properties "f3dGrid" and "f3dLightList"
        // and binds buffers at those slots.
        if (getProperty(tid, HlmsBaseProp::ForwardPlus))
        {
            // ReadOnlyBuffer numbering is treated as if it shares the same contiguous index space
            // (see HlmsPbs::setupRootLayout comment).
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

        // --- Textures & Samplers ---
        // Ocean binds textures at units:
        // 0 oceanData, 1 weight, then shadow maps starting at 3 (or 5 if Forward+),
        // then optional env probe.
        // We compute a conservative but correct end.

        uint16 texStart = 0u;

        // Keep the PBS rule: texture slots begin after the last "buffer-as-texture" slot.
        if (set0[DescBindingTypes::ReadOnlyBuffer].isInUse())
            texStart = std::max<uint16>(texStart, set0[DescBindingTypes::ReadOnlyBuffer].end);
        if (set0[DescBindingTypes::TexBuffer].isInUse())
            texStart = std::max<uint16>(texStart, set0[DescBindingTypes::TexBuffer].end);

        // Base texture unit where shadow maps begin in your Ocean code:
        const uint16 baseTexUnit = getProperty(tid, HlmsBaseProp::ForwardPlus) ? 5u : 3u;

        // Shadow maps count approximation:
        // Ocean uses mPreparedPass.shadowMaps.size(), which corresponds to:
        // NumShadowMapLights * max(1, PssmSplits)
        const int32 numShadowMapLights = getProperty(tid, HlmsBaseProp::NumShadowMapLights);
        const int32 numPssmSplits = std::max<int32>(1, getProperty(tid, HlmsBaseProp::PssmSplits));
        const int32 numShadowMaps = std::max<int32>(0, numShadowMapLights * numPssmSplits);

        // Do we use env probe in shader?
        const bool hasEnvProbe = getProperty(tid, OceanProperty::EnvProbeMap) != 0;

        uint16 texEnd = baseTexUnit;
        texEnd = static_cast<uint16>(texEnd + numShadowMaps + (hasEnvProbe ? 1u : 0u));

        // Must at least cover oceanData(0) and weight(1)
        texEnd = std::max<uint16>(texEnd, 2u);

        set0[DescBindingTypes::Texture].start = texStart;
        set0[DescBindingTypes::Texture].end = std::max<uint16>(texStart, texEnd);

        // Ocean uses samplers for those textures as well.
        set0[DescBindingTypes::Sampler].start = set0[DescBindingTypes::Texture].start;
        set0[DescBindingTypes::Sampler].end = set0[DescBindingTypes::Texture].end;

        // Ocean doesn't use additional baked descriptor sets (set1 etc.) in this legacy design.
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

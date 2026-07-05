/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2021 Torus Knot Software Ltd

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
#include "Terra.h"
#include "TerraShadowMapper.h"
#include "OgreHlmsTerraDatablock.h"
#include "OgreHlmsTerra.h"

#include "OgreImage2.h"

#include "OgreCamera.h"
#include "OgreSceneManager.h"
#include "Compositor/OgreCompositorManager2.h"
#include "Compositor/OgreCompositorWorkspace.h"
#include "Compositor/OgreCompositorChannel.h"
#include "OgreDepthBuffer.h"
#include "OgreMaterialManager.h"
#include "OgreTechnique.h"
#include "OgreTextureGpuManager.h"
#include "OgreStagingTexture.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreDescriptorSetTexture.h"
#include "OgreAsyncTextureTicket.h"
#include "OgreHlmsManager.h"
#include "OgreRoot.h"
#include "OgreBitwise.h"

#include "OgreLogManager.h"

namespace Ogre
{
    inline Vector3 ZupToYup(Vector3 value)
    {
        std::swap(value.y, value.z);
        value.z = -value.z;
        return value;
    }

    inline Ogre::Vector3 YupToZup(Ogre::Vector3 value)
    {
        std::swap(value.y, value.z);
        value.y = -value.y;
        return value;
    }

    /*inline Ogre::Quaternion ZupToYup( Ogre::Quaternion value )
    {
        return value * Ogre::Quaternion( Ogre::Radian( Ogre::Math::HALF_PI ), Ogre::Vector3::UNIT_X );
    }*/

    float Terra::computeAutoLodRing0WorldSize(float effectiveVisibleDistance, uint32 desiredLodLevels) const
    {
        desiredLodLevels = std::max(1u, desiredLodLevels);

        float ring0FromDistance = effectiveVisibleDistance / float(1u << desiredLodLevels);

        // Don't let LOD-0 alone eat up most of the terrain -- reserve room for
        // at least 3 successive doublings (LOD 0..3) within the terrain bounds.
        const float terrainSize = std::max(m_xzDimensions.x, m_xzDimensions.y);
        const uint32 minRingsThatMustFit = 3u;
        float ring0FromTerrain = terrainSize / float(1u << minRingsThatMustFit);

        float ring0 = std::min(ring0FromDistance, ring0FromTerrain);

        // Absolute floor so we never produce a degenerate near-zero footprint.
        return std::max(ring0, 1.0f);
    }

    //-----------------------------------------------------------------------------------
    //uint32 Terra::computeLodLevelForDistance(float distanceFromCamera) const
    //{
    //    // Normalize distance into 0..1, where 1.0 == m_maxLodDistance ("100%").
    //    float t = Math::Clamp(distanceFromCamera / std::max(1.0f, m_maxLodDistance), 0.0f, 1.0f);

    //    // Shape the falloff curve. curvePower == 1.0 -> linear.
    //    float shaped = std::pow(t, std::max(0.01f, m_lodCurvePower));

    //    // Desired *area* vertex density fraction at this distance:
    //    // 1.0 at t=0 (no extra decimation), m_lodFarVertexPercent at t=1.
    //    float densityFraction = 1.0f - (1.0f - m_lodFarVertexPercent) * shaped;
    //    densityFraction = std::max(densityFraction, m_lodFarVertexPercent);

    //    // Convert area-density fraction into an EXTRA per-axis decimation factor,
    //    // on top of whatever the ring-doubling baseline already does. At t=0 this
    //    // must come out to 0 extra levels; it only grows as t -> 1.
    //    float extraDivisorF = 1.0f / std::sqrt(std::max(densityFraction, 0.0001f));
    //    float extraLevelF = std::log2(std::max(extraDivisorF, 1.0f));

    //    uint32 extraLevels = static_cast<uint32>(std::round(extraLevelF));
    //    return extraLevels;
    //}

    //-----------------------------------------------------------------------------------
    //uint32 Terra::computeLodLevelForDistance(float distanceFromCamera) const
    //{
    //    // Explicit distance bands -> extra decimation levels, scaled by
    //    // m_maxLodDistance ("100%"). Edit the fractions/levels directly for
    //    // full, predictable control - no sqrt/log2 ceiling to fight against.
    //    const float t = (m_maxLodDistance > 1.0f) ? (distanceFromCamera / m_maxLodDistance) : 0.0f;

    //    if (t < 0.15f)       return 0u;  // near camera: untouched
    //    else if (t < 0.30f)  return 1u;  // 4x area decimation
    //    else if (t < 0.50f)  return 2u;  // 16x
    //    else if (t < 0.70f)  return 3u;  // 64x
    //    else if (t < 0.85f)  return 4u;  // 256x
    //    else                 return 6u;  // far field: 4096x, brutally flat
    //}

    //-----------------------------------------------------------------------------------
    uint32 Terra::computeLodLevelForDistance(float distanceFromCamera) const
    {
        // Generic, percentage-based: t is always 0..1 regardless of whether
        // m_maxLodDistance is 200 or 10000, since it's a ratio - this is the
        // part that already worked and stays untouched.
        const float t = Math::Clamp(distanceFromCamera / std::max(1.0f, m_maxLodDistance), 0.0f, 1.0f);

        // Ramp shape - power < 1.0 reaches high decimation earlier in the range,
        // so mid-distance terrain doesn't stay over-detailed. Unchanged behavior.
        const float shaped = std::pow(t, std::max(0.01f, m_lodCurvePower));

        // How many extra halving-steps are even achievable on THIS heightmap.
        // Derived from m_width/m_depth, which already exist as members - no new
        // state needed. Bigger heightmaps naturally allow more extra levels.
        const uint32 maxRes = std::max(m_width, m_depth);
        const float maxAchievableExtraLevels = std::max(1.0f, std::log2(float(std::max(1u, maxRes))));

        // m_lodFarVertexPercent is now a DIRECT 0..1 fraction of that achievable
        // budget - no sqrt, no log2 in between, no hidden ceiling. Set it to 0.8
        // and you get 80% of whatever this heightmap can give you at max range.
        const float fractionOfBudget = Math::Clamp(m_lodFarVertexPercent, 0.0f, 1.0f);

        const float extraLevelF = shaped * fractionOfBudget * maxAchievableExtraLevels;

        return static_cast<uint32>(std::round(extraLevelF));
    }

    Terra::Terra(IdType id, ObjectMemoryManager* objectMemoryManager,
        SceneManager* sceneManager, uint8 renderQueueId,
        CompositorManager2* compositorManager, Camera* camera, bool zUp) :
        MovableObject(id, objectMemoryManager, sceneManager, renderQueueId),
        m_width(0u),
        m_depth(0u),
        m_depthWidthRatio(1.0f),
        m_skirtSize(10.0f),
        m_invWidth(1.0f),
        m_invDepth(1.0f),
        m_zUp(zUp),
        m_xzDimensions(Vector2::UNIT_SCALE),
        m_xzInvDimensions(Vector2::UNIT_SCALE),
        m_xzRelativeSize(Vector2::UNIT_SCALE),
        m_dimension(Vector3::UNIT_SCALE),
        m_height(1.0f),
        m_heightUnormScaled(1.0f),
        m_terrainOrigin(Vector3::ZERO),
        m_basePixelDimension(256u),
        m_invMaxValue(1.0f),
        m_currentCell(0u),
        m_heightMapTex(0),
        m_normalMapTex(0),
        m_blendWeightTex(0),
        m_heightMapStagingTexture(nullptr),
        m_blendWeightStagingTexture(nullptr),
        m_prevLightDir(Vector3::ZERO),
        m_shadowMapper(0),
        m_sharedResources(0),
        m_compositorManager(compositorManager),
        m_camera(camera),
        mNormalMapperWorkspace(0),
        mNormalMapCamera(0),
        m_brushSize(64),
        mHlmsTerraIndex(std::numeric_limits<uint32>::max()),
        m_lodRing0WorldSize(10.0f),
        m_maxLodDistance(500.0f),
        m_lodFarVertexPercent(0.1f),
        m_lodCurvePower(1.0f)
    {
        if (nullptr == m_camera)
        {
            throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[Terra] Camera for terra is null!", "Terra");
        }
    }
    //-----------------------------------------------------------------------------------
    Terra::~Terra()
    {
        if (!m_terrainCells[0].empty() && m_terrainCells[0].back().getDatablock())
        {
            HlmsDatablock* datablock = m_terrainCells[0].back().getDatablock();
            OGRE_ASSERT_HIGH(dynamic_cast<Ogre::HlmsTerra*>(datablock->getCreator()));
            Ogre::HlmsTerra* hlms = static_cast<Ogre::HlmsTerra*>(datablock->getCreator());
            hlms->_unlinkTerra(this);
        }
        if (nullptr != mNormalMapperWorkspace)
        {
            m_compositorManager->removeWorkspace(mNormalMapperWorkspace);
            mNormalMapperWorkspace = nullptr;
        }
        if (nullptr != mNormalMapCamera)
        {
            mManager->destroyCamera(mNormalMapCamera);
            mNormalMapCamera = nullptr;
        }

        if (m_shadowMapper)
        {
            m_shadowMapper->destroyShadowMap();
            delete m_shadowMapper;
            m_shadowMapper = 0;
        }
        destroyNormalTexture();
        destroyHeightmapTexture();
        destroyBlendWeightTexture();
        m_terrainCells[0].clear();
        m_terrainCells[1].clear();
    }
    //-----------------------------------------------------------------------------------
    Vector3 Terra::fromYUp(Vector3 value) const
    {
        if (m_zUp)
            return YupToZup(value);
        return value;
    }
    //-----------------------------------------------------------------------------------
    Vector3 Terra::fromYUpSignPreserving(Vector3 value) const
    {
        if (m_zUp)
            std::swap(value.y, value.z);
        return value;
    }
    //-----------------------------------------------------------------------------------
    Vector3 Terra::toYUp(Vector3 value) const
    {
        if (m_zUp)
            return ZupToYup(value);
        return value;
    }
    //-----------------------------------------------------------------------------------
    Vector3 Terra::toYUpSignPreserving(Vector3 value) const
    {
        if (m_zUp)
            std::swap(value.y, value.z);
        return value;
    }
    //-----------------------------------------------------------------------------------
    void Terra::destroyHeightmapTexture(void)
    {
        if (m_heightMapTex)
        {
            TextureGpuManager* textureManager =
                mManager->getDestinationRenderSystem()->getTextureGpuManager();
            textureManager->destroyTexture(m_heightMapTex);
            m_heightMapTex = 0;

            if (nullptr != m_heightMapStagingTexture)
            {
                textureManager->removeStagingTexture(m_heightMapStagingTexture);
                m_heightMapStagingTexture = nullptr;
            }
        }
    }

    void Terra::destroyBlendWeightTexture(void)
    {
        if (m_blendWeightTex)
        {
            TextureGpuManager* textureManager = mManager->getDestinationRenderSystem()->getTextureGpuManager();
            textureManager->destroyTexture(m_blendWeightTex);
            m_blendWeightTex = 0;

            if (nullptr != m_blendWeightStagingTexture)
            {
                TextureGpuManager* textureManager = mManager->getDestinationRenderSystem()->getTextureGpuManager();
                textureManager->removeStagingTexture(m_blendWeightStagingTexture);
                m_blendWeightStagingTexture = nullptr;
            }
        }
    }

    //-----------------------------------------------------------------------------------
    void Terra::createHeightmapTexture(const Ogre::Image2& image)
    {
        destroyHeightmapTexture();

        if (image.getPixelFormat() != PFG_R16_UNORM)
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
                "Heightmap texture must be greyscale 8 bpp, 16 bpp, or 32-bit Float",
                "Terra::createHeightmapTexture");
        }

        //const uint8 numMipmaps = image.getNumMipmaps();

        TextureGpuManager* textureManager = mManager->getDestinationRenderSystem()->getTextureGpuManager();
        m_heightUnormScaled = m_height;
        Ogre::PixelFormatGpu pixelFormat = image.getPixelFormat();
#if OGRE_PLATFORM == OGRE_PLATFORM_ANDROID // Many Android GPUs don't support PFG_R16_UNORM so we scale it by hand
        if (pixelFormat == PFG_R16_UNORM && !textureManager->checkSupport(PFG_R16_UNORM, TextureTypes::Type2D, 0))
        {
            pixelFormat = PFG_R16_UINT;
            m_heightUnormScaled /= 65535.0f;
        }
#endif
        m_heightMapTex = textureManager->createTexture(m_prefix + "heightMap.png",
            GpuPageOutStrategy::SaveToSystemRam, TextureFlags::RenderToTexture | TextureFlags::AllowAutomipmaps, TextureTypes::Type2D);
        m_heightMapTex->setResolution(image.getWidth(), image.getHeight());
        m_heightMapTex->setPixelFormat(pixelFormat);
        m_heightMapTex->scheduleTransitionTo(GpuResidency::Resident);

        if (nullptr == m_heightMapStagingTexture)
        {
            m_heightMapStagingTexture = textureManager->getStagingTexture(image.getWidth(), image.getHeight(), 1u, 1u, pixelFormat);
        }
        m_heightMapStagingTexture->startMapRegion();
        TextureBox texBox = m_heightMapStagingTexture->mapRegion(image.getWidth(), image.getHeight(), 1u, 1u, pixelFormat);

        //for( uint8 mip=0; mip<numMipmaps; ++mip )
        texBox.copyFrom(image.getData(0));
        m_heightMapStagingTexture->stopMapRegion();
        m_heightMapStagingTexture->upload(texBox, m_heightMapTex, 0, 0, 0);

        m_currentHeightMapImageName = m_prefix + "_heightMap.png";
    }
    //-----------------------------------------------------------------------------------
    void Terra::createHeightmap(Image2& image, bool bMinimizeMemoryConsumption, bool bLowResShadow)
    {
        m_width = image.getWidth();
        m_depth = image.getHeight();
        m_depthWidthRatio = float(m_depth) / (float)(m_width);
        m_invWidth = 1.0f / float(m_width);
        m_invDepth = 1.0f / float(m_depth);

        //image.generateMipmaps( false, Image::FILTER_NEAREST );

        createHeightmapTexture(image);

        m_heightMap.resize(m_width * m_depth);
        m_oldHeightData.resize(m_width * m_depth, 0);
        m_newHeightData.resize(m_width * m_depth, 0);

        float fBpp = (float)(PixelFormatGpuUtils::getBytesPerPixel(image.getPixelFormat()) << 3u);
        const float maxValue = powf(2.0f, fBpp) - 1.0f;
        m_invMaxValue = 1.0f / maxValue;

        if (image.getPixelFormat() == PFG_R16_UNORM)
        {
            TextureBox texBox = image.getData(0);
            for (uint32 y = 0; y < m_depth; y++)
            {
                const uint16* RESTRICT_ALIAS data =
                    reinterpret_cast<uint16 * RESTRICT_ALIAS>(texBox.at(0, y, 0));
                for (uint32 x = 0; x < m_width; x++)
                {
                    auto height = (data[x] * m_invMaxValue) * m_height;
                    m_heightMap[y * m_width + x] = height;
                }
            }
        }

        m_xzRelativeSize = m_xzDimensions / Vector2(static_cast<Real>(m_width),
            static_cast<Real>(m_depth));

        createNormalTexture();

        m_prevLightDir = Vector3::ZERO;

        delete m_shadowMapper;
        m_shadowMapper = new ShadowMapper(mManager, m_compositorManager);
        // TODO: false = make low shadows configureable?
        m_shadowMapper->createShadowMap(getId(), m_heightMapTex, bLowResShadow);

        calculateOptimumSkirtSize();
    }

    void Terra::createHeightmap(float height, Ogre::uint16 imageWidth, Ogre::uint16 imageHeight, bool bMinimizeMemoryConsumption, bool bLowResShadow)
    {
        // https://ogrecave.github.io/ogre-next/api/2.2/_ogre22_changes.html

        destroyHeightmapTexture();

        m_width = imageWidth;
        m_depth = imageHeight;
        m_depthWidthRatio = m_depth / (float)(m_width);
        m_invWidth = 1.0f / m_width;
        m_invDepth = 1.0f / m_depth;

        // m_height = height;
        m_height = m_dimension.y;
        m_heightUnormScaled = m_height;

        TextureGpuManager* textureManager = mManager->getDestinationRenderSystem()->getTextureGpuManager();

        m_heightMapTex = textureManager->createTexture(m_prefix + "heightMap.png",
            GpuPageOutStrategy::SaveToSystemRam, /*TextureFlags::ManualTexture*/ TextureFlags::RenderToTexture | TextureFlags::AllowAutomipmaps, TextureTypes::Type2D);

        m_heightMapTex->setResolution(imageWidth, imageHeight);
        // m_heightMapTex->setNumMipmaps(PixelFormatGpuUtils::getMaxMipmapCount(m_heightMapTex->getWidth(), m_heightMapTex->getHeight()));
        m_heightMapTex->setPixelFormat(/*PFG_R8_UNORM*/ PFG_R16_UNORM /*PFG_R32_FLOAT*/);

        //Tell the texture we're going resident. The imageData pointer is only needed
        //if the texture pageout strategy is GpuPageOutStrategy::AlwaysKeepSystemRamCopy
        //which is in this example is not, so a nullptr would also work just fine.
        m_heightMapTex->scheduleTransitionTo(GpuResidency::Resident);

        //We have to upload the data via a StagingTexture, which acts as an intermediate stash
        if (nullptr == m_heightMapStagingTexture)
        {
            //memory that is both visible to CPU and GPU.
            m_heightMapStagingTexture = textureManager->getStagingTexture(m_heightMapTex->getWidth(), m_heightMapTex->getHeight(), 1u, 1u, m_heightMapTex->getPixelFormat());
        }
        //Call this function to indicate you're going to start calling mapRegion. startMapRegion
        //must be called from main thread.
        m_heightMapStagingTexture->startMapRegion();

        //Map region of the staging texture. This function can be called from any thread after
        //startMapRegion has already been called.
        TextureBox texBox = m_heightMapStagingTexture->mapRegion(m_heightMapTex->getWidth(), m_heightMapTex->getHeight(), 1u, 1u, m_heightMapTex->getPixelFormat());

        float fBpp = (float)(PixelFormatGpuUtils::getBytesPerPixel(m_heightMapTex->getPixelFormat()) << 3u);
        const float maxValue = powf(2.0f, fBpp) - 1.0f;
        m_invMaxValue = 1.0f / maxValue;

        // Note: If PFG_R16_UNORM, uint16 must be used!
        m_heightMap.resize(m_width * m_depth, 0);
        m_oldHeightData.resize(m_width * m_depth, 0);
        m_newHeightData.resize(m_width * m_depth, 0);

        if (m_heightMapTex->getPixelFormat() == PFG_R16_UNORM)
        {
            for (uint32 y = 0; y < m_depth; y++)
            {
                uint16* RESTRICT_ALIAS data = reinterpret_cast<uint16 * RESTRICT_ALIAS>(texBox.at(0, y, 0));
                for (uint32 x = 0; x < m_width; x++)
                {
                    float dataValue = height / (m_invMaxValue * m_dimension.y);
                    data[x] = dataValue;
                    auto height = (data[x] * m_invMaxValue) * m_height;
                    m_heightMap[y * m_width + x] = height;
                }
            }
        }

        //stopMapRegion indicates you're done calling mapRegion. Call this from the main thread.
        //It is your responsability to ensure you're done using all pointers returned from
        //previous mapRegion calls, and that you won't call it again.
        //You cannot upload until you've called this function.
        //Do NOT call startMapRegion again until you're done with upload() calls.
        m_heightMapStagingTexture->stopMapRegion();

        //Upload an area of the staging texture into the texture. Must be done from main thread.
        //The last bool parameter, 'skipSysRamCopy', is only relevant for AlwaysKeepSystemRamCopy
        //textures, and we set it to true because we know it's already up to date. Otherwise
        //it needs to be false.
        m_heightMapStagingTexture->upload(texBox, m_heightMapTex, 0, 0, 0);

        m_xzRelativeSize = m_xzDimensions / Vector2(static_cast<Real>(m_width), static_cast<Real>(m_depth));

        createNormalTexture();

        m_prevLightDir = Vector3::ZERO;

        delete m_shadowMapper;
        m_shadowMapper = new ShadowMapper(mManager, m_compositorManager);
        m_shadowMapper->_setSharedResources(m_sharedResources);
        m_shadowMapper->setMinimizeMemoryConsumption(bMinimizeMemoryConsumption);
        // TODO: false -> make low shadow res shadows configurable?
        m_shadowMapper->createShadowMap(getId(), m_heightMapTex, bLowResShadow);

        calculateOptimumSkirtSize();
    }

    //-----------------------------------------------------------------------------------
    void Terra::createNormalTexture(void)
    {
        destroyNormalTexture();

        TextureGpuManager* textureManager =
            mManager->getDestinationRenderSystem()->getTextureGpuManager();
        m_normalMapTex = textureManager->createTexture(
            "NormalMapTex_" + StringConverter::toString(getId()), GpuPageOutStrategy::SaveToSystemRam,
            TextureFlags::RenderToTexture | TextureFlags::AllowAutomipmaps, TextureTypes::Type2D);
        m_normalMapTex->setResolution(m_heightMapTex->getWidth(), m_heightMapTex->getHeight());
        m_normalMapTex->setNumMipmaps(PixelFormatGpuUtils::getMaxMipmapCount(
            m_normalMapTex->getWidth(), m_normalMapTex->getHeight()));
        if (textureManager->checkSupport(
            PFG_R10G10B10A2_UNORM, TextureTypes::Type2D,
            TextureFlags::RenderToTexture | TextureFlags::AllowAutomipmaps))
        {
            m_normalMapTex->setPixelFormat(PFG_R10G10B10A2_UNORM);
        }
        else
        {
            m_normalMapTex->setPixelFormat(PFG_RGBA8_UNORM);
        }
        m_normalMapTex->scheduleTransitionTo(GpuResidency::Resident);
        // Dangerous: No heighmap will be rendered for minimap
        // m_normalMapTex->notifyDataIsReady();

        MaterialPtr normalMapperMat =
            std::static_pointer_cast<Material>(MaterialManager::getSingleton().load(
            "Terra/GpuNormalMapper", ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME));
        Pass* pass = normalMapperMat->getTechnique(0)->getPass(0);
        TextureUnitState* texUnit = pass->getTextureUnitState(0);
        texUnit->setTexture(m_heightMapTex);

        // Normalize vScale for better precision in the shader math
        const Vector3 vScale =
            Vector3(m_xzRelativeSize.x, m_heightUnormScaled, m_xzRelativeSize.y).normalisedCopy();

        GpuProgramParametersSharedPtr psParams = pass->getFragmentProgramParameters();
        psParams->setNamedConstant(
            "heightMapResolution",
            Vector4(static_cast<Real>(m_width), static_cast<Real>(m_depth), 1, 1));
        psParams->setNamedConstant("vScale", vScale);

        CompositorChannelVec finalTargetChannels(1, CompositorChannel());
        finalTargetChannels[0] = m_normalMapTex;

        if (nullptr == mNormalMapCamera)
        {
            mNormalMapCamera = mManager->createCamera("NormalMapperDummyCamera");
            mNormalMapCamera->setQueryFlags(0 << 0);
        }

        if (nullptr == mNormalMapperWorkspace)
        {
            const IdString workspaceName = m_heightMapTex->getPixelFormat() == PFG_R16_UINT
                ? "Terra/GpuNormalMapperWorkspaceU16"
                : "Terra/GpuNormalMapperWorkspace";
            mNormalMapperWorkspace = m_compositorManager->addWorkspace(
                mManager, finalTargetChannels, mNormalMapCamera, workspaceName, false);

        }

        mNormalMapperWorkspace->_beginUpdate(true);
        mNormalMapperWorkspace->_update();
        mNormalMapperWorkspace->_endUpdate(true);
    }
    //-----------------------------------------------------------------------------------
    void Terra::destroyNormalTexture(void)
    {
        if (m_normalMapTex)
        {
            TextureGpuManager* textureManager =
                mManager->getDestinationRenderSystem()->getTextureGpuManager();
            textureManager->destroyTexture(m_normalMapTex);
            m_normalMapTex = 0;
        }
    }
    //-----------------------------------------------------------------------------------
    void Terra::createBlendWeightTexture(Ogre::uint16 imageWidth, Ogre::uint16 imageHeight)
    {
        destroyBlendWeightTexture();

        //const uint8 numMipmaps = image.getNumMipmaps();

        TextureGpuManager* textureManager = mManager->getDestinationRenderSystem()->getTextureGpuManager();
        m_blendWeightTex = textureManager->createTexture(m_prefix + "_detailMap.png", Ogre::GpuPageOutStrategy::SaveToSystemRam,
            TextureFlags::RenderToTexture, TextureTypes::Type2D);

        m_blendWeightTex->setResolution(imageWidth, imageHeight);
        // Attention: blend weights are linear scalar data, NOT color - must NOT use
        // an _SRGB format here. _SRGB formats trigger an automatic gamma-decode on
        // every GPU texture sample, which warps the linear 0..1 weight values and
        // causes visible banding/graininess in smooth brush gradients.
        m_blendWeightTex->setPixelFormat(PFG_RGBA8_UNORM);
        m_blendWeightTex->_transitionTo(GpuResidency::Resident, (uint8*)0);
        m_blendWeightTex->_setNextResidencyStatus(GpuResidency::Resident);

        if (nullptr == m_blendWeightStagingTexture)
        {
            m_blendWeightStagingTexture = textureManager->getStagingTexture(imageWidth, imageHeight, 1u, 1u, PFG_RGBA8_UNORM);
        }

        m_blendWeightStagingTexture->startMapRegion();
        TextureBox texBox = m_blendWeightStagingTexture->mapRegion(imageWidth, imageHeight, 1u, 1u, PFG_RGBA8_UNORM);

        const size_t bytesPerPixel = texBox.bytesPerPixel;

        m_oldBlendWeightData.resize(imageWidth * imageHeight * 4, 0);
        m_newBlendWeightData.resize(imageWidth * imageHeight * 4, 0);

        for (uint32 y = 0; y < imageHeight; y++)
        {
            uint8* RESTRICT_ALIAS pixBoxData = reinterpret_cast<uint8 * RESTRICT_ALIAS>(texBox.at(0, y, 0));
            for (uint32 x = 0; x < imageWidth; x++)
            {
                const size_t dstIdx = x * bytesPerPixel;
                float rgba[4];
                // Create blank texture without details
                rgba[0] = 1.0f;
                rgba[1] = 0.0f;
                rgba[2] = 0.0f;
                rgba[3] = 0.0f;
                PixelFormatGpuUtils::packColour(rgba, PFG_RGBA8_UNORM, &pixBoxData[dstIdx]);
            }
        }

        m_blendWeightStagingTexture->stopMapRegion();
        m_blendWeightStagingTexture->upload(texBox, m_blendWeightTex, 0);

        //  if (!m_blendWeightTex->isDataReady())
          //    m_blendWeightTex->notifyDataIsReady();

        const uint32 rowAlignment = 4u;
        const size_t totalBytes = PixelFormatGpuUtils::calculateSizeBytes(m_blendWeightTex->getWidth(), m_blendWeightTex->getHeight(),
            m_blendWeightTex->getDepth(), m_blendWeightTex->getNumSlices(), m_blendWeightTex->getPixelFormat(), m_blendWeightTex->getNumMipmaps(), rowAlignment);
        void* data = OGRE_MALLOC_SIMD(totalBytes, MEMCATEGORY_RESOURCE);

        m_blendWeightImage.loadDynamicImage(data, true, m_blendWeightTex);
    }

    void Terra::createBlendWeightTexture(Image2& image)
    {
        destroyBlendWeightTexture();

        TextureGpuManager* textureManager = mManager->getDestinationRenderSystem()->getTextureGpuManager();
        m_blendWeightTex = textureManager->createTexture(m_prefix + "_detailMap.png",
            GpuPageOutStrategy::SaveToSystemRam, TextureFlags::RenderToTexture, TextureTypes::Type2D);
        m_blendWeightTex->setResolution(image.getWidth(), image.getHeight());

        // Attention: force the LINEAR equivalent of whatever format the PNG loader
        // detected. Image2 commonly auto-tags 8-bit PNGs as PFG_RGBA8_UNORM_SRGB,
        // but blend weights are linear scalar data, not color, so that tag must be
        // overridden or the GPU will gamma-decode the weights on every sample.
        PixelFormatGpu loadedFormat = image.getPixelFormat();
        PixelFormatGpu linearFormat = PixelFormatGpuUtils::getEquivalentLinear(loadedFormat);
        m_blendWeightTex->setPixelFormat(linearFormat);
        m_blendWeightTex->scheduleTransitionTo(GpuResidency::Resident);

        m_currentBlendWeightImageName = m_prefix + "_detailMap.png";

        m_oldBlendWeightData.resize(image.getWidth() * image.getHeight() * 4, 0);
        m_newBlendWeightData.resize(image.getWidth() * image.getHeight() * 4, 0);
        size_t correctSizeInBytesFor1024x1024 = m_oldBlendWeightData.size();

        if (nullptr == m_blendWeightStagingTexture)
        {
            // Note: Maybe candidate is wrong, getting resolution of 2048x2048 instead of 1024x1024
            bool correctCandidate = false;
            while (false == correctCandidate)
            {
                m_blendWeightStagingTexture = textureManager->getStagingTexture(image.getWidth(), image.getHeight(), 1u, 1u, linearFormat);
                if (m_blendWeightStagingTexture->_getSizeBytes() == correctSizeInBytesFor1024x1024)
                {
                    correctCandidate = true;
                }
            }
        }

        m_blendWeightStagingTexture->startMapRegion();
        TextureBox texBox = m_blendWeightStagingTexture->mapRegion(image.getWidth(), image.getHeight(), 1u, 1u, linearFormat);

        //for( uint8 mip=0; mip<numMipmaps; ++mip )
        texBox.copyFrom(image.getData(0));

        // Note: If crash occurs, check if offsetscale0 etc. is 1 and not 128, also check the datablock.json in the workspace terra folder!
        // Or candidate is wrong, getting resolution of 2048x2048 instead of 1024x1024
        memcpy(&m_oldBlendWeightData[0], texBox.data, m_blendWeightStagingTexture->_getSizeBytes());
        memcpy(&m_newBlendWeightData[0], texBox.data, m_blendWeightStagingTexture->_getSizeBytes());

        const size_t bytesPerPixel = texBox.bytesPerPixel;

        m_blendWeightStagingTexture->stopMapRegion();

        m_blendWeightStagingTexture->upload(texBox, m_blendWeightTex, 0);
        // if (!m_blendWeightTex->isDataReady())
         //    m_blendWeightTex->notifyDataIsReady();

        m_blendWeightImage = image;
    }

    void Terra::calculateOptimumSkirtSize(void)
    {
        m_skirtSize = std::numeric_limits<float>::max();

        const uint32 basePixelDimension = m_basePixelDimension;
        const uint32 vertPixelDimension = static_cast<uint32>(m_basePixelDimension * m_depthWidthRatio);

        for (size_t y = vertPixelDimension - 1u; y < m_depth - 1u; y += vertPixelDimension)
        {
            const size_t ny = y + 1u;

            bool allEqualInLine = true;
            float minHeight = m_heightMap[y * m_width];
            for (size_t x = 0; x < m_width; x++)
            {
                const float minValue = std::min(m_heightMap[y * m_width + x],
                    m_heightMap[ny * m_width + x]);
                minHeight = std::min(minValue, minHeight);
                allEqualInLine &= m_heightMap[y * m_width + x] == m_heightMap[ny * m_width + x];
            }

            if (!allEqualInLine)
                m_skirtSize = std::min(minHeight, m_skirtSize);
        }

        for (size_t x = basePixelDimension - 1u; x < m_width - 1u; x += basePixelDimension)
        {
            const size_t nx = x + 1u;

            bool allEqualInLine = true;
            float minHeight = m_heightMap[x];
            for (size_t y = 0; y < m_depth; y++)
            {
                const float minValue = std::min(m_heightMap[y * m_width + x],
                    m_heightMap[y * m_width + nx]);
                minHeight = std::min(minValue, minHeight);
                allEqualInLine &= m_heightMap[y * m_width + x] == m_heightMap[y * m_width + nx];
            }

            if (!allEqualInLine)
                m_skirtSize = std::min(minHeight, m_skirtSize);
        }

        m_skirtSize /= m_height;

        // Many Android GPUs don't support PFG_R16_UNORM so we scale it by hand
        if (m_heightMapTex->getPixelFormat() == PFG_R16_UINT)
            m_skirtSize *= 65535.0f;
    }

    //-----------------------------------------------------------------------------------
    void Terra::createTerrainCells()
    {
        {
            // Find out how many TerrainCells we need. I think this might be
            // solved analitically with a power series. But my math is rusty.
            const uint32 basePixelDimension = m_basePixelDimension;
            const uint32 vertPixelDimension =
                static_cast<uint32>(m_basePixelDimension * m_depthWidthRatio);
            const uint32 maxPixelDimension = std::max(basePixelDimension, vertPixelDimension);
            const uint32 maxRes = std::max(m_width, m_depth);

            uint32 numCells = 16u;  // 4x4
            uint32 accumDim = 0u;
            uint32 iteration = 1u;
            while (accumDim < maxRes)
            {
                numCells += 12u;  // 4x4 - 2x2
                accumDim += maxPixelDimension * (1u << iteration);
                ++iteration;
            }

            numCells += 12u;
            accumDim += maxPixelDimension * (1u << iteration);
            ++iteration;

            for (size_t i = 0u; i < 2u; ++i)
            {
                m_terrainCells[i].clear();
                m_terrainCells[i].resize(numCells, TerrainCell(this));
            }
        }

        VaoManager* vaoManager = mManager->getDestinationRenderSystem()->getVaoManager();

        for (size_t i = 0u; i < 2u; ++i)
        {
            std::vector<TerrainCell>::iterator itor = m_terrainCells[i].begin();
            std::vector<TerrainCell>::iterator endt = m_terrainCells[i].end();

            const std::vector<TerrainCell>::iterator begin = itor;

            while (itor != endt)
            {
                itor->initialize(vaoManager, (itor - begin) >= 16u);
                ++itor;
            }
        }
    }
    //-----------------------------------------------------------------------------------

    //-----------------------------------------------------------------------------------
    inline GridPoint Terra::worldToGrid(const Vector3& vPos) const
    {
        GridPoint retVal;
        const float fWidth = static_cast<float>(m_width);
        const float fDepth = static_cast<float>(m_depth);

        const float fX = floorf(((vPos.x - m_terrainOrigin.x) * m_xzInvDimensions.x) * fWidth);
        const float fZ = floorf(((vPos.z - m_terrainOrigin.z) * m_xzInvDimensions.y) * fDepth);
        retVal.x = fX >= 0.0f ? static_cast<int32>(fX) : -1;
        retVal.z = fZ >= 0.0f ? static_cast<int32>(fZ) : -1;

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    inline Vector2 Terra::gridToWorld(const GridPoint& gPos) const
    {
        Vector2 retVal;
        const float fWidth = static_cast<float>(m_width);
        const float fDepth = static_cast<float>(m_depth);

        retVal.x = (float(gPos.x) / fWidth) * m_xzDimensions.x + m_terrainOrigin.x;
        retVal.y = (float(gPos.z) / fDepth) * m_xzDimensions.y + m_terrainOrigin.z;

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    bool Terra::isVisible(const GridPoint& gPos, const GridPoint& gSize) const
    {
        if (gPos.x >= static_cast<int32>(m_width) || gPos.z >= static_cast<int32>(m_depth) ||
            gPos.x + gSize.x <= 0 || gPos.z + gSize.z <= 0)
        {
            //Outside terrain bounds.
            return false;
        }

        //        return true;

        const Vector2 cellPos = gridToWorld(gPos);
        const Vector2 cellSize(Real(gSize.x + 1) * m_xzRelativeSize.x,
            Real(gSize.z + 1) * m_xzRelativeSize.y);

        const Vector3 vHalfSizeYUp = Vector3(cellSize.x, m_height, cellSize.y) * 0.5f;
        const Vector3 vCenter =
            fromYUp(Vector3(cellPos.x, m_terrainOrigin.y, cellPos.y) + vHalfSizeYUp);
        const Vector3 vHalfSize = fromYUpSignPreserving(vHalfSizeYUp);
#if 0
        for (unsigned i = 0; i < 6u; ++i)
        {
            //Skip far plane if view frustum is infinite
            if (i == FRUSTUM_PLANE_FAR && m_camera->getFarClipDistance() == 0)
                continue;

            Plane::Side side = m_camera->getFrustumPlane((uint16)i).getSide(vCenter, vHalfSize);

            //We only need one negative match to know the obj is outside the frustum
            if (side == Plane::NEGATIVE_SIDE)
                return false;
        }
#endif
        return true;
    }
    //-----------------------------------------------------------------------------------
    void Terra::addRenderable(const GridPoint& gridPos, const GridPoint& cellSize, uint32 lodLevel)
    {
        TerrainCell* cell = &m_terrainCells[0][m_currentCell++];
        cell->setOrigin(gridPos, cellSize.x, cellSize.z, lodLevel);
        m_collectedCells[0].push_back(cell);
    }
    //-----------------------------------------------------------------------------------
    void Terra::optimizeCellsAndAdd(void)
    {
        //Keep iterating until m_collectedCells[0] stops shrinking
        size_t numCollectedCells = std::numeric_limits<size_t>::max();
        while (numCollectedCells != m_collectedCells[0].size())
        {
            numCollectedCells = m_collectedCells[0].size();

            if (m_collectedCells[0].size() > 1)
            {
                m_collectedCells[1].clear();

                std::vector<TerrainCell*>::const_iterator itor = m_collectedCells[0].begin();
                std::vector<TerrainCell*>::const_iterator end = m_collectedCells[0].end();

                while (end - itor >= 2u)
                {
                    TerrainCell* currCell = *itor;
                    TerrainCell* nextCell = *(itor + 1);

                    m_collectedCells[1].push_back(currCell);
                    if (currCell->merge(nextCell))
                        itor += 2;
                    else
                        ++itor;
                }

                while (itor != end)
                    m_collectedCells[1].push_back(*itor++);

                m_collectedCells[1].swap(m_collectedCells[0]);
            }
        }

        std::vector<TerrainCell*>::const_iterator itor = m_collectedCells[0].begin();
        std::vector<TerrainCell*>::const_iterator end = m_collectedCells[0].end();
        while (itor != end)
            mRenderables.push_back(*itor++);

        m_collectedCells[0].clear();
    }
    //-----------------------------------------------------------------------------------
    void Terra::setSharedResources(TerraSharedResources* sharedResources)
    {
        m_sharedResources = sharedResources;
        if (m_shadowMapper)
            m_shadowMapper->_setSharedResources(sharedResources);
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
#if 0
    void Terra::update(const Vector3& lightDir, float lightEpsilon)
    {
        const float lightCosAngleChange =
            Math::Clamp((float)m_prevLightDir.dotProduct(lightDir.normalisedCopy()), -1.0f, 1.0f);
        if (lightCosAngleChange <= (1.0f - lightEpsilon))
        {
            m_shadowMapper->updateShadowMap(toYUp(lightDir), m_xzDimensions, m_height);
            m_prevLightDir = lightDir.normalisedCopy();
        }

        mRenderables.clear();
        m_currentCell = 0;

        const Vector3 camPos = toYUp(m_camera->getDerivedPosition());

        // Computes the REAL world-space distance (XZ plane only, height ignored
        // since terrain LOD shouldn't care about vertical camera offset) from the
        // camera to a specific cell's center, then asks the falloff curve how many
        // EXTRA decimation levels that cell deserves on top of the ring baseline.
        // This is per-cell, not per-ring, so it genuinely responds to camera
        // movement: two cells in the same ring can get different results if one
        // is on the near side of the terrain and one is on the far side.
        auto computeCellAppliedLod = [this, &camPos](const GridPoint& cellPos, const GridPoint& cellGridSize, uint32 baselineLod) -> uint32
        {
            const Vector2 cellOriginWorld = gridToWorld(cellPos);
            const float cellHalfWidth = float(cellGridSize.x) * m_xzRelativeSize.x * 0.5f;
            const float cellHalfDepth = float(cellGridSize.z) * m_xzRelativeSize.y * 0.5f;

            const float cellCenterWorldX = cellOriginWorld.x + cellHalfWidth;
            const float cellCenterWorldZ = cellOriginWorld.y + cellHalfDepth;

            const float dx = cellCenterWorldX - camPos.x;
            const float dz = cellCenterWorldZ - camPos.z;
            const float distanceFromCamera = std::sqrt(dx * dx + dz * dz);

            const uint32 extraLodLevels = computeLodLevelForDistance(distanceFromCamera);
            return baselineLod + extraLodLevels;
        };

        const int32 basePixelDimension = static_cast<int32>(m_basePixelDimension);
        const int32 vertPixelDimensionBase =
            static_cast<int32>(float(m_basePixelDimension) * m_depthWidthRatio);

        const float pixelsPerWorldUnitX = (m_xzRelativeSize.x > 1e-6f) ? (1.0f / m_xzRelativeSize.x) : 1.0f;
        const float pixelsPerWorldUnitZ = (m_xzRelativeSize.y > 1e-6f) ? (1.0f / m_xzRelativeSize.y) : 1.0f;

        int32 targetCellPixelsX = static_cast<int32>(m_lodRing0WorldSize * pixelsPerWorldUnitX);
        int32 targetCellPixelsZ = static_cast<int32>(m_lodRing0WorldSize * m_depthWidthRatio * pixelsPerWorldUnitZ);

        int32 cellPixelsX = std::max(basePixelDimension,
            (targetCellPixelsX / basePixelDimension) * basePixelDimension);
        int32 cellPixelsZ = std::max(vertPixelDimensionBase,
            (targetCellPixelsZ / vertPixelDimensionBase) * vertPixelDimensionBase);

        GridPoint cellSize;
        cellSize.x = cellPixelsX;
        cellSize.z = cellPixelsZ;

        GridPoint camCenter = worldToGrid(camPos);
        camCenter.x = (camCenter.x / cellSize.x) * cellSize.x;
        camCenter.z = (camCenter.z / cellSize.z) * cellSize.z;

        uint32 currentLod = 0;

        // LOD 0: Add full 4x4 grid. Per-cell distance is evaluated here too -
        // normally these cells are all close to the camera so extraLodLevels
        // will compute to 0, but this keeps the system honest near terrain
        // edges where ring0 itself might already be far from camera.
        for (int32 z = -2; z < 2; ++z)
        {
            for (int32 x = -2; x < 2; ++x)
            {
                GridPoint pos = camCenter;
                pos.x += x * cellSize.x;
                pos.z += z * cellSize.z;

                if (isVisible(pos, cellSize))
                {
                    const uint32 appliedLodLevel = computeCellAppliedLod(pos, cellSize, currentLod);
                    addRenderable(pos, cellSize, appliedLodLevel);
                }
            }
        }

        optimizeCellsAndAdd();

        m_currentCell = 16u;  // The first 16 cells don't use skirts.

        const uint32 maxRes = std::max(m_width, m_depth);

        static int logCounter = 0;

        size_t numObjectsAdded = std::numeric_limits<size_t>::max();
        uint32 previousRingAppliedLod = 0u;

        while (numObjectsAdded != m_currentCell ||
            (mRenderables.empty() && (1u << currentLod) <= maxRes))
        {
            numObjectsAdded = m_currentCell;

            cellSize.x <<= 1u;
            cellSize.z <<= 1u;
            ++currentLod;

            // Evaluate distance ONCE per ring (using one representative cell,
            // not every cell individually) so every cell in this ring gets the
            // exact same applied LOD - this removes intra-ring cracks.
            GridPoint representativePos = camCenter;
            representativePos.x += (-2) * cellSize.x;
            representativePos.z += 1 * cellSize.z;
            const uint32 desiredAppliedLod = computeCellAppliedLod(representativePos, cellSize, currentLod);

            // Clamp so the TOTAL applied lod can only climb by +1 over the
            // previous ring. This restores the "neighbors differ by at most one
            // LOD level" rule that calculateOptimumSkirtSize's fixed-size skirts
            // depend on. Without this, the far-field curve can make adjacent
            // rings jump many levels at once on steep terrain - the skirt can't
            // cover that gap, producing an exposed wall/crack.
            const uint32 appliedLodLevel = std::min(desiredAppliedLod, previousRingAppliedLod + 1u);
            previousRingAppliedLod = appliedLodLevel;

            // Row 0
            {
                const int32 z = 1;
                for (int32 x = -2; x < 2; ++x)
                {
                    GridPoint pos = camCenter;
                    pos.x += x * cellSize.x;
                    pos.z += z * cellSize.z;

                    if (isVisible(pos, cellSize))
                        addRenderable(pos, cellSize, appliedLodLevel);
                }
            }
            // Row 3
            {
                const int32 z = -2;
                for (int32 x = -2; x < 2; ++x)
                {
                    GridPoint pos = camCenter;
                    pos.x += x * cellSize.x;
                    pos.z += z * cellSize.z;

                    if (isVisible(pos, cellSize))
                        addRenderable(pos, cellSize, appliedLodLevel);
                }
            }
            // Cells [0, 1] & [0, 2];
            {
                const int32 x = -2;
                for (int32 z = -1; z < 1; ++z)
                {
                    GridPoint pos = camCenter;
                    pos.x += x * cellSize.x;
                    pos.z += z * cellSize.z;

                    if (isVisible(pos, cellSize))
                        addRenderable(pos, cellSize, appliedLodLevel);
                }
            }
            // Cells [3, 1] & [3, 2];
            {
                const int32 x = 1;
                for (int32 z = -1; z < 1; ++z)
                {
                    GridPoint pos = camCenter;
                    pos.x += x * cellSize.x;
                    pos.z += z * cellSize.z;

                    if (isVisible(pos, cellSize))
                        addRenderable(pos, cellSize, appliedLodLevel);
                }
            }

            optimizeCellsAndAdd();
        }
    }
#endif

    void Terra::update(const Vector3& lightDir, float lightEpsilon)
    {
        const float lightCosAngleChange =
            Math::Clamp((float)m_prevLightDir.dotProduct(lightDir.normalisedCopy()), -1.0f, 1.0f);
        if (lightCosAngleChange <= (1.0f - lightEpsilon))
        {
            m_shadowMapper->updateShadowMap(toYUp(lightDir), m_xzDimensions, m_height);
            m_prevLightDir = lightDir.normalisedCopy();
        }

        mRenderables.clear();
        m_currentCell = 0;

        const Vector3 camPos = toYUp(m_camera->getDerivedPosition());

        // -----------------------------------------------------------------
        // LOD footprint decoupling fix:
        //
        // BasePixelDimension previously controlled BOTH vertex density (how
        // many heightmap pixels each cell spans) AND the world-space size of
        // the LOD 0 ring (since cellSize is expressed in pixels and pixels
        // map directly to world units via m_xzRelativeSize). For small
        // terrains with high pixel resolution, BasePixelDimension had to stay
        // large just to keep the LOD 0 ring world-space footprint reasonable
        // -- but that also meant fewer vertices per cell -> visibly blocky
        // "huge pixels" once BasePixelDimension was lowered to fix LOD ring
        // sizing.
        //
        // Fix: keep BasePixelDimension as the vertex-density control (tied to
        // m_basePixelDimension, unchanged), but introduce an independent
        // world-space "innermost LOD ring radius" (m_lodRing0WorldSize) that
        // determines how large LOD 0 is in world units. We then compute how
        // many BasePixelDimension-sized cells fit into that world-space target,
        // and use THAT as the effective starting cellSize for the LOD loop --
        // never going below 1 cell so degenerate configs don't divide by zero.
        //
        // This means: BasePixelDimension only affects vertex density per cell
        // (visual smoothness), while m_lodRing0WorldSize controls how far you
        // must travel before LOD increases, completely independent of terrain
        // pixel resolution or world size.
        // -----------------------------------------------------------------

        const int32 basePixelDimension = static_cast<int32>(m_basePixelDimension);
        const int32 vertPixelDimensionBase =
            static_cast<int32>(float(m_basePixelDimension) * m_depthWidthRatio);

        // Convert desired world-space LOD0 cell size into pixel units using the
        // terrain's pixel-to-world ratio (m_xzRelativeSize = world units per pixel).
        const float pixelsPerWorldUnitX = (m_xzRelativeSize.x > 1e-6f) ? (1.0f / m_xzRelativeSize.x) : 1.0f;
        const float pixelsPerWorldUnitZ = (m_xzRelativeSize.y > 1e-6f) ? (1.0f / m_xzRelativeSize.y) : 1.0f;

        int32 targetCellPixelsX = static_cast<int32>(m_lodRing0WorldSize * pixelsPerWorldUnitX);
        int32 targetCellPixelsZ = static_cast<int32>(m_lodRing0WorldSize * m_depthWidthRatio * pixelsPerWorldUnitZ);

        // Snap to a whole multiple of basePixelDimension so cell boundaries still
        // align with vertex grid lines (avoids seams/cracks between cells).
        int32 cellPixelsX = std::max(basePixelDimension,
            (targetCellPixelsX / basePixelDimension) * basePixelDimension);
        int32 cellPixelsZ = std::max(vertPixelDimensionBase,
            (targetCellPixelsZ / vertPixelDimensionBase) * vertPixelDimensionBase);

        GridPoint cellSize;
        cellSize.x = cellPixelsX;
        cellSize.z = cellPixelsZ;

        // Quantize the camera position to cellSize steps (was basePixelDimension).
        GridPoint camCenter = worldToGrid(camPos);
        camCenter.x = (camCenter.x / cellSize.x) * cellSize.x;
        camCenter.z = (camCenter.z / cellSize.z) * cellSize.z;

        /*static int logCounter = 0;
        if (++logCounter % 60 == 0)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                "[Terra::update] camPos=" + Ogre::StringConverter::toString(camPos) +
                " m_lodRing0WorldSize=" + Ogre::StringConverter::toString(m_lodRing0WorldSize) +
                " pixelsPerWorldUnitX=" + Ogre::StringConverter::toString(pixelsPerWorldUnitX) +
                " targetCellPixelsX=" + Ogre::StringConverter::toString(targetCellPixelsX) +
                " cellSize.x=" + Ogre::StringConverter::toString(cellSize.x) +
                " cellSize.z=" + Ogre::StringConverter::toString(cellSize.z) +
                " basePixelDimension=" + Ogre::StringConverter::toString(basePixelDimension) +
                " camCenter.x=" + Ogre::StringConverter::toString(camCenter.x) +
                " camCenter.z=" + Ogre::StringConverter::toString(camCenter.z));
        }*/

        uint32 currentLod = 0;

        // LOD 0: Add full 4x4 grid
        for (int32 z = -2; z < 2; ++z)
        {
            for (int32 x = -2; x < 2; ++x)
            {
                GridPoint pos = camCenter;
                pos.x += x * cellSize.x;
                pos.z += z * cellSize.z;

                if (isVisible(pos, cellSize))
                    addRenderable(pos, cellSize, currentLod);
            }
        }

        optimizeCellsAndAdd();

        m_currentCell = 16u;  // The first 16 cells don't use skirts.

        const uint32 maxRes = std::max(m_width, m_depth);

        size_t numObjectsAdded = std::numeric_limits<size_t>::max();
        while (numObjectsAdded != m_currentCell ||
            (mRenderables.empty() && (1u << currentLod) <= maxRes))
        {
            numObjectsAdded = m_currentCell;

            cellSize.x <<= 1u;
            cellSize.z <<= 1u;
            ++currentLod;

            // Row 0
            {
                const int32 z = 1;
                for (int32 x = -2; x < 2; ++x)
                {
                    GridPoint pos = camCenter;
                    pos.x += x * cellSize.x;
                    pos.z += z * cellSize.z;

                    if (isVisible(pos, cellSize))
                        addRenderable(pos, cellSize, currentLod);
                }
            }
            // Row 3
            {
                const int32 z = -2;
                for (int32 x = -2; x < 2; ++x)
                {
                    GridPoint pos = camCenter;
                    pos.x += x * cellSize.x;
                    pos.z += z * cellSize.z;

                    if (isVisible(pos, cellSize))
                        addRenderable(pos, cellSize, currentLod);
                }
            }
            // Cells [0, 1] & [0, 2];
            {
                const int32 x = -2;
                for (int32 z = -1; z < 1; ++z)
                {
                    GridPoint pos = camCenter;
                    pos.x += x * cellSize.x;
                    pos.z += z * cellSize.z;

                    if (isVisible(pos, cellSize))
                        addRenderable(pos, cellSize, currentLod);
                }
            }
            // Cells [3, 1] & [3, 2];
            {
                const int32 x = 1;
                for (int32 z = -1; z < 1; ++z)
                {
                    GridPoint pos = camCenter;
                    pos.x += x * cellSize.x;
                    pos.z += z * cellSize.z;

                    if (isVisible(pos, cellSize))
                        addRenderable(pos, cellSize, currentLod);
                }
            }

            optimizeCellsAndAdd();
        }
    }
    //-----------------------------------------------------------------------------------
    void Terra::load(const String& heightMapTextureName, const String& blendWeightTextureName, Vector3& center, Vector3& dimensions, bool bMinimizeMemoryConsumption, bool bLowResShadow)
    {
        Ogre::Image2 heightMapImage;
        heightMapImage.load(heightMapTextureName, ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);


        Ogre::Image2 blendWeightImage;
        if (false == blendWeightTextureName.empty())
        {
            blendWeightImage.load(blendWeightTextureName, ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
        }

        load(heightMapImage, blendWeightImage, center, dimensions, bMinimizeMemoryConsumption, bLowResShadow);
    }
    //-----------------------------------------------------------------------------------
    void Terra::load(Image2& heightMapImage, Image2& blendWeightImage, Vector3& center, Vector3& dimensions, bool bMinimizeMemoryConsumption, bool bLowResShadow)
    {
        // Use sign-preserving because origin in XZ plane is always from
        // bottom-left to top-right.
        // If we use toYUp, we'll start from top-right and go up and right
        m_terrainOrigin = toYUpSignPreserving(center - dimensions * 0.5f);
        center = toYUp(center);
        dimensions = toYUpSignPreserving(dimensions);
        m_dimension = dimensions;

        m_xzDimensions = Vector2(dimensions.x, dimensions.z);
        m_xzInvDimensions = 1.0f / m_xzDimensions;
        // m_height = center.y;
        m_height = dimensions.y;
        m_heightUnormScaled = m_height;
        m_basePixelDimension = 64u;

        // Attention: Verknaupt, since there can be a heighmap but no detail map, so detail map must be created with width height
        // Better: Use 4 function will all combinations!
        if (blendWeightImage.getWidth() > 1)
        {
            createBlendWeightTexture(blendWeightImage);
        }
        else
        {
            createBlendWeightTexture(1024, 1024);
        }

        if (heightMapImage.getWidth() > 1)
        {
            createHeightmap(heightMapImage, bMinimizeMemoryConsumption, bLowResShadow);
        }
        else
        {
            //If an image was not provided it's assumed we're working with the singular height rather than the image.
            createHeightmap(center.y, 1024, 1024, bMinimizeMemoryConsumption, bLowResShadow);
        }

        {
            //Find out how many TerrainCells we need. I think this might be
            //solved analitically with a power series. But my math is rusty.
            const uint32 basePixelDimension = m_basePixelDimension;
            const uint32 vertPixelDimension = static_cast<uint32>(m_basePixelDimension *
                m_depthWidthRatio);
            const uint32 maxPixelDimension = std::max(basePixelDimension, vertPixelDimension);
            const uint32 maxRes = std::max(m_width, m_depth);

            uint32 numCells = 16u; //4x4
            uint32 accumDim = 0u;
            uint32 iteration = 1u;
            while (accumDim < maxRes)
            {
                numCells += 12u; //4x4 - 2x2
                accumDim += maxPixelDimension * (1u << iteration);
                ++iteration;
            }

            numCells += 12u;
            accumDim += maxPixelDimension * (1u << iteration);
            ++iteration;


            for (size_t i = 0u; i < 2u; ++i)
            {
                m_terrainCells[i].clear();
                m_terrainCells[i].resize(numCells, TerrainCell(this));
            }
        }

        VaoManager* vaoManager = mManager->getDestinationRenderSystem()->getVaoManager();

        for (size_t i = 0u; i < 2u; ++i)
        {
            std::vector<TerrainCell>::iterator itor = m_terrainCells[i].begin();
            std::vector<TerrainCell>::iterator endt = m_terrainCells[i].end();

            const std::vector<TerrainCell>::iterator begin = itor;

            while (itor != endt)
            {
                itor->initialize(vaoManager, (itor - begin) >= 16u);
                ++itor;
            }
        }
    }

    void Terra::load(float height, Ogre::uint16 imageWidth, Ogre::uint16 imageHeight, Vector3& center, Vector3& dimensions, bool bMinimizeMemoryConsumption, bool bLowResShadow)
    {
        _loadInternal(nullptr, nullptr, center, dimensions, "", "", height, imageWidth, imageHeight, bMinimizeMemoryConsumption, bLowResShadow);
    }
    //-----------------------------------------------------------------------------------
    void Terra::_loadInternal(Image2* heightMapImage, Image2* blendWeightImage, Vector3& center, Vector3& dimensions,
        const String& heightMapImageName, const String& blendWeightImageName, float height, Ogre::uint16 imageWidth, Ogre::uint16 imageHeight, bool bMinimizeMemoryConsumption, bool bLowResShadow)
    {
        // Use sign-preserving because origin in XZ plane is always from
        // bottom-left to top-right.
        // If we use toYUp, we'll start from top-right and go up and right
        m_terrainOrigin = toYUpSignPreserving(center - dimensions * 0.5f);
        center = toYUp(center);
        dimensions = toYUpSignPreserving(dimensions);
        m_dimension = dimensions;

        m_xzDimensions = Vector2(dimensions.x, dimensions.z);
        m_xzInvDimensions = 1.0f / m_xzDimensions;
        // m_height = height;
        m_height = m_dimension.y;
        m_basePixelDimension = 64u;

        if (blendWeightImage)
        {
            createBlendWeightTexture(*blendWeightImage);
        }
        else
        {
            createBlendWeightTexture(imageWidth, imageHeight);
        }

        if (heightMapImage)
        {
            createHeightmap(*heightMapImage, bMinimizeMemoryConsumption, bLowResShadow);
        }
        else
        {
            //If an image was not provided it's assumed we're working with the singular height rather than the image.
            createHeightmap(height, imageWidth, imageHeight, bMinimizeMemoryConsumption, bLowResShadow);
        }

        {
            //Find out how many TerrainCells we need. I think this might be
            //solved analitically with a power series. But my math is rusty.
            const uint32 basePixelDimension = m_basePixelDimension;
            const uint32 vertPixelDimension = static_cast<uint32>(m_basePixelDimension *
                m_depthWidthRatio);
            const uint32 maxPixelDimension = std::max(basePixelDimension, vertPixelDimension);
            const uint32 maxRes = std::max(m_width, m_depth);

            uint32 numCells = 16u; //4x4
            uint32 accumDim = 0u;
            uint32 iteration = 1u;
            while (accumDim < maxRes)
            {
                numCells += 12u; //4x4 - 2x2
                accumDim += maxPixelDimension * (1u << iteration);
                ++iteration;
            }

            numCells += 12u;
            accumDim += maxPixelDimension * (1u << iteration);
            ++iteration;


            for (size_t i = 0u; i < 2u; ++i)
            {
                m_terrainCells[i].clear();
                m_terrainCells[i].resize(numCells, TerrainCell(this));
            }
        }

        VaoManager* vaoManager = mManager->getDestinationRenderSystem()->getVaoManager();

        for (size_t i = 0u; i < 2u; ++i)
        {
            std::vector<TerrainCell>::iterator itor = m_terrainCells[i].begin();
            std::vector<TerrainCell>::iterator endt = m_terrainCells[i].end();

            const std::vector<TerrainCell>::iterator begin = itor;

            while (itor != endt)
            {
                itor->initialize(vaoManager, (itor - begin) >= 16u);
                ++itor;
            }
        }

        createTerrainCells();
    }
    //-----------------------------------------------------------------------------------
    void Terra::setBasePixelDimension(uint32 basePixelDimension)
    {
        m_basePixelDimension = basePixelDimension;
        if (!m_heightMap.empty() && !m_terrainCells->empty())
        {
            HlmsDatablock* datablock = m_terrainCells[0].back().getDatablock();

            calculateOptimumSkirtSize();
            createTerrainCells();

            if (datablock)
            {
                for (size_t i = 0u; i < 2u; ++i)
                {
                    for (TerrainCell& terrainCell : m_terrainCells[i])
                        terrainCell.setDatablock(datablock);
                }
            }
        }
    }
    //-----------------------------------------------------------------------------------
    bool Terra::getHeightAt(Vector3& vPosArg) const
    {
        bool retVal = false;

        Vector3 vPos = toYUp(vPosArg);

        GridPoint pos2D = worldToGrid(vPos);

        const int32 iWidth = static_cast<int32>(m_width);
        const int32 iDepth = static_cast<int32>(m_depth);

        if (pos2D.x < iWidth - 1 && pos2D.z < iDepth - 1)
        {
            const Vector2 vPos2D = gridToWorld(pos2D);

            const float dx = (vPos.x - vPos2D.x) * float(m_width) * m_xzInvDimensions.x;
            const float dz = (vPos.z - vPos2D.y) * float(m_depth) * m_xzInvDimensions.y;

            float a, b, c;
            const float h00 = m_heightMap[size_t(pos2D.z * iWidth + pos2D.x)];
            const float h11 = m_heightMap[size_t((pos2D.z + 1) * iWidth + pos2D.x + 1)];

            c = h00;
            if (dx < dz)
            {
                //Plane eq: y = ax + bz + c
                //x=0 z=0 -> c		= h00
                //x=0 z=1 -> b + c	= h01 -> b = h01 - c
                //x=1 z=1 -> a + b + c  = h11 -> a = h11 - b - c
                const float h01 = m_heightMap[size_t((pos2D.z + 1) * iWidth + pos2D.x)];

                b = h01 - c;
                a = h11 - b - c;
            }
            else
            {
                //Plane eq: y = ax + bz + c
                //x=0 z=0 -> c		= h00
                //x=1 z=0 -> a + c	= h10 -> a = h10 - c
                //x=1 z=1 -> a + b + c  = h11 -> b = h11 - a - c
                const float h10 = m_heightMap[size_t(pos2D.z * iWidth + pos2D.x + 1)];

                a = h10 - c;
                b = h11 - a - c;
            }

            vPos.y = a * dx + b * dz + c + m_terrainOrigin.y;
            retVal = true;
        }

        vPosArg = fromYUp(vPos);

        return retVal;
    }

    std::vector<int> Terra::getLayerAt(const Ogre::Vector3& position)
    {
        Ogre::GridPoint point = this->worldToGrid(position);

        std::vector<int> layers(4);

        Ogre::ColourValue cVal = m_blendWeightImage.getColourAt(point.x, point.z, 0);

        layers[0] = static_cast<int>(cVal.r * 255.0f);
        layers[1] = static_cast<int>(cVal.g * 255.0f);
        layers[2] = static_cast<int>(cVal.b * 255.0f);
        layers[3] = static_cast<int>(cVal.a * 255.0f);

        return layers;
    }
    //-----------------------------------------------------------------------------------
    void Terra::setDatablock(HlmsDatablock* datablock)
    {
        if (!datablock && !m_terrainCells[0].empty() && m_terrainCells[0].back().getDatablock())
        {
            // Unsetting the datablock. We have no way of unlinking later on. Do it now
            HlmsDatablock* oldDatablock = m_terrainCells[0].back().getDatablock();
            OGRE_ASSERT_HIGH(dynamic_cast<HlmsTerra*>(oldDatablock->getCreator()));
            HlmsTerra* hlms = static_cast<HlmsTerra*>(oldDatablock->getCreator());
            hlms->_unlinkTerra(this);
        }

        for (size_t i = 0u; i < 2u; ++i)
        {
            std::vector<TerrainCell>::iterator itor = m_terrainCells[i].begin();
            std::vector<TerrainCell>::iterator end = m_terrainCells[i].end();

            while (itor != end)
            {
                itor->setDatablock(datablock);
                ++itor;
            }
        }

        Ogre::HlmsTerraDatablock* terraDataBlock = dynamic_cast<Ogre::HlmsTerraDatablock*>(datablock);
        terraDataBlock->setTexture(Ogre::TerraTextureTypes::TERRA_DETAIL_WEIGHT, m_blendWeightTex);

        // Triplanar enabled
        // Causes ugly scale effects
        // terraDataBlock->setDetailTriplanarDiffuseEnabled(true);
        //terraDataBlock->setDetailTriplanarRoughnessEnabled(true);
        // terraDataBlock->setDetailTriplanarMetalnessEnabled(true);
        // terraDataBlock->setDetailTriplanarNormalEnabled(true);

        if (mHlmsTerraIndex == std::numeric_limits<uint32>::max())
        {
            OGRE_ASSERT_HIGH(dynamic_cast<HlmsTerra*>(datablock->getCreator()));
            HlmsTerra* hlms = static_cast<HlmsTerra*>(datablock->getCreator());
            hlms->_linkTerra(this);
        }
    }
    //-----------------------------------------------------------------------------------
    Ogre::TextureGpu* Terra::_getShadowMapTex(void) const
    {
        return m_shadowMapper->getShadowMapTex();
    }
    //-----------------------------------------------------------------------------------
    Vector2 Terra::getTerrainXZCenter(void) const
    {
        return Vector2(m_terrainOrigin.x + m_xzDimensions.x * 0.5f,
            m_terrainOrigin.z + m_xzDimensions.y * 0.5f);
    }

    Vector3 Terra::getTerrainOriginRaw(void) const
    {
        return m_terrainOrigin;
    }

    Vector3 Terra::getTerrainOrigin(void) const
    {
        return fromYUpSignPreserving(m_terrainOrigin);
    }

    //-----------------------------------------------------------------------------------
    const String& Terra::getMovableType(void) const
    {
        static const String movType = "Terra";
        return movType;
    }

    void Terra::_swapSavedState(void)
    {
        m_terrainCells[0].swap(m_terrainCells[1]);
        m_savedState.m_renderables.swap(mRenderables);
        std::swap(m_savedState.m_currentCell, m_currentCell);
        std::swap(m_savedState.m_camera, m_camera);
    }

    void Terra::levelTerrain(float height)
    {
        const Ogre::uint16 heightValue = this->heightToVal<float>(height);

        TextureGpuManager* textureManager = mManager->getDestinationRenderSystem()->getTextureGpuManager();
        StagingTexture* stagingTexture = textureManager->getStagingTexture(m_heightMapTex->getWidth(), m_heightMapTex->getHeight(), 1u, 1u, m_heightMapTex->getPixelFormat());
        stagingTexture->startMapRegion();

        TextureBox texBox = stagingTexture->mapRegion(m_heightMapTex->getWidth(), m_heightMapTex->getHeight(), 1u, 1u, m_heightMapTex->getPixelFormat());

        float fBpp = (float)(PixelFormatGpuUtils::getBytesPerPixel(m_heightMapTex->getPixelFormat()) << 3u);
        const float maxValue = powf(2.0f, fBpp) - 1.0f;
        m_invMaxValue = 1.0f / maxValue;

        if (m_heightMapTex->getPixelFormat() == PFG_R8_UNORM)
        {
            const uint8 targetVal = this->heightToVal<Ogre::uint8>(height);

            for (uint32 y = 0; y < m_depth; y++)
            {
                uint8* RESTRICT_ALIAS data = reinterpret_cast<uint8 * RESTRICT_ALIAS>(texBox.at(0, y, 0));
                for (uint32 x = 0; x < m_width; x++)
                {
                    m_heightMap[y * m_width + x] = heightValue;
                    data[x] = height;
                }
            }
        }
        else if (m_heightMapTex->getPixelFormat() == PFG_R16_UNORM)
        {
            for (uint32 y = 0; y < m_depth; y++)
            {
                uint16* RESTRICT_ALIAS data = reinterpret_cast<uint16 * RESTRICT_ALIAS>(texBox.at(0, y, 0));
                for (uint32 x = 0; x < m_width; x++)
                {
                    m_heightMap[y * m_width + x] = heightValue;
                    data[x] = height;
                }
            }
        }
        else if (m_heightMapTex->getPixelFormat() == PFG_R32_FLOAT)
        {
            for (uint32 y = 0; y < m_depth; y++)
            {
                float* RESTRICT_ALIAS data = reinterpret_cast<float* RESTRICT_ALIAS>(texBox.at(0, y, 0));
                for (uint32 x = 0; x < m_width; x++)
                {
                    m_heightMap[y * m_width + x] = height;
                    data[x] = heightValue;
                }
            }
        }

        stagingTexture->stopMapRegion();
        stagingTexture->upload(texBox, m_heightMapTex, 0, 0, 0);
        textureManager->removeStagingTexture(stagingTexture);
        stagingTexture = 0;

        // m_heightMapTex->notifyDataIsReady();

        updateHeightTextures();
    }

    void Terra::setBrushName(const Ogre::String& brushName)
    {
        TextureGpuManager* textureManager = mManager->getDestinationRenderSystem()->getTextureGpuManager();

        Ogre::Image2 brushImage;
        brushImage.load(brushName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

        brushImage.resize(m_brushSize, m_brushSize);

        size_t size = brushImage.getWidth() * brushImage.getHeight();
        if (0 == size)
            return;

        m_brushData.resize(size, 0);

        Ogre::ColourValue cval;
        for (uint32 y = 0; y < brushImage.getHeight(); y++)
        {
            // TextureBox texBox = brushImage.getData(0);
            // const float* RESTRICT_ALIAS imageData = reinterpret_cast<float* RESTRICT_ALIAS>(texBox.at(0, y, 0));
            for (uint32 x = 0; x < brushImage.getWidth(); x++)
            {
                cval = brushImage.getColourAt(x, y, 0);
                m_brushData[y * brushImage.getWidth() + x] = cval.r;
            }
        }
    }

    void Terra::setBrushSize(short brushSize)
    {
        m_brushSize = brushSize;
    }

    void Terra::modifyTerrainStart(const Ogre::Vector3& position, float strength)
    {
        m_oldHeightData.resize(m_width * m_depth, 0);
        m_newHeightData.resize(m_width * m_depth, 0);

        TextureGpuManager* textureManager = mManager->getDestinationRenderSystem()->getTextureGpuManager();
        // Important: Only create once, and after that work on that staging texture, else the heights get resetted every time
        if (nullptr == m_heightMapStagingTexture)
        {
            m_heightMapStagingTexture = textureManager->getStagingTexture(m_heightMapTex->getWidth(), m_heightMapTex->getHeight(), 1u, 1u, m_heightMapTex->getPixelFormat());
        }

        m_heightMapStagingTexture->startMapRegion();

        TextureBox texBox = m_heightMapStagingTexture->mapRegion(m_heightMapTex->getWidth(), m_heightMapTex->getHeight(), 1u, 1u, m_heightMapTex->getPixelFormat());

        memcpy(&m_oldHeightData[0], texBox.data, m_heightMapStagingTexture->_getSizeBytes());

        m_heightMapStagingTexture->stopMapRegion();

        this->applyHeightDiff(position, m_brushData, m_brushSize, strength);
    }

    void Terra::smoothTerrainStart(const Ogre::Vector3& position, float strength)
    {
        m_oldHeightData.resize(m_width * m_depth, 0);
        m_newHeightData.resize(m_width * m_depth, 0);

        TextureGpuManager* textureManager = mManager->getDestinationRenderSystem()->getTextureGpuManager();
        // Important: Only create once, and after that work on that staging texture, else the heights get resetted every time
        if (nullptr == m_heightMapStagingTexture)
        {
            m_heightMapStagingTexture = textureManager->getStagingTexture(m_heightMapTex->getWidth(), m_heightMapTex->getHeight(), 1u, 1u, m_heightMapTex->getPixelFormat());
        }

        m_heightMapStagingTexture->startMapRegion();

        TextureBox texBox = m_heightMapStagingTexture->mapRegion(m_heightMapTex->getWidth(), m_heightMapTex->getHeight(), 1u, 1u, m_heightMapTex->getPixelFormat());

        memcpy(&m_oldHeightData[0], texBox.data, m_heightMapStagingTexture->_getSizeBytes());

        m_heightMapStagingTexture->stopMapRegion();

        this->applySmoothDiff(position, m_brushData, m_brushSize, strength);
    }

    void Terra::modifyTerrain(const Ogre::Vector3& position, float strength)
    {
        // Apply the height modification
        this->applyHeightDiff(position, m_brushData, m_brushSize, strength);
    }

    void Terra::smoothTerrain(const Ogre::Vector3& position, float strength)
    {
        this->applySmoothDiff(position, m_brushData, m_brushSize, strength);
    }

    void Terra::applyHeightDiff(const Ogre::Vector3& position, const std::vector<float>& data, int boxSize, float strength)
    {
        if (data.size() != boxSize * boxSize)
            return;

        Ogre::GridPoint point = this->worldToGrid(position);
        //Offset the box so that the corner isn't the centre.
        point.x -= boxSize / 2;
        point.z -= boxSize / 2;

        int x = point.x;
        int y = point.z;

        TextureGpuManager* textureManager = mManager->getDestinationRenderSystem()->getTextureGpuManager();
        // Important: Only create once, and after that work on that staging texture, else the heights get resetted every time
        if (nullptr == m_heightMapStagingTexture)
        {
            m_heightMapStagingTexture = textureManager->getStagingTexture(m_heightMapTex->getWidth(), m_heightMapTex->getHeight(), 1u, 1u, m_heightMapTex->getPixelFormat());
        }
        if (m_heightMapTex->getNextResidencyStatus() != Ogre::GpuResidency::Resident)
        {
            m_heightMapTex->_transitionTo(Ogre::GpuResidency::Resident, (Ogre::uint8*)0);
            m_heightMapTex->_setNextResidencyStatus(Ogre::GpuResidency::Resident);
        }
        m_heightMapStagingTexture->startMapRegion();

        TextureBox texBox = m_heightMapStagingTexture->mapRegion(m_heightMapTex->getWidth(), m_heightMapTex->getHeight(), 1u, 1u, m_heightMapTex->getPixelFormat());

        float fBpp = (float)(PixelFormatGpuUtils::getBytesPerPixel(m_heightMapTex->getPixelFormat()) << 3u);
        const float maxValue = powf(2.0f, fBpp) - 1.0f;
        m_invMaxValue = 1.0f / maxValue;

        uint32 movAmount;
        uint32 shouldMoveAmount;
        int boxStartX, boxStartY, boxWidth, boxHeight;
        _calculateBoxSpecification(x, y, boxSize, m_heightMapTex->getWidth(), m_heightMapTex->getHeight(), &movAmount, &shouldMoveAmount, &boxStartX, &boxStartY, &boxWidth, &boxHeight);

        if (m_heightMapTex->getPixelFormat() == PFG_R16_UNORM)
        {

            for (int by = boxStartY; by < boxHeight; by++)
            {
                unsigned int posY = y + by;
                uint16* RESTRICT_ALIAS pixBoxData = reinterpret_cast<uint16 * RESTRICT_ALIAS>(texBox.at(0, posY, 0));

                for (int bx = boxStartX; bx < boxWidth; bx++)
                {
                    unsigned int posX = x + bx;

                    uint16 oldValue = pixBoxData[posX];

                    float determinedValue = oldValue + data[by * boxSize + bx] * strength;
                    if (determinedValue < 0.0f)
                    {
                        determinedValue = 0.0f;
                    }

                    //If value is 0 (we're lowering the terrain), the value should never be greater than what it previously was (possible with integer wrap-arounds).
                    if (strength < 0 && determinedValue > oldValue)
                    {
                        determinedValue = 0;
                    }
                    else if (strength > 0 && determinedValue < oldValue)
                    {
                        determinedValue = heightToVal<float>(m_height);
                    }

                    pixBoxData[posX] = determinedValue;
                    const float targetValue = this->valToHeight<float>(determinedValue);
                    m_heightMap[movAmount] = targetValue;

                    // Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "---->detV: " + Ogre::StringConverter::toString(determinedValue));

                    movAmount++;
                }
                movAmount += shouldMoveAmount; //Align to the start of the next line.
            }
        }

        m_heightMapStagingTexture->stopMapRegion();
        m_heightMapStagingTexture->upload(texBox, m_heightMapTex, 0, 0, 0);

        // is this necessary?
        updateHeightTextures();

        calculateOptimumSkirtSize();
    }

    void Terra::applySmoothDiff(const Ogre::Vector3& position, const std::vector<float>& data, int boxSize, float strength)
    {
        if (data.size() != boxSize * boxSize)
            return;

        strength = Ogre::Math::Abs(strength);

        Ogre::GridPoint point = this->worldToGrid(position);

        int x = point.x;
        int y = point.z;

        int targetStartX = x;
        int targetStartY = y;
        if (x < 0)
        {
            targetStartX = 0;
        }
        if (y < 0)
        {
            targetStartY = 0;
        }

        strength = strength / 100.0f;

        TextureGpuManager* textureManager = mManager->getDestinationRenderSystem()->getTextureGpuManager();
        // Important: Only create once, and after that work on that staging texture, else the heights get resetted every time
        if (nullptr == m_heightMapStagingTexture)
        {
            m_heightMapStagingTexture = textureManager->getStagingTexture(m_heightMapTex->getWidth(), m_heightMapTex->getHeight(), 1u, 1u, m_heightMapTex->getPixelFormat());
        }
        if (m_heightMapTex->getNextResidencyStatus() != Ogre::GpuResidency::Resident)
        {
            m_heightMapTex->_transitionTo(Ogre::GpuResidency::Resident, (Ogre::uint8*)0);
            m_heightMapTex->_setNextResidencyStatus(Ogre::GpuResidency::Resident);
        }
        m_heightMapStagingTexture->startMapRegion();

        TextureBox texBox = m_heightMapStagingTexture->mapRegion(m_heightMapTex->getWidth(), m_heightMapTex->getHeight(), 1u, 1u, m_heightMapTex->getPixelFormat());

        float fBpp = (float)(PixelFormatGpuUtils::getBytesPerPixel(m_heightMapTex->getPixelFormat()) << 3u);
        const float maxValue = powf(2.0f, fBpp) - 1.0f;
        m_invMaxValue = 1.0f / maxValue;

        uint32 movAmount;
        uint32 shouldMoveAmount;
        int boxStartX, boxStartY, boxWidth, boxHeight;
        _calculateBoxSpecification(x, y, boxSize, m_heightMapTex->getWidth(), m_heightMapTex->getHeight(), &movAmount, &shouldMoveAmount, &boxStartX, &boxStartY, &boxWidth, &boxHeight);

        if (m_heightMapTex->getPixelFormat() == PFG_R16_UNORM)
        {
            int xVal = targetStartX;
            int yVal = targetStartY;
            for (int by = boxStartY; by < boxHeight; by++)
            {
                unsigned int posY = y + by;
                uint16* RESTRICT_ALIAS pixBoxData = reinterpret_cast<uint16 * RESTRICT_ALIAS>(texBox.at(0, posY, 0));

                for (int bx = boxStartX; bx < boxWidth; bx++)
                {
                    unsigned int posX = x + bx;

                    const float prevCurrentHeight = m_heightMap[xVal + yVal * texBox.height];
                    float currentHeight = prevCurrentHeight;

                    float determinedValue = 0.0f;
                    uint8 totalAmount = 0;
                    //Find the average around this point.
                    for (int ya = yVal - 1; ya <= yVal + 1; ya++)
                    {
                        if (ya < 0 || ya >= texBox.height)
                        {
                            continue;
                        }

                        for (int xa = xVal - 1; xa <= xVal + 1; xa++)
                        {
                            if (xa < 0 || xa >= texBox.height)
                            {
                                continue;
                            }
                            determinedValue += m_heightMap[xa + ya * texBox.height];
                            totalAmount++;
                        }
                    }
                    if (totalAmount == 0)
                    {
                        currentHeight = prevCurrentHeight;
                    }
                    else
                    {
                        determinedValue /= totalAmount;
                        float diffVal = (determinedValue - currentHeight) * strength;
                        currentHeight += diffVal;
                    }

                    pixBoxData[posX] = this->heightToVal<float>(currentHeight);
                    m_heightMap[movAmount] = currentHeight;

                    movAmount++;
                    xVal++;
                }
                // Align to the start of the next line.
                movAmount += shouldMoveAmount;
                yVal++;
                xVal = targetStartX;
            }
        }

        m_heightMapStagingTexture->stopMapRegion();
        m_heightMapStagingTexture->upload(texBox, m_heightMapTex, 0, 0, 0);

        // is this necessary?
        updateHeightTextures();

        calculateOptimumSkirtSize();
    }

    //-----------------------------------------------------------------------------------
    // applyHeightBlend - Conform-to-footprint primitive (roads, building pads, etc.)
    //
    // Unlike applyHeightDiff (which ADDS a delta from a brush image - interactive
    // sculpting) this BLENDS terrain height toward an explicit absolute target height,
    // with a circular flat "footprint" radius and a cosine-falloff blend margin around
    // it - exactly the technique callers like ProceduralRoadComponent's terrain-conform
    // step need: flatten the terrain to the road's own already-correct height under the
    // road, then smoothly taper back to the untouched terrain over blendMarginWorld.
    //
    // - position: world-space stamp centre
    // - targetHeight: world-space height to snap to inside 'radiusWorld'
    // - radiusWorld: flat footprint radius in world units (e.g. half the road width
    //   including edges)
    // - blendMarginWorld: additional distance beyond radiusWorld over which the target
    //   height fades back out to whatever the terrain already had (0 = hard edge)
    //
    // Call once per sample point along a path (road centreline, building edge, etc.);
    // overlapping stamps blend naturally since each one only ever pulls height TOWARD
    // its own target by its own influence, never overwrites unconditionally.
    //
    // Must be called from the render thread, same as applyHeightDiff/setHeightData.
    //-----------------------------------------------------------------------------------
    void Terra::applyHeightBlend(const Ogre::Vector3& position, float targetHeight, float radiusWorld, float blendMarginWorld)
    {
        if (radiusWorld <= 0.0f)
        {
            return;
        }

        // m_heightMap (and valToHeight/heightToVal) work in RELATIVE height space
        // (0..m_height) - the absolute world Y is only ever formed by callers adding
        // m_terrainOrigin.y on top (see getHeightAt: "... + m_terrainOrigin.y"). The
        // caller of this function passes 'targetHeight' as an absolute world Y (the
        // natural unit for a road/building height), so convert it to the same
        // relative space m_heightMap already uses before blending.
        const float targetHeightRelative = targetHeight - m_terrainOrigin.y;

        const float pixelsPerMeterX = (m_xzRelativeSize.x > 1e-6f) ? (1.0f / m_xzRelativeSize.x) : 1.0f;
        const float pixelsPerMeterZ = (m_xzRelativeSize.y > 1e-6f) ? (1.0f / m_xzRelativeSize.y) : 1.0f;
        const float pixelsPerMeter = 0.5f * (pixelsPerMeterX + pixelsPerMeterZ);

        const float radiusPixels = radiusWorld * pixelsPerMeter;
        const float blendPixels = std::max(0.0f, blendMarginWorld) * pixelsPerMeter;
        const float outerRadiusPixels = radiusPixels + blendPixels;

        const int boxHalf = static_cast<int>(std::ceil(outerRadiusPixels)) + 1;
        const int boxSize = boxHalf * 2 + 1;

        Ogre::GridPoint point = this->worldToGrid(position);

        // Offset the box so that the corner isn't the centre (same convention as applyHeightDiff).
        point.x -= boxSize / 2;
        point.z -= boxSize / 2;

        int x = point.x;
        int y = point.z;

        TextureGpuManager* textureManager = mManager->getDestinationRenderSystem()->getTextureGpuManager();
        // Important: Only create once, and after that work on that staging texture, else the heights get resetted every time
        if (nullptr == m_heightMapStagingTexture)
        {
            m_heightMapStagingTexture = textureManager->getStagingTexture(m_heightMapTex->getWidth(), m_heightMapTex->getHeight(), 1u, 1u, m_heightMapTex->getPixelFormat());
        }
        if (m_heightMapTex->getNextResidencyStatus() != Ogre::GpuResidency::Resident)
        {
            m_heightMapTex->_transitionTo(Ogre::GpuResidency::Resident, (Ogre::uint8*)0);
            m_heightMapTex->_setNextResidencyStatus(Ogre::GpuResidency::Resident);
        }
        m_heightMapStagingTexture->startMapRegion();

        TextureBox texBox = m_heightMapStagingTexture->mapRegion(m_heightMapTex->getWidth(), m_heightMapTex->getHeight(), 1u, 1u, m_heightMapTex->getPixelFormat());

        uint32 movAmount;
        uint32 shouldMoveAmount;
        int boxStartX, boxStartY, boxWidth, boxHeight;
        _calculateBoxSpecification(x, y, boxSize, m_heightMapTex->getWidth(), m_heightMapTex->getHeight(), &movAmount, &shouldMoveAmount, &boxStartX, &boxStartY, &boxWidth, &boxHeight);

        // Local box-space coordinates of the stamp centre, matching the point.x/point.z
        // offset applied above (boxSize/2, same integer division truncation).
        const int centreBx = boxSize / 2;
        const int centreBy = boxSize / 2;

        if (m_heightMapTex->getPixelFormat() == PFG_R16_UNORM)
        {
            for (int by = boxStartY; by < boxHeight; by++)
            {
                unsigned int posY = y + by;
                uint16* RESTRICT_ALIAS pixBoxData = reinterpret_cast<uint16 * RESTRICT_ALIAS>(texBox.at(0, posY, 0));

                for (int bx = boxStartX; bx < boxWidth; bx++)
                {
                    unsigned int posX = x + bx;

                    const float dx = static_cast<float>(bx - centreBx);
                    const float dy = static_cast<float>(by - centreBy);
                    const float dist = std::sqrt(dx * dx + dy * dy);

                    if (dist > outerRadiusPixels)
                    {
                        movAmount++;
                        continue;
                    }

                    // Cosine falloff: 1.0 inside the flat footprint radius, tapering to
                    // 0.0 at the outer edge of the blend margin - same shape as the
                    // offline road-stamp technique in ProceduralTerrainCreationComponent,
                    // just applied directly to the live Terra height data.
                    float influence;
                    if (dist <= radiusPixels)
                    {
                        influence = 1.0f;
                    }
                    else
                    {
                        const float t = (dist - radiusPixels) / std::max(0.001f, blendPixels);
                        influence = 0.5f + 0.5f * std::cos(t * Ogre::Math::PI);
                    }

                    const uint16 oldValue = pixBoxData[posX];
                    const float oldHeight = this->valToHeight<float>(static_cast<float>(oldValue));

                    // One-way carve: never RAISE terrain to meet the road, only ever
                    // LOWER it if it's currently higher than the road's target height
                    // (i.e. a hill/ridge is in the way). If the terrain is already at
                    // or below the target (a dip the road already floats above cleanly),
                    // leave it untouched. This mirrors the existing brush's own
                    // negative-strength-only convention for lowering ground, and
                    // structurally cannot ever push terrain above the road, unlike a
                    // symmetric blend toward an absolute target in both directions.
                    const float ceilingHeight = std::min(oldHeight, targetHeightRelative);
                    const float newHeight = oldHeight + (ceilingHeight - oldHeight) * influence;

                    float newValue = this->heightToVal<float>(newHeight);
                    newValue = Ogre::Math::Clamp(newValue, 0.0f, 65535.0f);

                    pixBoxData[posX] = static_cast<uint16>(newValue);
                    m_heightMap[movAmount] = newHeight;

                    movAmount++;
                }
                movAmount += shouldMoveAmount; //Align to the start of the next line.
            }
        }

        m_heightMapStagingTexture->stopMapRegion();
        m_heightMapStagingTexture->upload(texBox, m_heightMapTex, 0, 0, 0);

        updateHeightTextures();

        calculateOptimumSkirtSize();
    }

    std::pair<std::vector<Ogre::uint16>, std::vector<Ogre::uint16>> Terra::modifyTerrainFinished(void)
    {
        TextureGpuManager* textureManager = mManager->getDestinationRenderSystem()->getTextureGpuManager();
        // Important: Only create once, and after that work on that staging texture, else the heights get resetted every time
        if (nullptr == m_heightMapStagingTexture)
        {
            m_heightMapStagingTexture = textureManager->getStagingTexture(m_heightMapTex->getWidth(), m_heightMapTex->getHeight(), 1u, 1u, m_heightMapTex->getPixelFormat());
        }

        m_heightMapStagingTexture->startMapRegion();

        TextureBox texBox = m_heightMapStagingTexture->mapRegion(m_heightMapTex->getWidth(), m_heightMapTex->getHeight(), 1u, 1u, m_heightMapTex->getPixelFormat());

        memcpy(&m_newHeightData[0], texBox.data, m_heightMapStagingTexture->_getSizeBytes());

        m_heightMapStagingTexture->stopMapRegion();

        return std::move(std::make_pair(m_oldHeightData, m_newHeightData));
    }

    std::pair<std::vector<Ogre::uint16>, std::vector<Ogre::uint16>> Terra::smoothTerrainFinished(void)
    {
        TextureGpuManager* textureManager = mManager->getDestinationRenderSystem()->getTextureGpuManager();
        // Important: Only create once, and after that work on that staging texture, else the heights get resetted every time
        if (nullptr == m_heightMapStagingTexture)
        {
            m_heightMapStagingTexture = textureManager->getStagingTexture(m_heightMapTex->getWidth(), m_heightMapTex->getHeight(), 1u, 1u, m_heightMapTex->getPixelFormat());
        }

        m_heightMapStagingTexture->startMapRegion();

        TextureBox texBox = m_heightMapStagingTexture->mapRegion(m_heightMapTex->getWidth(), m_heightMapTex->getHeight(), 1u, 1u, m_heightMapTex->getPixelFormat());

        memcpy(&m_newHeightData[0], texBox.data, m_heightMapStagingTexture->_getSizeBytes());

        m_heightMapStagingTexture->stopMapRegion();

        return std::move(std::make_pair(m_oldHeightData, m_newHeightData));
    }

    void Terra::setHeightData(const std::vector<Ogre::uint16>& heightData)
    {
        TextureGpuManager* textureManager = mManager->getDestinationRenderSystem()->getTextureGpuManager();
        // Important: Only create once, and after that work on that staging texture, else the heights get resetted every time
        if (nullptr == m_heightMapStagingTexture)
        {
            m_heightMapStagingTexture = textureManager->getStagingTexture(m_heightMapTex->getWidth(), m_heightMapTex->getHeight(), 1u, 1u, m_heightMapTex->getPixelFormat());
        }
        if (m_heightMapTex->getNextResidencyStatus() != Ogre::GpuResidency::Resident)
        {
            m_heightMapTex->_transitionTo(Ogre::GpuResidency::Resident, (Ogre::uint8*)0);
            m_heightMapTex->_setNextResidencyStatus(Ogre::GpuResidency::Resident);
        }
        m_heightMapStagingTexture->startMapRegion();

        TextureBox texBox = m_heightMapStagingTexture->mapRegion(m_heightMapTex->getWidth(), m_heightMapTex->getHeight(), 1u, 1u, m_heightMapTex->getPixelFormat());

        if (m_heightMapTex->getPixelFormat() == PFG_R16_UNORM)
        {
            for (int y = 0; y < m_depth; y++)
            {
                uint16* RESTRICT_ALIAS pixBoxData = reinterpret_cast<uint16 * RESTRICT_ALIAS>(texBox.at(0, y, 0));

                for (int x = 0; x < m_width; x++)
                {
                    int value = heightData[y * m_width + x];

                    pixBoxData[x] = value;
                    const float targetVal = this->valToHeight<float>(value);
                    m_heightMap[y * m_width + x] = targetVal;
                }
            }
        }

        m_heightMapStagingTexture->stopMapRegion();
        m_heightMapStagingTexture->upload(texBox, m_heightMapTex, 0, 0, 0);

        // is this necessary?
        updateHeightTextures();

        calculateOptimumSkirtSize();
    }

    void Terra::paintTerrainStart(const Ogre::Vector3& position, float intensity, int blendLayer)
    {
        m_oldBlendWeightData.resize(m_blendWeightTex->getWidth() * m_blendWeightTex->getHeight() * 4, 0);
        m_newBlendWeightData.resize(m_blendWeightTex->getWidth() * m_blendWeightTex->getHeight() * 4, 0);

        TextureGpuManager* textureManager = mManager->getDestinationRenderSystem()->getTextureGpuManager();

        // Important: Only create once, and after that work on that staging texture, else the heights get resetted every time
        if (nullptr == m_blendWeightStagingTexture)
        {
            m_blendWeightStagingTexture = textureManager->getStagingTexture(m_blendWeightTex->getWidth(), m_blendWeightTex->getHeight(), 1u, 1u, m_blendWeightTex->getPixelFormat());
        }

        m_blendWeightStagingTexture->startMapRegion();

        TextureBox detailMapTexBox = m_blendWeightStagingTexture->mapRegion(m_blendWeightTex->getWidth(), m_blendWeightTex->getHeight(), 1u, 1u, m_blendWeightTex->getPixelFormat());

        memcpy(&m_oldBlendWeightData[0], detailMapTexBox.data, m_blendWeightStagingTexture->_getSizeBytes());

        m_blendWeightStagingTexture->stopMapRegion();

        this->applyBlendDiff(position, m_brushData, m_brushSize, intensity, blendLayer);
    }

    void Terra::applyBlendDiff(const Ogre::Vector3& position, const std::vector<float>& brushData, int boxSize, float intensity, int blendLayer)
    {
        if (brushData.size() != boxSize * boxSize || nullptr == m_blendWeightTex || brushData.empty())
            return;

        // assert(data.size() == boxSize * boxSize);

        Ogre::GridPoint point = this->worldToGrid(position);
        //Offset the box so that the corner isn't the centre.
        point.x -= boxSize / 2;
        point.z -= boxSize / 2;

        int x = point.x;
        int y = point.z;

        TextureGpuManager* textureManager = mManager->getDestinationRenderSystem()->getTextureGpuManager();

        // Important: Only create once, and after that work on that staging texture, else the heights get resetted every time
        if (nullptr == m_blendWeightStagingTexture)
        {
            m_blendWeightStagingTexture = textureManager->getStagingTexture(m_blendWeightTex->getWidth(), m_blendWeightTex->getHeight(), 1u, 1u, m_blendWeightTex->getPixelFormat());
        }
        // Attention: When blendWeightTex is created in DatablockTerraComponent, texture->scheduleTransitionTo(Ogre::GpuResidency::OnSystemRam); is set! This maps the actual texture!
        if (m_blendWeightTex->getResidencyStatus() != Ogre::GpuResidency::Resident)
        {
            m_blendWeightTex->_transitionTo(Ogre::GpuResidency::Resident, (Ogre::uint8*)0);
            m_blendWeightTex->_setNextResidencyStatus(Ogre::GpuResidency::Resident);
        }
        m_blendWeightStagingTexture->startMapRegion();

        TextureBox detailMapTexBox = m_blendWeightStagingTexture->mapRegion(m_blendWeightTex->getWidth(), m_blendWeightTex->getHeight(), 1u, 1u, m_blendWeightTex->getPixelFormat());

        uint32 movAmount, shouldMoveAmount;
        int boxStartX, boxStartY, boxWidth, boxHeight;
        _calculateBoxSpecification(x, y, boxSize, m_blendWeightTex->getWidth(), m_blendWeightTex->getHeight(), &movAmount, &shouldMoveAmount, &boxStartX, &boxStartY, &boxWidth, &boxHeight);

        Ogre::Vector4 vals(-1.0f, -1.0f, -1.0f, -1.0f);
        if (blendLayer == 0)
            vals.x = -vals.x;
        else if (blendLayer == 1)
            vals.y = -vals.y;
        else if (blendLayer == 2)
            vals.z = -vals.z;
        else if (blendLayer == 3)
            vals.w = -vals.w;

        vals *= intensity;

        const size_t bytesPerPixel = detailMapTexBox.bytesPerPixel;

        for (int by = boxStartY; by < boxHeight; by++)
        {
            unsigned int posY = y + by;
            uint8* RESTRICT_ALIAS detailMapPixBoxData = reinterpret_cast<uint8 * RESTRICT_ALIAS>(detailMapTexBox.at(0, posY, 0));

            for (int bx = boxStartX; bx < boxWidth; bx++)
            {
                // bytesPerPixel 4 for rgba array
                unsigned int posX = (x + bx) * bytesPerPixel;

                float rgba[4] = { detailMapPixBoxData[posX + 0], detailMapPixBoxData[posX + 1], detailMapPixBoxData[posX + 2], detailMapPixBoxData[posX + 3] };

                float brushVal = brushData[bx + by * boxSize];

                // Note: / 255 must be done, because packColour expects values of min 0, max 1 and forms it from 0 to 255
                rgba[0] = this->alterBlend(rgba[0], vals.x * brushVal) / 255.0f;
                rgba[1] = this->alterBlend(rgba[1], vals.y * brushVal) / 255.0f;
                rgba[2] = this->alterBlend(rgba[2], vals.z * brushVal) / 255.0f;
                rgba[3] = this->alterBlend(rgba[3], vals.w * brushVal) / 255.0f;

                PixelFormatGpuUtils::packColour(rgba, PFG_RGBA8_UNORM, &detailMapPixBoxData[posX]);

                movAmount += 4;
            }
            movAmount += shouldMoveAmount * 4; //Align to the start of the next line.
            detailMapPixBoxData += shouldMoveAmount * 4;
        }

        m_blendWeightStagingTexture->stopMapRegion();
        m_blendWeightStagingTexture->upload(detailMapTexBox, m_blendWeightTex, 0);
    }

    uint8 Terra::alterBlend(uint8 oldValue, float diff)
    {
        uint8 retVal = oldValue;

        retVal += diff;

        if (diff < 0 && retVal > oldValue)
        { //If the value has rolled over.
            retVal = 0;
        }
        if (diff > 0 && retVal < oldValue)
        {
            retVal = 255;
        }

        return retVal;
    }

    void Terra::paintTerrain(const Ogre::Vector3& position, float intensity, int blendLayer)
    {
        this->applyBlendDiff(position, m_brushData, m_brushSize, intensity, blendLayer);
    }

    std::pair<std::vector<Ogre::uint8>, std::vector<Ogre::uint8>> Terra::paintTerrainFinished(void)
    {
        TextureGpuManager* textureManager = mManager->getDestinationRenderSystem()->getTextureGpuManager();

        // Important: Only create once, and after that work on that staging texture, else the heights get resetted every time
        if (nullptr == m_blendWeightStagingTexture)
        {
            m_blendWeightStagingTexture = textureManager->getStagingTexture(m_blendWeightTex->getWidth(), m_blendWeightTex->getHeight(), 1u, 1u, m_blendWeightTex->getPixelFormat());
        }

        m_blendWeightStagingTexture->startMapRegion();

        TextureBox detailMapTexBox = m_blendWeightStagingTexture->mapRegion(m_blendWeightTex->getWidth(), m_blendWeightTex->getHeight(), 1u, 1u, m_blendWeightTex->getPixelFormat());

        memcpy(&m_newBlendWeightData[0], detailMapTexBox.data, m_blendWeightStagingTexture->_getSizeBytes());

        m_blendWeightStagingTexture->stopMapRegion();

        return std::move(std::make_pair(m_oldBlendWeightData, m_newBlendWeightData));
    }

    void Terra::setBlendWeightData(const std::vector<Ogre::uint8>& blendWeightData)
    {
        TextureGpuManager* textureManager = mManager->getDestinationRenderSystem()->getTextureGpuManager();

        // Important: Only create once, and after that work on that staging texture, else the heights get resetted every time
        if (nullptr == m_blendWeightStagingTexture)
        {
            m_blendWeightStagingTexture = textureManager->getStagingTexture(m_blendWeightTex->getWidth(), m_blendWeightTex->getHeight(), 1u, 1u, m_blendWeightTex->getPixelFormat());
        }
        // Attention: When blendWeightTex is created in DatablockTerraComponent, texture->scheduleTransitionTo(Ogre::GpuResidency::OnSystemRam); is set! This maps the actual texture!
        if (m_blendWeightTex->getResidencyStatus() != Ogre::GpuResidency::Resident)
        {
            m_blendWeightTex->_transitionTo(Ogre::GpuResidency::Resident, (Ogre::uint8*)0);
            m_blendWeightTex->_setNextResidencyStatus(Ogre::GpuResidency::Resident);
        }
        m_blendWeightStagingTexture->startMapRegion();

        TextureBox detailMapTexBox = m_blendWeightStagingTexture->mapRegion(m_blendWeightTex->getWidth(), m_blendWeightTex->getHeight(), 1u, 1u, m_blendWeightTex->getPixelFormat());

        memcpy(detailMapTexBox.data, &blendWeightData[0], m_blendWeightStagingTexture->_getSizeBytes());

        m_blendWeightStagingTexture->stopMapRegion();
        m_blendWeightStagingTexture->upload(detailMapTexBox, m_blendWeightTex, 0, 0, 0);
    }

    std::tuple<bool, Vector3, Vector3, Real> Terra::checkRayIntersect(const Ogre::Ray& ray)
    {
        typedef std::tuple<bool, Vector3, Vector3, Real> Result;

        assert(m_width == m_depth); //Just to make sure the terrain has consistant vertex resolution.

        Vector3 rayOrigin = ray.getOrigin() - m_terrainOrigin;

        Vector3 rayDirection = ray.getDirection();
        const float scale = m_xzDimensions.x / (Real)(m_width);

        rayOrigin.x /= scale;
        rayOrigin.z /= scale;
        rayDirection.x /= scale;
        rayDirection.z /= scale;
        rayDirection.normalise();

        Ray localRay(rayOrigin, rayDirection);

        Real maxHeight = m_height;
        Real minHeight = 0;

        AxisAlignedBox aabb(Vector3(0, minHeight, 0), Vector3(m_width, maxHeight, m_width));
        std::pair<bool, Real> aabbTest = localRay.intersects(aabb);

        if (!aabbTest.first)
        {
            return Result(false, Vector3(), Vector3(), 10000.0f);
        }

        Vector3 cur = localRay.getPoint(aabbTest.second);

        int quadX = std::min(std::max(static_cast<int>(cur.x), 0), (int)m_width - 2);
        int quadZ = std::min(std::max(static_cast<int>(cur.z), 0), (int)m_width - 2);
        int flipX = (rayDirection.x < 0 ? 0 : 1);
        int flipZ = (rayDirection.z < 0 ? 0 : 1);
        int xDir = (rayDirection.x < 0 ? -1 : 1);
        int zDir = (rayDirection.z < 0 ? -1 : 1);

        Result result(true, Vector3::ZERO, Vector3::ZERO, 10000.0f);
        Real dummyHighValue = (Real)m_width * 10000.0f;

        while (cur.y >= (minHeight - 1e-3) && cur.y <= (maxHeight + 1e-3))
        {
            if (quadX < 0 || quadX >= (int)m_width - 1 || quadZ < 0 || quadZ >= (int)m_depth - 1)
            {
                break;
            }

            result = checkQuadIntersection(quadX, quadZ, localRay);

            // Terra may have its origin in negative, hence subtract to become positive

            // Removed: Position is already corrected below
            /*if (m_terrainOrigin.y < 0.0f)
            {
                std::get<3>(result) -= m_terrainOrigin.y;
            }*/

            if (std::get<0>(result))
                break;

            // determine next quad to test
            Real xDist = Math::RealEqual(rayDirection.x, 0.0) ? dummyHighValue :
                (quadX - cur.x + flipX) / rayDirection.x;
            Real zDist = Math::RealEqual(rayDirection.z, 0.0) ? dummyHighValue :
                (quadZ - cur.z + flipZ) / rayDirection.z;
            if (xDist < zDist)
            {
                quadX += xDir;
                cur += rayDirection * xDist;
            }
            else
            {
                quadZ += zDir;
                cur += rayDirection * zDist;
            }
        }

        if (std::get<0>(result))
        {
            std::get<1>(result).x *= scale;
            std::get<1>(result).z *= scale;
            std::get<1>(result) += m_terrainOrigin;
        }

        return result;
    }

    Real Terra::getHeightData(int x, int y)
    {
        assert(x >= 0 && x < m_width && y >= 0 && y < m_depth);
        return m_heightMap[y * m_width + x];
    }

    std::tuple<bool, Vector3, Vector3, Real> Terra::checkQuadIntersection(int x, int z, const Ray& ray)
    {
        Vector3 v1((Real)x, getHeightData(x, z), (Real)z);
        Vector3 v2((Real)x + 1, getHeightData(x + 1, z), (Real)z);
        Vector3 v3((Real)x, getHeightData(x, z + 1), (Real)z + 1);
        Vector3 v4((Real)x + 1, getHeightData(x + 1, z + 1), (Real)z + 1);

        Vector4 p1, p2;
        bool oddRow = false;
        if (z % 2)
        {
            /*  3---4
                | \ |
                1---2*/
            p1 = Math::calculateFaceNormalWithoutNormalize(v2, v4, v3);
            p2 = Math::calculateFaceNormalWithoutNormalize(v1, v2, v3);
            oddRow = true;
        }
        else
        {
            /*  3---4
                | / |
                1---2*/
            p1 = Math::calculateFaceNormalWithoutNormalize(v1, v2, v4);
            p2 = Math::calculateFaceNormalWithoutNormalize(v1, v4, v3);
        }

        // Test for intersection with the two planes.
        // Then test that the intersection points are actually
        // still inside the triangle (with a small error margin)
        // Also check which triangle it is in
        std::pair<bool, Real> planeInt = ray.intersects(Plane(p1.x, p1.y, p1.z, p1.w));

        if (planeInt.first)
        {
            Vector3 where = ray.getPoint(planeInt.second);
            Vector3 rel = where - v1;
            if (rel.x >= -0.01 && rel.x <= 1.01 && rel.z >= -0.01 && rel.z <= 1.01 // quad bounds
                && ((rel.x >= rel.z && !oddRow) || (rel.x >= (1 - rel.z) && oddRow))) // triangle bounds
                return std::tuple<bool, Vector3, Vector3, Real>(true, where, Vector3(p1.x, p1.y, p1.z), planeInt.second);
        }
        planeInt = ray.intersects(Plane(p2.x, p2.y, p2.z, p2.w));
        if (planeInt.first)
        {
            Vector3 where = ray.getPoint(planeInt.second);
            Vector3 rel = where - v1;
            if (rel.x >= -0.01 && rel.x <= 1.01 && rel.z >= -0.01 && rel.z <= 1.01 // quad bounds
                && ((rel.x <= rel.z && !oddRow) || (rel.x <= (1 - rel.z) && oddRow))) // triangle bounds
                return std::tuple<bool, Vector3, Vector3, Real>(true, where, Vector3(p2.x, p2.y, p2.z), planeInt.second);
        }
        //Essentually, it checks the planes first as sort of like a broad phase, then it actually checks the individual triangles.
        return std::tuple<bool, Vector3, Vector3, Real>(false, Vector3(), Vector3(), 10000.0f);
    }

    void Terra::updateHeightTextures()
    {
        mNormalMapperWorkspace->_beginUpdate(true);
        mNormalMapperWorkspace->_update();
        mNormalMapperWorkspace->_endUpdate(true);
    }

    void Terra::saveTextures(const Ogre::String& path, const Ogre::String& prefix)
    {
        if (nullptr != m_heightMapTex)
        {
            m_currentHeightMapImageName = prefix + "_heightMap.png";
            m_heightMapTex->writeContentsToFile(path + "/" + m_currentHeightMapImageName, 0, m_heightMapTex->getNumMipmaps());
        }
        if (nullptr != m_blendWeightTex)
        {
            m_currentBlendWeightImageName = prefix + "_detailMap.png";
            m_blendWeightTex->writeContentsToFile(path + "/" + m_currentBlendWeightImageName, 0, m_blendWeightTex->getNumMipmaps());
        }

        /*Ogre::Image2 heightImg;


        heightImg.convertFromTexture(m_heightMapTex, 1, 8);
        heightImg.save(path + "/height.png", 1, 1);

        Ogre::Image2 blendImg;
        blendImg.convertFromTexture(m_blendWeightTex, 1, 8);
        blendImg.save(path + "/detailMap.png", 1, 1);*/
    }

    void Terra::_calculateBoxSpecification(int x, int y, int boxSize, uint32 pitch, uint32 depth, uint32* movAmount, uint32* shouldMoveAmount, int* boxStartX, int* boxStartY, int* boxWidth, int* boxHeight)
    {
        int drawX = x;
        int drawY = y;

        *boxWidth = boxSize;
        *boxHeight = boxSize;
        *boxStartX = 0;
        *boxStartY = 0;

        if (x < 0)
        {
            drawX = 0;
            *boxStartX = -x;
        }
        if (y < 0)
        {
            drawY = 0;
            *boxStartY = -y;
        }
        if (x + boxSize > pitch)
        {
            int amount = (x + boxSize) - pitch;
            *boxWidth = boxSize - amount;
        }
        if (y + boxSize > depth)
        {
            int amount = (y + boxSize) - depth;
            *boxHeight = boxSize - amount;
        }

        *movAmount = (drawY * pitch) + drawX;
        *shouldMoveAmount = (pitch - (x + *boxWidth)) + drawX;
    }

    Ogre::String Terra::getHeightMapTextureName(void) const
    {
        return this->m_currentHeightMapImageName;
    }

    Ogre::String Terra::getBlendWeightTextureName(void) const
    {
        return this->m_currentBlendWeightImageName;
    }

    void Terra::setLodRing0WorldSize(float worldSize)
    {
        m_lodRing0WorldSize = std::max(0.1f, worldSize);
    }

    float Terra::getLodRing0WorldSize() const
    {
        return m_lodRing0WorldSize;
    }

    //-----------------------------------------------------------------------------------
    void Terra::setMaxLodDistance(float maxLodDistance)
    {
        m_maxLodDistance = std::max(1.0f, maxLodDistance);
    }
    //-----------------------------------------------------------------------------------
    float Terra::getMaxLodDistance() const
    {
        return m_maxLodDistance;
    }
    //-----------------------------------------------------------------------------------
    void Terra::setLodFarVertexPercent(float percent)
    {
        m_lodFarVertexPercent = Math::Clamp(percent, 0.01f, 1.0f);
    }
    //-----------------------------------------------------------------------------------
    float Terra::getLodFarVertexPercent() const
    {
        return m_lodFarVertexPercent;
    }
    //-----------------------------------------------------------------------------------
    void Terra::setLodCurvePower(float power)
    {
        m_lodCurvePower = std::max(0.01f, power);
    }
    //-----------------------------------------------------------------------------------
    float Terra::getLodCurvePower() const
    {
        return m_lodCurvePower;
    }

    void Terra::setPrefix(const Ogre::String& prefix)
    {
        m_prefix = prefix;
    }

    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    TerraSharedResources::TerraSharedResources()
    {
        memset(textures, 0, sizeof(textures));
    }
    //-----------------------------------------------------------------------------------
    TerraSharedResources::~TerraSharedResources()
    {
        freeAllMemory();
    }
    //-----------------------------------------------------------------------------------
    void TerraSharedResources::freeAllMemory()
    {
        freeStaticMemory();

        for (size_t i = NumStaticTmpTextures; i < NumTemporaryUsages; ++i)
        {
            if (textures[i])
            {
                TextureGpuManager* textureManager = textures[i]->getTextureManager();
                textureManager->destroyTexture(textures[i]);
                textures[i] = 0;
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void TerraSharedResources::freeStaticMemory()
    {
        for (size_t i = 0u; i < NumStaticTmpTextures; ++i)
        {
            if (textures[i])
            {
                TextureGpuManager* textureManager = textures[i]->getTextureManager();
                textureManager->destroyTexture(textures[i]);
                textures[i] = 0;
            }
        }
    }
    //-----------------------------------------------------------------------------------
    TextureGpu* TerraSharedResources::getTempTexture(const char* texName, IdType id,
        TerraSharedResources* sharedResources,
        TemporaryUsages temporaryUsage,
        TextureGpu* baseTemplate, uint32 flags)
    {
        TextureGpu* tmpRtt = 0;

        if (sharedResources && sharedResources->textures[temporaryUsage])
            tmpRtt = sharedResources->textures[temporaryUsage];
        else
        {
            TextureGpuManager* textureManager = baseTemplate->getTextureManager();
            tmpRtt = textureManager->createTexture(texName + StringConverter::toString(id),
                GpuPageOutStrategy::Discard, flags,
                TextureTypes::Type2D);
            tmpRtt->copyParametersFrom(baseTemplate);
            tmpRtt->scheduleTransitionTo(GpuResidency::Resident);
            if (flags & TextureFlags::RenderToTexture)
                tmpRtt->_setDepthBufferDefaults(DepthBuffer::POOL_NO_DEPTH, false, PFG_UNKNOWN);

            if (sharedResources)
                sharedResources->textures[temporaryUsage] = tmpRtt;
        }
        return tmpRtt;
    }
    //-----------------------------------------------------------------------------------
    void TerraSharedResources::destroyTempTexture(TerraSharedResources* sharedResources,
        TextureGpu* tmpRtt)
    {
        if (!sharedResources)
        {
            TextureGpuManager* textureManager = tmpRtt->getTextureManager();
            textureManager->destroyTexture(tmpRtt);
        }
    }
}
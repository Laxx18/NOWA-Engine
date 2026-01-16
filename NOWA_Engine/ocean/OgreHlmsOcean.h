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
#ifndef _OgreHlmsOcean_H_
#define _OgreHlmsOcean_H_

#include "OgreHlmsOceanPrerequisites.h" 
#include "OgreHlmsBufferManager.h"
#include "OgreConstBufferPool.h"
#include "OgreMatrix4.h"
#include "OgreHlmsPbs.h"
#include "OgreTextureGpu.h"
#include "OgreRootLayout.h"

namespace Ogre
{
    class CompositorShadowNode;
    class Ocean;

    struct QueuedRenderable;

    /** \addtogroup Component
    *  @{
    */
    /** \addtogroup Material
    *  @{
    */

    class HlmsOceanDatablock;

    /** Physically based shading implementation specfically designed for
        OpenGL 3+, D3D11 and other RenderSystems which support uniform buffers.
    */
    class HlmsOcean : public HlmsPbs
    {
    public:
        enum ShadowFilter
        {
            /// Standard quality. Very fast.
            PCF_2x2,

            /// Good quality. Still quite fast on most modern hardware.
            PCF_3x3,

            /// High quality. Very slow in old hardware (i.e. DX10 level hw and below)
            /// Use RSC_TEXTURE_GATHER to check whether it will be slow or not.
            PCF_4x4,

            NumShadowFilter
        };

        enum AmbientLightMode
        {
            /// Use fixed-colour ambient lighting when upper hemisphere = lower hemisphere,
            /// use hemisphere lighting when they don't match.
            /// Disables ambient lighting if the colors are black.
            AmbientAuto,

            /// Force fixed-colour ambient light. Only uses the upper hemisphere paramter.
            AmbientFixed,

            /// Force hemisphere ambient light. Useful if you plan on adjusting the colors
            /// dynamically very often and this might cause swapping shaders.
            AmbientHemisphere,

            /// Disable ambient lighting.
            AmbientNone
        };

    protected:
        typedef vector<ConstBufferPacked*>::type ConstBufferPackedVec;
        typedef vector<HlmsDatablock*>::type HlmsDatablockVec;

        struct PassData
        {
            /// Shadow textures are TextureGpu in Ogre-Next
            FastArray<TextureGpu *> shadowMaps;
            FastArray<float>    vertexShaderSharedBuffer;
            FastArray<float>    pixelShaderSharedBuffer;

            Matrix4 viewMatrix;
        };

        HlmsSamplerblock const  *mShadowmapSamplerblock;    /// GL3+ only when not using depth textures
        HlmsSamplerblock const  *mShadowmapCmpSamplerblock; /// For depth textures & D3D11
        HlmsSamplerblock const  *mCurrentShadowmapSamplerblock;
        HlmsSamplerblock const  *mOceanSamplerblock;
		HlmsSamplerblock const  *mOceanDataSamplerblock;
        HlmsSamplerblock const  *mWeightSamplerblock;

        FastArray<Ocean*>       mLinkedOceans;

        ConstBufferPool::BufferPool const *mLastBoundPool;

        uint32 mLastTextureHash;
        MovableObject const *mLastMovableObject;

        bool mDebugPssmSplits;

        ShadowFilter mShadowFilter;
        AmbientLightMode mAmbientLightMode;

        uint16 mBaseTexUnitForPass = 3u;     // 3 or 5 (Forward+)
        uint16 mNumShadowMapsForPass = 0u;   // how many you actually bind
        uint16 mEnvProbeTexUnitForPass = 0u; // where probe should be bound (if enabled)
        bool   mHasEnvProbeForPass = false;

	    /// Resource names used to bind the ocean simulation textures.
	    /// Default values match the sample media: "oceanData.dds" and "weight.dds".
	    String mOceanDataTextureName;
	    String mWeightTextureName;
        TextureGpu* mOceanDataTex = nullptr;
        TextureGpu* mWeightTex = nullptr;
        bool        mOceanTexReady = false;
        uint32 mLastCacheHash = 0;

        const HlmsCache *createShaderCacheEntry(uint32 renderableHash, const HlmsCache& passCache,
                                            uint32                  finalHash,
                                            const QueuedRenderable& queuedRenderable,
                                            HlmsCache* reservedStubEntry, uint64 deadline,
                                            size_t tid) override;

        HlmsDatablock *createDatablockImpl( IdString datablockName, const HlmsMacroblock *macroblock,
                                            const HlmsBlendblock *blendblock,
                                            const HlmsParamVec &paramVec ) override;

        void setTextureProperty(size_t tid, const char* propertyName, HlmsOceanDatablock* datablock,
            OceanTextureTypes texType);

        void calculateHashForPreCreate( Renderable *renderable, PiecesMap *inOutPieces ) override;

        void calculateHashFor(Renderable* renderable, uint32& outHash, uint32& outCasterHash) override;

        PropertiesMergeStatus notifyPropertiesMergedPreGenerationStep(size_t tid, PiecesMap* inOutPieces) override;

        FORCEINLINE uint32 fillBuffersFor( const HlmsCache *cache,
                                           const QueuedRenderable &queuedRenderable,
                                           bool casterPass, uint32 lastCacheHash,
                                           CommandBuffer *commandBuffer, bool isV1 );

        void ensureOceanTexturesLoaded();
    public:
        HlmsOcean( Archive *dataFolder, ArchiveVec *libraryFolders );
        ~HlmsOcean() override;

        void _linkOcean(Ocean* ocean);
        void _unlinkOcean(Ocean* ocean);

        void _changeRenderSystem( RenderSystem *newRs ) override;

        /// Not supported
        virtual void setOptimizationStrategy( OptimizationStrategy optimizationStrategy ) {}

        uint32 fillBuffersFor( const HlmsCache *cache, const QueuedRenderable &queuedRenderable,
                               bool casterPass, uint32 lastCacheHash,
                               uint32 lastTextureHash ) override;

        uint32 fillBuffersForV1( const HlmsCache *cache, const QueuedRenderable &queuedRenderable,
                                 bool casterPass, uint32 lastCacheHash,
                                 CommandBuffer *commandBuffer ) override;
        uint32 fillBuffersForV2( const HlmsCache *cache, const QueuedRenderable &queuedRenderable,
                                 bool casterPass, uint32 lastCacheHash,
                                 CommandBuffer *commandBuffer ) override;

#if !OGRE_NO_JSON
        /// @copydoc Hlms::_loadJson
        void _loadJson(const rapidjson::Value& jsonValue, const HlmsJson::NamedBlocks& blocks,
            HlmsDatablock* datablock, const String& resourceGroup,
            HlmsJsonListener* listener,
            const String& additionalTextureExtension) const override;
        /// @copydoc Hlms::_saveJson
        void _saveJson(const HlmsDatablock* datablock, String& outString, HlmsJsonListener* listener,
            const String& additionalTextureExtension) const override;

        /// @copydoc Hlms::_collectSamplerblocks
        void _collectSamplerblocks(set<const HlmsSamplerblock*>::type& outSamplerblocks,
            const HlmsDatablock* datablock) const override;
#endif

        void setDebugPssmSplits( bool bDebug );
        bool getDebugPssmSplits(void) const                 { return mDebugPssmSplits; }

        void setShadowSettings( ShadowFilter filter );
        ShadowFilter getShadowFilter(void) const            { return mShadowFilter; }

        void setAmbientLightMode( AmbientLightMode mode );
        AmbientLightMode getAmbientLightMode(void) const    { return mAmbientLightMode; }

        /**
         * @brief Sets the environment probe (cubemap) used for ocean reflections.
         * Loads (or retrieves) a cubemap and assigns it to a specific ocean datablock.
         * Passing an empty texture name clears the reflection for that datablock.
         *
         * Per-datablock environment probe (override)
         * Individual ocean datablocks can override the global probe by assigning
         * a texture to the OCEAN_REFLECTION slot.
         *
         * Example:
         * @code
         * oceanDatablockTropical->setTexture( OCEAN_REFLECTION, tropicalSkyCubemap );
         * oceanDatablockArctic->setTexture( OCEAN_REFLECTION, arcticSkyCubemap );
         * @endcode
         *
         * Characteristics:
         * - Different ocean areas can reflect different skies
         * - Overrides the global probe when present
         * - Requires managing multiple cubemaps
         * - Higher memory usage
         *
         * #### 3. Hybrid approach (recommended)
         * Use a global probe as a default and override it only for special ocean areas.
         *
         * Example:
         * @code
         * // Global default
         * hlmsOcean->setEnvProbe( defaultSkyCubemap );
         *
         * // Most oceans use the global sky
         * oceanDatablock1; // defaultSkyCubemap
         * oceanDatablock2; // defaultSkyCubemap
         *
         * // Special cases override the reflection
         * oceanDatablockSunset->setTexture( OCEAN_REFLECTION, sunsetSkyCubemap );
         * oceanDatablockStorm->setTexture( OCEAN_REFLECTION, stormSkyCubemap );
         * @endcode
         *
         * Changing the global probe later will only affect datablocks that do not
         * have a per-datablock override.
         *
         * ### Notes
         * - Passing nullptr disables the global environment probe
         * - Per-datablock textures always take priority over the global probe
         * - Intended for static or semi-static sky reflections (not dynamic probes)
         *
         * @param probe Pointer to the cubemap texture used as the global environment probe,
         *              or nullptr to disable global ocean reflections.
         */
        void HlmsOcean::setDatablockEnvReflection( HlmsOceanDatablock* datablock, Ogre::TextureGpu* reflectionTexture );

        void HlmsOcean::setDatablockEnvReflection( HlmsOceanDatablock* datablock, const Ogre::String& textureName );


	    /// Overrides the resource name of the 3D texture used by the ocean shader.
	    /// Default: "oceanData.dds"
	    void setOceanDataTextureName( const String &name ) { mOceanDataTextureName = name; }
	    const String& getOceanDataTextureName() const { return mOceanDataTextureName; }

	    /// Overrides the resource name of the 2D weight texture used by the ocean shader.
	    /// Default: "weight.dds"
	    void setWeightTextureName( const String &name ) { mWeightTextureName = name; }
	    const String& getWeightTextureName() const { return mWeightTextureName; }

        /// Same helper as other Hlms implementations (e.g. HlmsTerra) to populate
        /// the default data folder + library folders based on the active RenderSystem.
        static void getDefaultPaths( String &outDataFolderPath, StringVector &outLibraryFoldersPaths );
    protected:
        // Add diagnostic tracking
        size_t mLastAllocatedPassBufferSize = 0;
        size_t mLastWrittenPassBufferSize = 0;
    };

    struct OceanProperty
    {
        static const IdString HwGammaRead;
        static const IdString HwGammaWrite;
        static const IdString SignedIntTex;
        static const IdString MaterialsPerBuffer;
        static const IdString DebugPssmSplits;

        /// Number of extra constant buffer slots reserved for AtmosphereComponent.
        static const IdString AtmosphereNumConstBuffers;

        static const IdString UseSkirts;

        static const char *EnvProbeMap;

        static const IdString FresnelScalar;
        static const IdString MetallicWorkflow;
        static const IdString ReceiveShadows;

        static const IdString DetailOffsets0;
        static const IdString DetailOffsets1;
        static const IdString DetailOffsets2;
        static const IdString DetailOffsets3;

        static const IdString DetailMapsDiffuse;
        static const IdString DetailMapsNormal;
        static const IdString FirstValidDetailMapNm;

        static const IdString Pcf3x3;
        static const IdString Pcf4x4;
        static const IdString PcfIterations;

        static const IdString EnvMapScale;
        static const IdString AmbientFixed;
        static const IdString AmbientHemisphere;

        static const IdString BrdfDefault;
        static const IdString BrdfCookTorrance;
        static const IdString BrdfBlinnPhong;
        static const IdString FresnelSeparateDiffuse;
        static const IdString GgxHeightCorrelated;

        static const IdString *DetailOffsetsPtrs[4];
    };

    /** @} */
    /** @} */
}

#endif

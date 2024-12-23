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

#if 0

#include "OgreHlmsBufferManager.h"
#include "OgreConstBufferPool.h"
#include "OgreMatrix4.h"
#include "OgreHeaderPrefix.h"

namespace Ogre
{
    class CompositorShadowNode;
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
    class HlmsOcean : public HlmsBufferManager, public ConstBufferPool
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
            FastArray<Texture*> shadowMaps;
            FastArray<float>    vertexShaderSharedBuffer;
            FastArray<float>    pixelShaderSharedBuffer;

            Matrix4 viewMatrix;
        };

        PassData                mPreparedPass;
        ConstBufferPackedVec    mPassBuffers;
        HlmsSamplerblock const  *mShadowmapSamplerblock;    /// GL3+ only when not using depth textures
        HlmsSamplerblock const  *mShadowmapCmpSamplerblock; /// For depth textures & D3D11
        HlmsSamplerblock const  *mCurrentShadowmapSamplerblock;
        HlmsSamplerblock const  *mOceanSamplerblock;

        uint32                  mCurrentPassBuffer;     /// Resets every to zero every new frame.

        TexBufferPacked         *mGridBuffer;
        TexBufferPacked         *mGlobalLightListBuffer;

        ConstBufferPool::BufferPool const *mLastBoundPool;

        uint32 mLastTextureHash;
        MovableObject const *mLastMovableObject;

        bool mDebugPssmSplits;

        ShadowFilter mShadowFilter;
        AmbientLightMode mAmbientLightMode;

        Ogre::TexturePtr mProbe;

        virtual const HlmsCache* createShaderCacheEntry( uint32 renderableHash,
                                                         const HlmsCache &passCache,
                                                         uint32 finalHash,
                                                         const QueuedRenderable &queuedRenderable );

        virtual HlmsDatablock* createDatablockImpl( IdString datablockName,
                                                    const HlmsMacroblock *macroblock,
                                                    const HlmsBlendblock *blendblock,
                                                    const HlmsParamVec &paramVec );

        virtual void calculateHashForPreCreate( Renderable *renderable, PiecesMap *inOutPieces );

        virtual void destroyAllBuffers(void);

        FORCEINLINE uint32 fillBuffersFor( const HlmsCache *cache,
                                           const QueuedRenderable &queuedRenderable,
                                           bool casterPass, uint32 lastCacheHash,
                                           CommandBuffer *commandBuffer, bool isV1 );

    public:
        HlmsOcean( Archive *dataFolder, ArchiveVec *libraryFolders );
        virtual ~HlmsOcean();

        virtual void _changeRenderSystem( RenderSystem *newRs );

        /// Not supported
        virtual void setOptimizationStrategy( OptimizationStrategy optimizationStrategy ) {}

        virtual HlmsCache preparePassHash( const Ogre::CompositorShadowNode *shadowNode,
                                           bool casterPass, bool dualParaboloid,
                                           SceneManager *sceneManager );

        virtual uint32 fillBuffersFor( const HlmsCache *cache, const QueuedRenderable &queuedRenderable,
                                       bool casterPass, uint32 lastCacheHash,
                                       uint32 lastTextureHash );

        virtual uint32 fillBuffersForV1( const HlmsCache *cache,
                                         const QueuedRenderable &queuedRenderable,
                                         bool casterPass, uint32 lastCacheHash,
                                         CommandBuffer *commandBuffer );
        virtual uint32 fillBuffersForV2( const HlmsCache *cache,
                                         const QueuedRenderable &queuedRenderable,
                                         bool casterPass, uint32 lastCacheHash,
                                         CommandBuffer *commandBuffer );

        virtual void frameEnded(void);

        void setDebugPssmSplits( bool bDebug );
        bool getDebugPssmSplits(void) const                 { return mDebugPssmSplits; }

        void setShadowSettings( ShadowFilter filter );
        ShadowFilter getShadowFilter(void) const            { return mShadowFilter; }

        void setAmbientLightMode( AmbientLightMode mode );
        AmbientLightMode getAmbientLightMode(void) const    { return mAmbientLightMode; }

        void setEnvProbe( Ogre::TexturePtr probe );
    };

    struct OceanProperty
    {
        static const IdString HwGammaRead;
        static const IdString HwGammaWrite;
        static const IdString SignedIntTex;
        static const IdString MaterialsPerBuffer;
        static const IdString DebugPssmSplits;

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

#include "OgreHeaderSuffix.h"

#endif

#endif

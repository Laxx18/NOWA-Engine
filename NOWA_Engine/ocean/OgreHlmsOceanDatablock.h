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
#ifndef _OgreHlmsOceanDatablock_H_
#define _OgreHlmsOceanDatablock_H_

#include "OgreHlmsOceanPrerequisites.h"
#include "OgreHlmsDatablock.h"
#include "OgreConstBufferPool.h"

// ADD THIS: Texture base class support
#define _OgreHlmsTextureBaseClassExport
#define OGRE_HLMS_TEXTURE_BASE_CLASS HlmsOceanBaseTextureDatablock
#define OGRE_HLMS_TEXTURE_BASE_MAX_TEX NUM_OCEAN_TEXTURE_TYPES
#define OGRE_HLMS_CREATOR_CLASS HlmsOcean
#include "OgreHlmsTextureBaseClass.h"
#undef _OgreHlmsTextureBaseClassExport
#undef OGRE_HLMS_TEXTURE_BASE_CLASS
#undef OGRE_HLMS_TEXTURE_BASE_MAX_TEX
#undef OGRE_HLMS_CREATOR_CLASS

namespace Ogre
{
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Resources
    *  @{
    */
    namespace OceanBrdf
    {
        enum OceanBrdf
        {
            FLAG_UNCORRELATED                           = 0x80000000,
            FLAG_SPERATE_DIFFUSE_FRESNEL                = 0x40000000,
            BRDF_MASK                                   = 0x00000FFF,

            /// Most physically accurate BRDF we have. Good for representing
            /// the majority of materials.
            /// Uses:
            ///     * Roughness/Distribution/NDF term: GGX
            ///     * Geometric/Visibility term: Smith GGX Height-Correlated
            ///     * Normalized Disney Diffuse BRDF,see
            ///         "Moving Frostbite to Physically Based Rendering" from
            ///         Sebastien Lagarde & Charles de Rousiers
            Default         = 0x00000000,

            /// Implements Cook-Torrance BRDF.
            /// Uses:
            ///     * Roughness/Distribution/NDF term: Beckmann
            ///     * Geometric/Visibility term: Cook-Torrance
            ///     * Lambertian Diffuse.
            ///
            /// Ideal for silk (use high roughness values), synthetic fabric
            CookTorrance    = 0x00000001,

            /// Implements Normalized Blinn Phong using a normalization
            /// factor of (n + 8) / (8 * pi)
            /// The main reason to use this BRDF is performance. It's cheap.
            BlinnPhong      = 0x00000002,

            /// Same as Default, but the geometry term is not height-correlated
            /// which most notably causes edges to be dimmer and is less correct.
            /// Unity (Marmoset too?) use an uncorrelated term, so you may want to
            /// use this BRDF to get the closest look for a nice exchangeable
            /// pipeline workflow.
            DefaultUncorrelated             = Default|FLAG_UNCORRELATED,

            /// Same as Default but the fresnel of the diffuse is calculated
            /// differently. Normally the diffuse component would be multiplied against
            /// the inverse of the specular's fresnel to maintain energy conservation.
            /// This has the nice side effect that to achieve a perfect mirror effect,
            /// you just need to raise the fresnel term to 1; which is very intuitive
            /// to artists (specially if using coloured fresnel)
            ///
            /// When using this BRDF, the diffuse fresnel will be calculated differently,
            /// causing the diffuse component to still affect the colour even when
            /// the fresnel = 1 (although subtly). To achieve a perfect mirror you will
            /// have to set the fresnel to 1 *and* the diffuse colour to black;
            /// which can be unintuitive for artists.
            ///
            /// This BRDF is very useful for representing surfaces with complex refractions
            /// and reflections like glass, transparent plastics, fur, and surface with
            /// refractions and multiple rescattering that cannot be represented well
            /// with the default BRDF.
            DefaultSeparateDiffuseFresnel   = Default|FLAG_SPERATE_DIFFUSE_FRESNEL,

            /// @see DefaultSeparateDiffuseFresnel. This is the same
            /// but the Cook Torrance model is used instead.
            ///
            /// Ideal for shiny objects like glass toy marbles, some types of rubber.
            /// silk, synthetic fabric.
            CookTorranceSeparateDiffuseFresnel  = CookTorrance|FLAG_SPERATE_DIFFUSE_FRESNEL,

            BlinnPhongSeparateDiffuseFresnel    = BlinnPhong|FLAG_SPERATE_DIFFUSE_FRESNEL,
        };
    }

    /** 
     * Contains information needed by Ocean (Physically Based Shading) for OpenGL 3+ & D3D11+
    */
    class HlmsOceanDatablock : public HlmsOceanBaseTextureDatablock
    {
        friend class HlmsOcean;

    protected:
        Vector4 mDeepColour;        // RGB + reflectionStrength
        Vector4 mShallowColour;     // RGB + waveScale
        Vector4 mkD;                // RGB + shadowConstantBias
        Vector4 mRoughness;         // x=baseRoughness, y=foamRoughness, z=unused, w=unused
        Vector4 mMetalness;         // x=ambientReduction, y=diffuseScale, z=foamIntensity, w=unused
        Vector4 mDetailOffsetScale[4];
        uint32  mIndices0_7[4];
        uint32  mIndices8_15[4];
        uint32  mIndices16_24[4];

        uint32  mBrdf;

        void cloneImpl(HlmsDatablock* datablock) const override;

        void scheduleConstBufferUpdate(void);
        virtual void uploadToConstBuffer(char* dstPtr, uint8 dirtyFlags);

    public:
        HlmsOceanDatablock( IdString name, HlmsOcean *creator,
                          const HlmsMacroblock *macroblock,
                          const HlmsBlendblock *blendblock,
                          const HlmsParamVec &params );
        virtual ~HlmsOceanDatablock();

        void setDeepColour( const Vector3 &deepColour );
        Vector3 getDeepColour(void) const;

        void setShallowColour( const Vector3 &shallowColour );
        Vector3 getShallowColour(void) const;

        // ===== Reflection =====
        /// Controls strength of cubemap reflections (0.0 = none, 1.0 = full)
        /// Stored in deepColour.w
        void setReflectionStrength(float strength);
        float getReflectionStrength() const;

        // ===== Roughness =====
        /// Base roughness for calm water (typically 0.01-0.05)
        void setBaseRoughness(float roughness);
        float getBaseRoughness() const;

        /// Roughness for foamy water (typically 0.3-0.7)
        void setFoamRoughness(float roughness);
        float getFoamRoughness() const;

        // ===== Ambient & Diffuse =====
        /// Reduction factor for ambient hemisphere when reflections are active (0.0 = none, 1.0 = 50% reduction)
        void setAmbientReduction(float reduction);
        float getAmbientReduction() const;

        /// Scale factor for diffuse color (0.0 = pure reflective, 1.0 = full diffuse)
        void setDiffuseScale(float scale);
        float getDiffuseScale() const;

        // ===== Foam =====
        /// Controls foam intensity (0.0 = no foam, 1.0 = full foam)
        void setFoamIntensity(float intensity);
        float getFoamIntensity() const;


        void setWavesScale( float scale );
        float getWavesScale() const;

        void setShadowConstantBias(float bias);
        float getShadowConstantBias() const;

        /// Overloaded to tell it's unsupported
        virtual void setAlphaTestThreshold( float threshold );

        /// Changes the BRDF in use. Calling this function may trigger an
        /// HlmsDatablock::flushRenderables
        void setBrdf( OceanBrdf::OceanBrdf brdf );
        uint32 getBrdf(void) const;

        bool suggestUsingSRGB(OceanTextureTypes type) const;

        virtual void calculateHash();

        // Size calculation: 
        // deepColour(16) + shallowColour(16) + kD(16) + roughness(16) + metalness(16) 
        // + detailOffsetScale[4](64) + indices0_7(16) + indices8_15(16) + indices16_24(16)
        // = 192 bytes
        static const size_t MaterialSizeInGpu = 192;
        static const size_t MaterialSizeInGpuAligned = 192; // Already aligned to 16
    };

    /** @} */
    /** @} */

}

#include "OgreHeaderSuffix.h"

#endif
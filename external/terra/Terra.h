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

#ifndef _OgreTerra_H_
#define _OgreTerra_H_

#include "OgrePrerequisites.h"
#include "OgreMovableObject.h"
#include "OgreShaderParams.h"

#include "TerrainCell.h"
#include "OgreImage2.h"

namespace Ogre
{
    struct GridPoint
    {
        int32 x;
        int32 z;
    };

    struct GridDirection
    {
        int x;
        int z;
    };

    class ShadowMapper;
    struct TerraSharedResources;

    /**
    @brief The Terra class
        Internally Terra operates in Y-up space so input and outputs may
        be converted to/from the correct spaces based on setting, unless
        explicitly stated to be always Y-up by documentation.
    */
    class Terra : public MovableObject
    {
        friend class TerrainCell;

        struct SavedState
        {
            RenderableArray m_renderables;
            size_t m_currentCell;
            Camera const* m_camera;
        };

        std::vector<float>          m_heightMap;
        std::vector<Ogre::uint16>   m_oldHeightData;
        std::vector<Ogre::uint8>    m_oldBlendWeightData;

        std::vector<Ogre::uint16>   m_newHeightData;
        std::vector<Ogre::uint8>    m_newBlendWeightData;

        uint32                      m_width;
        uint32                      m_depth; //PNG's Height
        float                       m_depthWidthRatio;
        float                       m_skirtSize;//Alreadyunormscaled
        float                       m_invWidth;
        float                       m_invDepth;

        bool m_zUp;

        Vector2     m_xzDimensions;
        Vector2     m_xzInvDimensions;
        Vector2     m_xzRelativeSize; // m_xzDimensions / [m_width, m_height]
        Vector3     m_dimension;
        float       m_height;
        float       m_heightUnormScaled; // m_height / 1 or m_height / 65535
        Vector3     m_terrainOrigin;
        uint32      m_basePixelDimension;
        float       m_invMaxValue;
       
        /// 0 is currently in use
        /// 1 is SavedState
        std::vector<TerrainCell>   m_terrainCells[2];
        /// 0 & 1 are for tmp use

        std::vector<TerrainCell*>  m_collectedCells[2];
        size_t                     m_currentCell;

        Ogre::TextureGpu*   m_heightMapTex;
        Ogre::TextureGpu*   m_normalMapTex;
        Ogre::TextureGpu*   m_blendWeightTex;

        StagingTexture* m_heightMapStagingTexture;
        StagingTexture* m_blendWeightStagingTexture;

        Vector3             m_prevLightDir;
        ShadowMapper        *m_shadowMapper;
        
        TerraSharedResources *m_sharedResources;

        CompositorWorkspace* mNormalMapperWorkspace;
        Camera* mNormalMapCamera;

        /// When rendering shadows we want to override the data calculated by update
        /// but only temporarily, for later restoring it.
        SavedState m_savedState;

        //Ogre stuff
        CompositorManager2*     m_compositorManager;
        Camera const*           m_camera;
        std::vector<float>      m_brushData;
        uint8                   m_brushSize;
        uint8                   m_blendLayer;
        Ogre::String            m_prefix;
        Ogre::String            m_currentBlendWeightImageName;
        Ogre::String            m_currentHeightMapImageName;
        Ogre::Image2            m_blendWeightImage;

        void destroyHeightmapTexture(void);

        /// Calls createHeightmapTexture, loads image data to our CPU-side buffers
        void createHeightmap( Image2 &image, bool bMinimizeMemoryConsumption, bool bLowResShadow);

        void createHeightmap(float height, Ogre::uint16 imageWidth, Ogre::uint16 imageHeight, bool bMinimizeMemoryConsumption, bool bLowResShadow);

        void createNormalTexture(void);
        void destroyNormalTexture(void);

        void createBlendWeightTexture(Image2& image);

        void createBlendWeightTexture(Ogre::uint16 imageWidth, Ogre::uint16 imageHeight);

        void destroyBlendWeightTexture(void);
   
        ///	Automatically calculates the optimum skirt size (no gaps with
        /// lowest overdraw possible).
        ///	This is done by taking the heighest delta between two adjacent
        /// pixels in a 4x4 block.
        ///	This calculation may not be perfect, as the block search should
        /// get bigger for higher LODs.
        void calculateOptimumSkirtSize(void);

        void createTerrainCells();

        inline GridPoint worldToGrid(const Vector3& vPos) const;
        inline Vector2 gridToWorld(const GridPoint& gPos) const;
        
        bool isVisible( const GridPoint &gPos, const GridPoint &gSize ) const;

        void addRenderable( const GridPoint &gridPos, const GridPoint &cellSize, uint32 lodLevel );

        void optimizeCellsAndAdd(void);

        void _loadInternal(Image2* heightMapImage, Image2* blendWeightImage, Vector3& center, Vector3& dimensions, 
            const String& heightMapImageName, const String& blendWeightImageName, float height, Ogre::uint16 imageWidth, Ogre::uint16 imageHeight, bool bMinimizeMemoryConsumption, bool bLowResShadow);

        void _calculateBoxSpecification(int x, int y, int boxSize, uint32 pitch, uint32 depth, uint32* movAmount, uint32* shouldMoveAmount, int* boxStartX, int* boxWidth, int* boxStartY, int* boxHeight);

    public:
        Terra( IdType id, ObjectMemoryManager *objectMemoryManager, SceneManager *sceneManager,
               uint8 renderQueueId, CompositorManager2 *compositorManager, Camera *camera, bool zUp );
        ~Terra();

        /// Creates the Ogre texture based on the image data.
        /// Called by @see createHeightmap
        void createHeightmapTexture(const Image2& image);

        /// Sets shared resources for minimizing memory consumption wasted on temporary
        /// resources when you have more than one Terra.
        ///
        /// This function is only useful if you load/have multiple Terras at once.
        ///
        /// @see    TerraSharedResources
        void setSharedResources( TerraSharedResources *sharedResources );

        /// How low should the skirt be. Normally you should let this value untouched and let
        /// calculateOptimumSkirtSize do its thing for best performance/quality ratio.
        ///
        /// However if your height values are unconventional (i.e. artificial, non-natural) and you
        /// need to look the terrain from the "outside" (rather than being inside the terrain),
        /// you may have to tweak this value manually.
        ///
        /// This value should be between min height and max height of the heightmap.
        ///
        /// A value of 0.0 will give you the biggest skirt and fix all skirt-related issues.
        /// Note however, this may have a *tremendous* GPU performance impact.
        void setCustomSkirtMinHeight( const float skirtMinHeight ) { m_skirtSize = skirtMinHeight; }

        float getCustomSkirtMinHeight( void ) const { return m_skirtSize; }
        
        /** Must be called every frame so we can check the camera's position
            (passed in the constructor) and update our visible batches (and LODs)
            We also update the shadow map if the light direction changed.
        @param lightDir
            Light direction for computing the shadow map.
        @param lightEpsilon
            Epsilon to consider how different light must be from previous
            call to recompute the shadow map.
            Interesting values are in the range [0; 2], but any value is accepted.
        @par
            Large epsilons will reduce the frequency in which the light is updated,
            improving performance (e.g. only compute the shadow map when needed)
        @par
            Use an epsilon of <= 0 to force recalculation every frame. This is
            useful to prevent heterogeneity between frames (reduce stutter) if
            you intend to update the light slightly every frame.
        */
        void update( const Vector3 &lightDir, float lightEpsilon=1e-6f );

        void load(const String& heightMapTextureName, const String& blendWeightTextureName, Vector3& center, Vector3& dimensions, bool bMinimizeMemoryConsumption, bool bLowResShadow);
        void load(Image2 &heightMapImage, Image2& blendWeightImage, Vector3& center, Vector3& dimensions, bool bMinimizeMemoryConsumption, bool bLowResShadow);
        void load(float height,Ogre::uint16 imageWidth, Ogre::uint16 imageHeight, Vector3& center, Vector3& dimensions, bool bMinimizeMemoryConsumption, bool bLowResShadow);

        /** Lower values makes LOD very aggressive. Higher values less aggressive.
        @param basePixelDimension
            Must be power of 2.
        */
        void setBasePixelDimension(uint32 basePixelDimension);

         /** Gets the interpolated height at the given location.
            If outside the bounds, it leaves the height untouched.
        @param vPos
            Y-up:
                [in] XZ position, Y for default height.
                [out] Y height, or default Y (from input) if outside terrain bounds.
            Z-up
                [in] XY position, Z for default height.
                [out] Z height, or default Z (from input) if outside terrain bounds.
        @return
            True if Y (or Z for Z-up) component was changed
        */
        bool getHeightAt( Vector3 &vPosArg ) const;

        std::vector<int> getLayerAt(const Ogre::Vector3& position);

        /// load must already have been called.
        void setDatablock( HlmsDatablock *datablock );

        //MovableObject overloads
        const String& getMovableType(void) const;

         /// Swaps current state with a saved one. Useful for rendering shadow maps
        void _swapSavedState( void );

        const Camera* getCamera() const                       { return m_camera; }
        void setCamera(const Camera *camera )                { m_camera = camera; }

        bool isZUp( void ) const { return m_zUp; }

        const ShadowMapper* getShadowMapper(void) const { return m_shadowMapper; }

        Ogre::TextureGpu* getHeightMapTex(void) const   { return m_heightMapTex; }
        Ogre::TextureGpu* getNormalMapTex(void) const   { return m_normalMapTex; }
        TextureGpu* _getShadowMapTex(void) const;

        // These are always in Y-up space
        const Vector2& getXZDimensions(void) const      { return m_xzDimensions; }
        const Vector2& getXZInvDimensions(void) const   { return m_xzInvDimensions; }
        float getHeight(void) const                     { return m_height; }

        /// Return value is in client-space (i.e. could be y- or z-up)
        Vector3 getTerrainOriginRaw(void) const;

        Vector3 getTerrainOrigin(void) const;

        // Always in Y-up space
        Vector2 getTerrainXZCenter(void) const;
        // TODO: Is this correct??
		std::vector<TerrainCell>& Terra::getTerrainCells(void) { return m_terrainCells[0]; }

        /**
         Set the height of the terrain to be a single value.
        */
        void levelTerrain(float height);

        inline uint8 alterBlend(uint8 oldValue, float diff);

        void setBrushName(const Ogre::String& brushName);

        void setBrushSize(short brushSize);

        void modifyTerrainStart(const Ogre::Vector3& position, float strength);

        void smoothTerrainStart(const Ogre::Vector3& position, float strength);

        void paintTerrainStart(const Ogre::Vector3& position, float intensity, int blendLayer);

        void modifyTerrain(const Ogre::Vector3& position, float strength);

        void smoothTerrain(const Ogre::Vector3& position, float strength);

        void paintTerrain(const Ogre::Vector3& position, float intensity, int blendLayer);

        std::pair<std::vector<Ogre::uint16>, std::vector<Ogre::uint16>> modifyTerrainFinished(void);

         std::pair<std::vector<Ogre::uint16>, std::vector<Ogre::uint16>> smoothTerrainFinished(void);

        std::pair<std::vector<Ogre::uint8>, std::vector<Ogre::uint8>> paintTerrainFinished(void);

        void setHeightData(const std::vector<Ogre::uint16>& heightData);

        void setBlendWeightData(const std::vector<Ogre::uint8>& blendWeightData);

        void applyHeightDiff(const Ogre::Vector3& position, const std::vector<float>& data, int boxSize, float strength);

        void applySmoothDiff(const Ogre::Vector3& position, const std::vector<float>& data, int boxSize, float strength);

        void applyBlendDiff(const Ogre::Vector3& position, const std::vector<float>& brushData, int boxSize, float intensity, int blendLayer);

        void saveTextures(const Ogre::String& path, const Ogre::String& prefix);

        /**
        Update the textures related to the height map (such as the generated normal map).
        This should be called each time the heightmap is changed.
        */
        void updateHeightTextures();

        std::tuple<bool, Vector3, Vector3, Real> checkRayIntersect(const Ogre::Ray& ray);

        std::tuple<bool, Vector3, Vector3, Real> checkQuadIntersection(int x, int z, const Ray& ray);

        Real getHeightData(int x, int y);

        template <typename T>
        inline float valToHeight(T height)
        {
            return (height * m_invMaxValue) * m_height;
        }

        template <typename T>
        inline T heightToVal(float height)
        {
            return (height / m_invMaxValue) / m_height;
        }

        void setPrefix(const Ogre::String& prefix);

        /// Converts value from Y-up to whatever the user up vector is (see m_zUp)
        inline Vector3 fromYUp( Vector3 value ) const;
        /// Same as fromYUp, but preserves original sign. Needed when value is a scale
        inline Vector3 fromYUpSignPreserving( Vector3 value ) const;
        /// Converts value from user up vector to Y-up
        inline Vector3 toYUp( Vector3 value ) const;
        /// Same as toYUp, but preserves original sign. Needed when value is a scale
        inline Vector3 toYUpSignPreserving( Vector3 value ) const;

        Ogre::String getHeightMapTextureName(void) const;

        Ogre::String getBlendWeightTextureName(void) const;
    public:
        Ogre::uint32 mHlmsTerraIndex;
    };
    
    
    /** Terra during creation requires a number of temporary surfaces that are used then discarded.
        These resources can be shared by all the Terra 'islands' since as long as they have the
        same resolution and format.

        Originally, we were creating them and destroying them immediately. But because
        GPUs are async and drivers don't discard resources immediately, it was causing
        out of memory conditions.

        Once all N Terras are constructed, memory should be freed.

        Usage:

        @code
            // At level load:
            TerraSharedResources *sharedResources = new TerraSharedResources();
            for( i < numTerras )
            {
                terra[i]->setSharedResources( sharedResources );
                terra[i]->load( "Heightmap.png" );
            }

            // Free memory that is only used during Terra::load()
            sharedResources->freeStaticMemory();

            // Every frame
            for( i < numTerras )
                terra[i]->update( lightDir );

            // On shutdown:
            for( i < numTerras )
                delete terra[i];
            delete sharedResources;
        @endcode
    */
    struct TerraSharedResources
    {
        enum TemporaryUsages
        {
            TmpNormalMap,
            NumStaticTmpTextures,
            TmpShadows = NumStaticTmpTextures,
            NumTemporaryUsages
        };

        TextureGpu *textures[NumTemporaryUsages];

        TerraSharedResources();
        ~TerraSharedResources();

        /// Destroys all textures in the cache
        void freeAllMemory();

        /// Destroys all textures that are only used during heightmap load
        void freeStaticMemory();

        /**
        @brief getTempTexture
            Retrieves a cached texture to be shared with all Terras.
            If no such texture exists, creates it.

            If sharedResources is a nullptr (i.e. no sharing) then we create a temporary
            texture that will be freed by TerraSharedResources::destroyTempTexture

            If sharedResources is not nullptr, then the texture will be freed much later on
            either by TerraSharedResources::freeStaticMemory or the destructor
        @param terra
        @param sharedResources
            Can be nullptr
        @param temporaryUsage
            Type of texture to use
        @param baseTemplate
            A TextureGpu that will be used for basis of constructing our temporary RTT
        @return
            A valid ptr
        */
        static TextureGpu *getTempTexture( const char *texName, IdType id,
                                           TerraSharedResources *sharedResources,
                                           TemporaryUsages temporaryUsage, TextureGpu *baseTemplate,
                                           uint32 flags );
        /**
        @brief destroyTempTexture
            Destroys a texture created by TerraSharedResources::getTempTexture ONLY IF sharedResources
            is nullptr
        @param sharedResources
        @param tmpRtt
        */
        static void destroyTempTexture( TerraSharedResources *sharedResources, TextureGpu *tmpRtt );
	};
}

#endif

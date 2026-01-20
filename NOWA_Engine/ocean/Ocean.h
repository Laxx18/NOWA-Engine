
#ifndef _OgreOcean_H_
#define _OgreOcean_H_

#include "OgrePrerequisites.h"
#include "OgreMovableObject.h"
#include "OgreShaderParams.h"
#include "Ocean/OceanCell.h"

namespace Ogre
{
	class HlmsOceanDatablock;

    struct OceanGridPoint
    {
        int32 x;
        int32 z;
    };

    struct OceanGridDirection
    {
        int x;
        int z;
    };


    class Ocean : public MovableObject
    {
        friend class OceanCell;
        friend class HlmsOcean;

        struct SavedState
        {
            RenderableArray m_renderables;
            size_t m_currentCell;
            Camera const* m_camera;
        };

        uint32 m_hlmsOceanIndex;

        std::vector<float>          m_heightMap;
        uint32                      m_width;
        uint32                      m_depth; //PNG's Height
        float                       m_depthWidthRatio;
        float                       m_invWidth;
        float                       m_invDepth;
        float                       m_uvScale;

        Vector2     m_xzDimensions;
        Vector2     m_xzInvDimensions;
        Vector2     m_xzRelativeSize; // m_xzDimensions / [m_width, m_height]
        float       m_height;
        float       m_WavesIntensity;
        Vector3     m_OceanOrigin;
        uint32      m_basePixelDimension;

        bool        m_useSkirts;
        float       m_oceanTime;
        float       m_waveTimeScale;
        float       m_waveFrequencyScale;
        float       m_waveChaos;
        bool        m_isUnderwater;

        std::vector<OceanCell>   m_OceanCells[2];
        std::vector<OceanCell*>  m_collectedCells[2];
        size_t                     m_currentCell;

        // Ogre stuff
        CompositorManager2* m_compositorManager;
        Camera const*       m_camera;
        SavedState          m_savedState;

        inline OceanGridPoint worldToGrid( const Vector3 &vPos ) const;
        inline Vector2 gridToWorld( const OceanGridPoint &gPos ) const;

        bool isVisible( const OceanGridPoint &gPos, const OceanGridPoint &gSize ) const;

        void addRenderable( const OceanGridPoint &gridPos, const OceanGridPoint &cellSize, uint32 lodLevel );

        void optimizeCellsAndAdd(void);

        void updateLocalBounds(void);

        void createOceanCells();

        void sortCellsByDepth(const Vector3& cameraPos);

    public:
        Ocean(IdType id, ObjectMemoryManager* objectMemoryManager, SceneManager* sceneManager,
               uint8 renderQueueId, CompositorManager2* compositorManager, const Camera* camera);

        ~Ocean() override;

        /** Must be called every frame so we can check the camera's position
            (passed in the constructor) and update our visible batches (and LODs).
        */
        void update(Ogre::Real dt);

        void create(const Vector3 center, const Vector2 &dimensions);

        void setWavesIntensity( float intensity );

        void setWavesScale( float scale );

        // Add this method declaration
        void setBasePixelDimension(uint32 basePixelDimension);

        // Add getter too
        uint32 getBasePixelDimension() const { return m_basePixelDimension; }

        /// Enable/disable skirts. Must be set before create() to affect geometry.
        void setUseSkirts( bool useSkirts ) { m_useSkirts = useSkirts; }
        bool getUseSkirts( void ) const { return m_useSkirts; }

        /// Extra runtime controls packed into cellData.oceanTime.yzw:
        ///  y = time scale, z = frequency scale, w = chaos
        void setWaveTimeScale( float timeScale ) { m_waveTimeScale = timeScale; }
        float getWaveTimeScale( void ) const { return m_waveTimeScale; }

        void setWaveFrequencyScale( float freqScale ) { m_waveFrequencyScale = freqScale; }
        float getWaveFrequencyScale( void ) const { return m_waveFrequencyScale; }

        void setWaveChaos( float chaos ) { m_waveChaos = chaos; }
        float getWaveChaos( void ) const { return m_waveChaos; }

        /// True when the active camera is below the ocean surface level (with small hysteresis).
        bool isUnderwater( void ) const { return m_isUnderwater; }

        /// True when the given world position is below the ocean base surface level.
        bool isUnderwater(const Vector3& worldPos) const;

        /// True when the given SceneNode's derived position is below the ocean base surface level.
        bool isUnderwater(const SceneNode * sceneNode) const;

        /// load must already have been called.
        void setDatablock( HlmsDatablock* datablock );

        void setCamera(Ogre::Camera* camera);

        //MovableObject overloads
        const String &getMovableType() const override;

        const Camera *getCamera() const { return m_camera; }
        void          setCamera( const Camera *camera ) { m_camera = camera; }

        /// Swaps current state with a saved one. Useful for rendering shadow maps
        void _swapSavedState(void);

        const Vector2& getXZDimensions(void) const      { return m_xzDimensions; }
        const Vector2& getXZInvDimensions(void) const   { return m_xzInvDimensions; }
        const Vector3& getOceanOrigin(void) const     { return m_OceanOrigin; }
    };

    // Factory so SceneManager::destroyMovableObject() works for type "Ocean"
    class OceanFactory : public MovableObjectFactory
    {
    public:
        static const String FACTORY_TYPE_NAME;

        const String& getType(void) const override;

    protected:
        MovableObject* createInstanceImpl(IdType id, ObjectMemoryManager* objectMemoryManager, SceneManager* manager, const NameValuePairList* params) override;

    public:
        void destroyInstance(MovableObject* obj) override;
    };
}

#endif
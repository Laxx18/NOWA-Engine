#ifndef PLANET_OCEAN_H
#define PLANET_OCEAN_H

#include "OgreItem.h"
#include "OgreMesh2.h"
#include "OgrePrerequisites.h"
#include "OgreVector2.h"
#include "OgreVector3.h"
#include "OgreVector4.h"

#include "defines.h"

namespace Ogre
{
    class SceneManager;
    class SceneNode;
    class VertexBufferPacked;
    class HlmsPbsDatablock;
    class StagingTexture;
    class TextureGpu;
}

namespace NOWA
{
    /**
     * @class PlanetOcean
     * @brief Spherical ocean mesh animated via CPU Gerstner waves.
     *
     * Vertex format: pos(3) + normal(3) + tangent(4) + uv(2) = 12 floats.
     * The vertex buffer is BT_DYNAMIC_DEFAULT and re-uploaded every render frame.
     *
     * Wave model: 4 configurable Gerstner waves. frequency = cycles per hemisphere
     * (radius-independent). amplitude calibrated for 50m reference planet, scales with radius.
     *
     * Normal map: procedurally generated 64x64 texture re-uploaded every frame with
     * time-varying phase offsets, giving animated water ripple appearance without
     * any PBS UV animation support. Frequencies scale with radius so physical wave
     * size is consistent regardless of planet scale.
     */
    class EXPORTED PlanetOcean
    {
    public:
        struct WaveParams
        {
            float directionAngle; // Propagation direction angle in radians (0 = +X axis)
            float amplitude;      // Crest height in metres on 50m reference planet
            float frequency;      // Cycles per hemisphere (radius-independent)
            float speed;          // Phase speed in radians per second
            float steepness;      // Reserved for future Gerstner Q term
        };

    public:
        PlanetOcean(const Ogre::String& objectName, Ogre::SceneManager* sceneManager);
        ~PlanetOcean();

        // ---- Lifecycle (render thread only) ----------------------------------

        bool create(float radius, int segmentsH, int segmentsV, Ogre::SceneNode* attachNode, const Ogre::String& datablockName);

        void destroy();

        /** Advance time, update geometry and normal map. Render thread only. */
        void update(float deltaTime);

        // ---- Material --------------------------------------------------------

        void setDatablockByName(const Ogre::String& name);
        void setDeepColour(const Ogre::Vector3& colour);
        void setShallowColour(const Ogre::Vector3& colour);
        void setRoughness(float roughness);
        void setTransparency(float transparency);
        void setReflectionTextureName(const Ogre::String& cubemapName);

        // ---- Wave configuration ----------------------------------------------

        void setWave(int index, const WaveParams& params);
        WaveParams getWave(int index) const;
        static int getWaveCount();

        void setWaveAmplitudeScale(float scale);
        float getWaveAmplitudeScale() const;

        // ---- Accessors -------------------------------------------------------

        Ogre::Item* getItem() const;
        Ogre::SceneNode* getAttachedNode() const;
        Ogre::SceneManager* getSceneManager() const;
        float getRadius() const;
        float getOceanTime() const;

        void setDynamic(bool dynamic);
    private:
        void generateBaseSphere();
        Ogre::Item* buildItem(const Ogre::String& meshName);

        /** Single-pass wave + sphere-normal + pack + geometry upload. */
        void updateAndUpload();

        /**
         * @brief Regenerates the 64x64 water normal map with time-varying phases
         *        and uploads to GPU. Called every frame from update().
         *        Frequencies are scaled by radius so physical wave size is constant.
         */
        void updateNormalMap();

        /**
         * @brief Creates the GPU texture and staging resources. Called once from create().
         *        Sets PBSM_NORMAL on the PBS datablock.
         */
        void initNormalMapTexture();

    private:
        Ogre::String objectName;
        Ogre::SceneManager* sceneManager;
        Ogre::SceneNode* attachedNode;
        Ogre::Item* oceanItem;
        Ogre::MeshPtr oceanMesh;

        Ogre::VertexBufferPacked* vertexBuffer;
        Ogre::TextureGpu* noiseTexture;
        float normalMapWeight;
        int noiseTexCounter; // Incremented each create() to guarantee unique texture name

        // Base sphere data computed once at create().
        std::vector<Ogre::Vector3> baseDirs;
        std::vector<Ogre::Vector2> uvCoords;
        std::vector<Ogre::Vector4> tangents;
        std::vector<uint32_t> indices;

        size_t vertexCount;
        size_t indexCount;

        std::array<WaveParams, 4> waves;
        float oceanTime;
        float waveAmplitudeScale;

        float radius;
        int segmentsH;
        int segmentsV;
    };

} // namespace NOWA

#endif // PLANET_OCEAN_H
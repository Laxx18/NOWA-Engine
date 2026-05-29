#ifndef PLANET_OCEAN_H
#define PLANET_OCEAN_H

#include "OgreItem.h"
#include "OgreMesh2.h"
#include "OgrePrerequisites.h"
#include "OgreVector2.h"
#include "OgreVector3.h"
#include "OgreVector4.h"

#include <array>
#include <string>
#include <vector>

#include "defines.h"

namespace Ogre
{
    class SceneManager;
    class SceneNode;
    class VertexBufferPacked;
    class HlmsPbsDatablock;
}

namespace NOWA
{
    /**
     * @class PlanetOcean
     * @brief Spherical ocean mesh animated via CPU Gerstner waves each frame.
     *
     * Vertex format: pos(3) + normal(3) + tangent(4) + uv(2) = 12 floats.
     * Uses BT_DYNAMIC_DEFAULT so wave displacement is re-uploaded every frame.
     *
     * Wave model: up to 4 Gerstner waves parameterised by direction, amplitude,
     * frequency and speed. Each vertex is displaced radially along its unit sphere
     * normal by the sum of all wave contributions.
     */
    class EXPORTED PlanetOcean
    {
    public:
        // One Gerstner-like wave definition.
        struct WaveParams
        {
            float directionAngle; // Propagation direction angle in radians (0 = +X axis)
            float amplitude;      // World-space height in metres
            float frequency;      // Spatial frequency (cycles per metre)
            float speed;          // Phase speed (metres per second)
            float steepness;      // Gerstner Q [0..1], 0 = sinusoidal
        };

    public:
        /**
         * @brief Constructor. Does not allocate GPU resources.
         * @param objectName Unique name used for the mesh and item.
         * @param sceneManager Scene manager that owns the item.
         */
        PlanetOcean(const Ogre::String& objectName, Ogre::SceneManager* sceneManager);

        ~PlanetOcean();

        // ---- Lifecycle --------------------------------------------------------

        /** Must be called on the render thread (inside enqueueAndWait). */
        bool create(float radius, int segmentsH, int segmentsV, Ogre::SceneNode* attachNode, const Ogre::String& datablockName);

        void destroy();

        /** Animate waves and upload new vertex data to GPU. Render thread only. */
        void update(float deltaTime);

        // ---- Material ---------------------------------------------------------

        void setDatablockByName(const Ogre::String& name);
        void setDeepColour(const Ogre::Vector3& colour);
        void setShallowColour(const Ogre::Vector3& colour);
        void setRoughness(float roughness);
        void setTransparency(float transparency);
        void setReflectionTextureName(const Ogre::String& cubemapName);

        // ---- Wave configuration -----------------------------------------------

        void setWave(int index, const WaveParams& params);
        WaveParams getWave(int index) const;

        static int getWaveCount();

        void setWaveAmplitudeScale(float scale);
        float getWaveAmplitudeScale() const;

        // ---- Accessors --------------------------------------------------------

        Ogre::Item* getItem() const;
        Ogre::SceneNode* getAttachedNode() const;
        Ogre::SceneManager* getSceneManager() const;
        float getRadius() const;
        float getOceanTime() const;

    private:
        void generateBaseSphere();
        void recalculateNormals();
        void recalculateTangents();
        Ogre::Item* buildItemFromCPUArrays(const Ogre::String& meshName);
        void applyWaves();
        void uploadVertexData();

    private:
        Ogre::String objectName;
        Ogre::SceneManager* sceneManager;
        Ogre::SceneNode* attachedNode;
        Ogre::Item* oceanItem;
        Ogre::MeshPtr oceanMesh;

        // Dynamic vertex buffer owned by the sub-mesh VAO.
        Ogre::VertexBufferPacked* vertexBuffer;

        // Base sphere geometry (CPU, never modified after create()).
        std::vector<Ogre::Vector3> baseDirs;
        std::vector<Ogre::Vector3> basePositions;
        std::vector<Ogre::Vector2> uvCoords;
        std::vector<uint32_t> indices;
        size_t vertexCount;
        size_t indexCount;

        // Animated data rebuilt every frame.
        std::vector<Ogre::Vector3> vertices;
        std::vector<Ogre::Vector3> normals;
        std::vector<Ogre::Vector4> tangents;

        std::array<WaveParams, 4> waves;
        float oceanTime;
        float waveAmplitudeScale;

        float radius;
        int segmentsH;
        int segmentsV;
    };

} // namespace NOWA

#endif // PLANET_OCEAN_H
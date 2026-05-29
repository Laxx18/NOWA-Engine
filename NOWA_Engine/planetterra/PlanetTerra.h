#ifndef PLANET_TERRA_H
#define PLANET_TERRA_H

#include "OgreItem.h"
#include "OgreMesh2.h"
#include "OgreMeshManager2.h"
#include "OgrePrerequisites.h"
#include "OgreSceneManager.h"
#include "OgreSceneNode.h"
#include "OgreStagingTexture.h"
#include "OgreTextureGpu.h"
#include "OgreVector2.h"
#include "OgreVector3.h"
#include "OgreVector4.h"
#include "Vao/OgreIndexBufferPacked.h"
#include "Vao/OgreVaoManager.h"
#include "Vao/OgreVertexBufferPacked.h"
#include "defines.h"

namespace NOWA
{
    /**
     * @brief Spherical planet terrain mesh with CPU-side height sculpting and blend-layer painting.
     *
     * Threading contract:
     *   Methods marked  // RENDER THREAD  must be called inside a GraphicsModule enqueueAndWait lambda.
     *   Methods marked  // MAIN THREAD    are safe to call from the main (logic) thread.
     *   No method is safe to call from both threads simultaneously.
     *
     * Typical usage (component drives this):
     *   // main thread
     *   planet = new PlanetTerra("MyPlanet", sceneManager);
     *   // render thread
     *   planet->create(500.f, 64, 64, 1024, sceneNode, "MyPlanetDatablock");
     *   // main thread per stroke
     *   planet->applyBrush(localHit, false, BrushMode::PULL, ...);
     *   // render thread after stroke
     *   planet->uploadVertexData();
     */
    class EXPORTED PlanetTerra
    {
    public:
        enum class BrushMode
        {
            PUSH,    // lower terrain (crater / valley)
            PULL,    // raise terrain (hill / mountain)
            SMOOTH,  // average with neighbors
            FLATTEN, // pull toward average height in brush radius
            INFLATE  // push radially outward (uniform puff)
        };

    public:
        explicit PlanetTerra(const Ogre::String& objectName, Ogre::SceneManager* sceneManager);
        ~PlanetTerra();

        // ── Creation / destruction (RENDER THREAD) ─────────────────────────────
        bool create(float radius, int segH, int segV, int blendTexSize, Ogre::SceneNode* attachNode, const Ogre::String& datablockName);
        void destroy();

        // ── Full topology rebuild when seg-count changes (RENDER THREAD) ───────
        void rebuildDynamicBuffers();

        // ── Fast GPU upload without topology change (RENDER THREAD) ────────────
        // Call after any CPU-side vertex modification.
        void uploadVertexData();

        // ── Blend texture GPU upload (RENDER THREAD) ───────────────────────────
        void uploadBlendData();

        // ── Datablock live-swap (RENDER THREAD) ────────────────────────────────
        void setDatablockByName(const Ogre::String& name);

        // ── CPU geometry (MAIN THREAD) ─────────────────────────────────────────
        // Recomputes vertices[] from baseDirs[] + heightData[].
        // Always call this after modifying heightData[], then recalculateNormals/Tangents.
        void rebuildGeometryFromHeight();
        void recalculateNormals();
        void recalculateTangents();
        void buildVertexAdjacency();

        // ── Sculpting brush (MAIN THREAD) ──────────────────────────────────────
        // hitPosLocal: ray intersection in the planet's local space.
        // brushData: flattened brush image (row-major, 0..1 per pixel).
        // After this call, invoke uploadVertexData() from render thread.
        void applyBrush(const Ogre::Vector3& hitPosLocal, bool invert, BrushMode mode, const std::vector<float>& brushData, int brushW, int brushH, float brushSize, float brushIntensity, float brushFalloff);

        // ── Painting brush (MAIN THREAD) ───────────────────────────────────────
        // hitUV: UV coordinate (0..1) of the intersection.
        // layer: 0..3 (maps to R,G,B,A of blendData RGBA8 texture).
        // brushSize is in blend-texture pixels.
        // After this call, invoke uploadBlendData() from render thread.
        void applyPaintBrush(const Ogre::Vector2& hitUV, bool invert, int layer, const std::vector<float>& brushData, int brushW, int brushH, float brushSize, float brushIntensity, float brushFalloff);

        // ── Snapshot / undo support (MAIN THREAD) ──────────────────────────────
        std::vector<float> getHeightDataCopy() const
        {
            return heightData;
        }

        std::vector<uint8_t> getBlendDataCopy() const
        {
            return blendData;
        }

        // Restores data and rebuilds geometry; call uploadVertexData / uploadBlendData afterward.
        void restoreHeightData(const std::vector<float>& data);
        void restoreBlendData(const std::vector<uint8_t>& data);

        // ── Accessors (thread-safe reads of immutable values) ──────────────────
        Ogre::Item* getItem() const
        {
            return planetItem;
        }

        Ogre::MeshPtr getMesh() const
        {
            return planetMesh;
        }

        Ogre::TextureGpu* getBlendTex() const
        {
            return blendWeightTex;
        }

        // Returns the Ogre texture name used for the blend weight texture.
        // Includes the .png extension so DatablockPbsComponent can find it
        // via createOrRetrieveTexture after the file has been saved to disk.
        Ogre::String getBlendTextureName() const
        {
            return this->objectName + "_blendWeight";
        }

        // Saves the CPU blendData array as a PNG file to the given file path.
        // Call before DatablockPbsComponent::postInit so the file exists on disk
        // when resourceExistsInAnyGroup is checked. Also call on mouseReleased
        // for persistence across sessions.
        bool saveBlendDataToFile(const Ogre::String& filePathName) const;
        const std::vector<Ogre::Vector3>& getVertices() const
        {
            return vertices;
        }
        const std::vector<Ogre::uint32>& getIndices() const
        {
            return indices;
        }
        size_t getVertexCount() const
        {
            return vertexCount;
        }
        size_t getIndexCount() const
        {
            return indexCount;
        }
        float getRadius() const
        {
            return radius;
        }
        int getSegmentsH() const
        {
            return segmentsH;
        }
        int getSegmentsV() const
        {
            return segmentsV;
        }
        int getBlendTexSize() const
        {
            return blendTexSize;
        }
        float getComputedMaxRadius() const
        {
            return computedMaxRadius;
        }

        // Sets the UV tiling scale for UV set 1 (used by main diffuse and normal textures).
        // UV1 = UV0 * baseUVScale. Call before create() or after rebuildDynamicBuffers().
        // Default 1.0f = tiles once across the sphere.
        void setBaseUVScale(float scale)
        {
            this->baseUVScale = std::max(0.01f, scale);
        }

        float getBaseUVScale() const
        {
            return this->baseUVScale;
        }

        const std::vector<Ogre::Vector2>& getUvCoords(void) const;

    private:
        // Internal CPU helpers (MAIN THREAD)
        void generateBaseSphere();
        void buildPositionGroups();
        float sampleBrushImage(const std::vector<float>& brushData, int brushW, int brushH, float ndx, float ndz) const;

        // Internal GPU helpers (RENDER THREAD)
        void createBlendWeightTexture();
        void destroyBlendWeightTexture();

        // Shared creation of VAO + SubMesh + Item from current CPU arrays.
        // Returns the new Item (does NOT swap/attach — caller does that).
        Ogre::Item* buildItemFromCPUArrays(const Ogre::String& meshName);

    private:
        Ogre::String objectName;
        Ogre::SceneManager* sceneManager = nullptr;
        Ogre::SceneNode* attachedNode = nullptr;

        // Sphere configuration
        float radius = 500.0f;
        int segmentsH = 64;
        int segmentsV = 64;
        int blendTexSize = 1024;

        // CPU vertex data (main-thread owner)
        std::vector<Ogre::Vector3> baseDirs; // unit-sphere directions — constant per topology
        std::vector<Ogre::Vector3> vertices; // baseDirs * (radius + heightData)
        std::vector<Ogre::Vector3> normals;
        std::vector<Ogre::Vector4> tangents;
        std::vector<Ogre::Vector2> uvCoords;
        std::vector<Ogre::uint32> indices;
        std::vector<float> heightData;  // per-vertex height above sphere surface
        std::vector<uint8_t> blendData; // blendTexSize*blendTexSize*4 (RGBA8, layers 0-3)

        // Adjacency (main-thread)
        std::vector<std::vector<size_t>> vertexNeighbors;
        std::vector<std::vector<size_t>> positionGroups;
        std::vector<int> vertexGroup; // -1 = lone vertex

        size_t vertexCount = 0u;
        size_t indexCount = 0u;
        float computedMaxRadius = 500.0f; // updated in rebuildGeometryFromHeight

        // GPU objects (render-thread only)
        Ogre::MeshPtr planetMesh;
        Ogre::Item* planetItem = nullptr;
        Ogre::VertexBufferPacked* dynamicVB = nullptr;
        Ogre::TextureGpu* blendWeightTex = nullptr;
        Ogre::StagingTexture* blendStagingTex = nullptr;

        uint32_t rebuildCounter = 0u;

        // UV scale applied to UV set 1 (main diffuse + normal).
        // UV1 = UV0 * baseUVScale. Stored so rebuildDynamicBuffers can reapply it.
        float baseUVScale = 1.0f;
    };

} // namespace NOWA

#endif
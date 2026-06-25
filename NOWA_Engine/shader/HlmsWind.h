#ifndef HLMS_WIND_H
#define HLMS_WIND_H

#include "CommandBuffer/OgreCbTexture.h"
#include "CommandBuffer/OgreCommandBuffer.h"
#include "OgreHlmsListener.h"
#include "OgreHlmsPbs.h"
#include "defines.h"

#include <mutex>

namespace NOWA
{
    /**
     * @brief Maximum number of simultaneous wind interactors uploaded to the GPU each frame.
     *
     * Each slot maps to one WindInteractionComponent in the scene. The array always occupies
     * exactly WIND_MAX_INTERACTORS * 16 bytes in the pass constant buffer regardless of how many
     * components are actually active (unused slots are zeroed). Change this constant and recompile
     * both engine and shaders to extend the limit.
     */
    static const int WIND_MAX_INTERACTORS = 8;

    /**
     * @brief POD describing a single wind interactor in world space.
     *
     * Populated each logic frame by WindInteractionComponent::update() and handed to
     * HlmsWind::addInteractorPending(). Uploaded verbatim into the GPU pass buffer by
     * HlmsWindListener::preparePassBuffer() every render frame.
     */
    struct WindInteractor
    {
        float worldX = 0.0f;   ///< World-space X position of the interactor.
        float worldZ = 0.0f;   ///< World-space Z position of the interactor.
        float radius = 0.0f;   ///< Radius of influence in world units.
        float strength = 0.0f; ///< Push strength multiplier (0 = no effect).
    };

    // -------------------------------------------------------------------------

    /**
     * @brief Internal Ogre::HlmsListener that writes per-pass wind uniforms.
     *
     * Owned exclusively by HlmsWind. All public methods fall into two groups that
     * must be respected to avoid data races:
     *
     * **Logic-thread API** (called from game update, physics tick, component update):
     *   - addTime(), setWindStrength(), setWindDirection(), setWindFrequency()
     *   - addInteractorPending(), clearInteractorsPending()
     *
     * These write into @c pendingInteractors / @c pendingParams, which are only
     * ever read by commitPendingToRender() — see below.
     *
     * **Render-thread API** (called from GraphicsModule render loop or closure):
     *   - commitPendingToRender() — copies the latest logic-thread snapshot into
     *     the render-thread-owned @c renderInteractors / @c renderParams. Must be
     *     called once per frame, before Ogre::Root::renderOneFrame().
     *
     * **Ogre internal callbacks** (render thread, called by Ogre during renderOneFrame):
     *   - getPassBufferSize(), preparePassBuffer() — read @c renderInteractors /
     *     @c renderParams only. Never touch pending state.
     *
     * The separation guarantees that the logic thread and the Ogre render callbacks
     * never touch the same memory simultaneously, eliminating the torn-read jitter
     * that occurred when both sides accessed activeInteractors[] directly.
     */
    class EXPORTED HlmsWindListener : public Ogre::HlmsListener
    {
    public:
        HlmsWindListener();
        virtual ~HlmsWindListener() = default;

        // -----------------------------------------------------------------
        // Logic-thread API — write to pending state only.
        // Safe to call from any logic/physics/component update tick.
        // -----------------------------------------------------------------

        /**
         * @brief Sets the absolute wind clock to @p time seconds.
         * @note Logic thread.
         */
        void setTime(Ogre::Real time);

        /**
         * @brief Advances the wind clock by @p dt seconds. Typically called
         *        once per render frame from WorkspaceBaseComponent's render closure.
         * @note Logic thread (or render-thread closure — addTime is the one
         *       exception that is already called inside a render closure in
         *       WorkspaceBaseComponent, so it is safe either way since the
         *       closure runs on the render thread before commitPendingToRender).
         */
        void addTime(Ogre::Real dt);

        /** @brief Sets wind sway amplitude. @note Logic thread. */
        void setWindStrength(Ogre::Real strength);

        /** @brief Returns the current pending wind strength. @note Logic thread. */
        Ogre::Real getWindStrength() const;

        /** @brief Sets normalised wind direction vector. @note Logic thread. */
        void setWindDirection(const Ogre::Vector3& direction);

        /** @brief Returns the current pending wind direction. @note Logic thread. */
        Ogre::Vector3 getWindDirection() const;

        /** @brief Sets the spatial frequency of the Perlin noise scroll. @note Logic thread. */
        void setWindFrequency(Ogre::Real frequency);

        /** @brief Returns the current pending wind frequency. @note Logic thread. */
        Ogre::Real getWindFrequency() const;

        int acquireInteractorSlot(void);

        void releaseInteractorSlot(int slot);

        void updateInteractorSlot(int slot, float worldX, float worldZ, float radius, float strength);


        // -----------------------------------------------------------------
        // Render-thread API
        // -----------------------------------------------------------------

        /**
         * @brief Atomically copies the pending snapshot into the render-side buffers.
         *
         * Must be called **once per render frame**, from the render thread, **before**
         * Ogre::Root::renderOneFrame(). In NOWA-Engine this belongs in the
         * GraphicsModule render loop or in a tracked render closure that is guaranteed
         * to execute before Ogre starts issuing draw calls.
         *
         * After this call, getPassBufferSize() and preparePassBuffer() will see the
         * data that was written by the logic thread during the previous logic tick.
         *
         * @note Render thread only.
         */
        void commitPendingToRender();

        // -----------------------------------------------------------------
        // Ogre HlmsListener callbacks — render thread, called by Ogre internally.
        // Read renderParams / renderInteractors only.
        // -----------------------------------------------------------------

        /**
         * @brief Returns the number of bytes this listener appends to the pass buffer.
         * @note Render thread (called by Ogre during pass preparation).
         */
        virtual Ogre::uint32 getPassBufferSize(const Ogre::CompositorShadowNode* shadowNode, bool casterPass, bool dualParaboloid, Ogre::SceneManager* sceneManager) const override;

        /**
         * @brief Writes wind uniforms into the pass constant buffer.
         *
         * Layout (all non-caster passes):
         * - float4 0 : windStrength, globalTime, windFrequency, pad
         * - float4 1 : windDirection.xyz, pad
         * - float4 2 : activeInteractorCount, pad, pad, pad
         * - float4 3..3+WIND_MAX_INTERACTORS-1 : worldX, worldZ, radius, strength
         *
         * @note Render thread (called by Ogre during pass preparation).
         */
        virtual float* preparePassBuffer(const Ogre::CompositorShadowNode* shadowNode, bool casterPass, bool dualParaboloid, Ogre::SceneManager* sceneManager, float* passBufferPtr) override;

    private:
        // -----------------------------------------------------------------
        // Pending state — written by the logic thread, read only by
        // commitPendingToRender(). Both sides must hold pendingMutex while
        // touching ANY of pendingParams / pendingInteractors / pendingInteractorCount
        // / slotOccupied, since commitPendingToRender() copies several of these
        // fields together and a partial (torn) copy across two different logic
        // frames would hand the GPU an internally inconsistent snapshot (e.g. a
        // new windDirection.x mixed with an old .y/.z, or a strength from one
        // frame paired with a direction from another). Critical sections here are
        // a handful of float/bool writes — negligible cost, called a few times per
        // frame, never per-vertex.
        // -----------------------------------------------------------------

        mutable std::mutex pendingMutex;

        /// Wind scalar parameters staged for next commit.
        struct WindParams
        {
            Ogre::Real windStrength = 0.5f;
            Ogre::Real globalTime = 0.0f;
            Ogre::Real windFrequency = 1.0f;
            Ogre::Vector3 windDirection{1.0f, 0.0f, 0.0f};
        };

        WindParams pendingParams;
        WindInteractor pendingInteractors[WIND_MAX_INTERACTORS];
        int pendingInteractorCount = 0;
        bool slotOccupied[WIND_MAX_INTERACTORS];

        // -----------------------------------------------------------------
        // Render state — written only by commitPendingToRender() on the render
        // thread, read only by getPassBufferSize() / preparePassBuffer() also on
        // the render thread. Never touched by the logic thread, so no locking
        // is needed here.
        // -----------------------------------------------------------------

        WindParams renderParams;
        WindInteractor renderInteractors[WIND_MAX_INTERACTORS];
        int renderInteractorCount = 0;
    };

    // =========================================================================

    class EXPORTED HlmsWind : public Ogre::HlmsPbs
    {
    public:
        /**
         * @brief Constructs HlmsWind and registers the internal HlmsWindListener.
         *
         * Ogre calls the listener's callbacks on the render thread during pass
         * preparation. The listener is a member of this object and must therefore
         * outlive any active render frames — satisfied because HlmsWind is owned
         * by the HlmsManager for the engine lifetime.
         */
        HlmsWind(Ogre::Archive* dataFolder, Ogre::ArchiveVec* libraryFolders);

        virtual ~HlmsWind() = default;

        // -----------------------------------------------------------------
        // Setup / teardown — must be called via enqueueAndWait (render thread).
        // -----------------------------------------------------------------

        /**
         * @brief Loads the 3-D Perlin noise texture and creates the sampler block.
         *
         * Called once when the workspace / scene manager becomes available.
         * Internally calls loadTexturesAndSamplers().
         *
         * @param sceneManager  Active scene manager whose render system owns the
         *                      TextureGpuManager.
         * @note Render thread (call via enqueueAndWait).
         */
        void setup(Ogre::SceneManager* sceneManager);

        /**
         * @brief Destroys the noise texture and sampler block.
         *
         * Must be called before the scene manager is destroyed.
         *
         * @param sceneManager  Same scene manager passed to setup().
         * @note Render thread (call via enqueueAndWait).
         */
        void shutdown(Ogre::SceneManager* sceneManager);

        // -----------------------------------------------------------------
        // Logic-thread wind parameter API.
        // Delegates to HlmsWindListener pending state.
        // -----------------------------------------------------------------

        /** @brief Advances the wind animation clock. @note Logic thread. */
        void addTime(float dt);

        /** @brief Sets wind sway amplitude [0, 1]. @note Logic thread. */
        void setWindStrength(float strength);

        /** @brief Returns the current pending wind strength. @note Logic thread. */
        float getWindStrength() const;

        /** @brief Sets the normalised wind direction vector. @note Logic thread. */
        void setWindDirection(const Ogre::Vector3& direction);

        /** @brief Returns the current pending wind direction. @note Logic thread. */
        Ogre::Vector3 getWindDirection() const;

        /** @brief Sets Perlin noise scroll frequency. @note Logic thread. */
        void setWindFrequency(float frequency);

        /** @brief Returns the current pending wind frequency. @note Logic thread. */
        float getWindFrequency(void) const;

        int acquireInteractorSlot(void);

        void releaseInteractorSlot(int slot);

        void updateInteractorSlot(int slot, float worldX, float worldZ, float radius, float strength);

        /**
         * @brief Copies pending interactor and parameter state to the render-side buffers.
         *
         * Must be called once per render frame on the render thread, strictly before
         * Ogre::Root::renderOneFrame(). In NOWA-Engine, place this call at the top of
         * the GraphicsModule render loop, after processCommandQueue() and before
         * renderOneFrame().
         *
         * @note Render thread.
         */
        void commitPendingToRender();

        // -----------------------------------------------------------------
        // Ogre overrides (render thread, called internally by Ogre).
        // -----------------------------------------------------------------

        /** @copydoc Ogre::HlmsPbs::notifyPropertiesMergedPreGenerationStep */
        PropertiesMergeStatus notifyPropertiesMergedPreGenerationStep(size_t tid, Ogre::PiecesMap* inOutPieces) override;

        /** @copydoc Ogre::HlmsPbs::fillBuffersForV1 */
        Ogre::uint32 fillBuffersForV1(const Ogre::HlmsCache* cache, const Ogre::QueuedRenderable& queuedRenderable, bool casterPass, Ogre::uint32 lastCacheHash, Ogre::CommandBuffer* commandBuffer) override;

        /** @copydoc Ogre::HlmsPbs::fillBuffersForV2 */
        Ogre::uint32 fillBuffersForV2(const Ogre::HlmsCache* cache, const Ogre::QueuedRenderable& queuedRenderable, bool casterPass, Ogre::uint32 lastCacheHash, Ogre::CommandBuffer* commandBuffer) override;

        /**
         * @brief Shared implementation for V1 and V2 fill-buffers paths.
         *
         * Binds the Perlin noise texture to slot 14 before delegating to
         * HlmsPbs::fillBuffersFor().
         */
        Ogre::uint32 fillBuffersFor(const Ogre::HlmsCache* cache, const Ogre::QueuedRenderable& queuedRenderable, bool casterPass, Ogre::uint32 lastCacheHash, Ogre::CommandBuffer* commandBuffer, bool isV1);

        // -----------------------------------------------------------------
        // Static helpers
        // -----------------------------------------------------------------

        /**
         * @brief Returns the shader archive paths required by HlmsWind.
         *
         * Called by Core::registerHlms() to construct the archive list passed to
         * the HlmsWind constructor. Mirrors the pattern used by HlmsPbs::getDefaultPaths().
         *
         * @param outDataFolderPath         Receives the main shader folder path.
         * @param outLibraryFoldersPaths    Receives the additional library paths.
         */
        static void getDefaultPaths(Ogre::String& outDataFolderPath, Ogre::StringVector& outLibraryFoldersPaths);

    private:
        virtual void calculateHashForPreCreate(Ogre::Renderable* renderable, Ogre::PiecesMap* inOutPieces) override;

        virtual void calculateHashForPreCaster(Ogre::Renderable* renderable, Ogre::PiecesMap* inOutPieces, const Ogre::PiecesMap* normalPassPieces) override;

        /**
         * @brief Creates the Perlin noise TextureGpu and HlmsSamplerblock.
         * @note Render thread.
         */
        void loadTexturesAndSamplers(Ogre::SceneManager* sceneManager);

    private:
        HlmsWindListener windListener;                   ///< Owned listener instance.
        Ogre::HlmsSamplerblock const* noiseSamplerBlock; ///< Sampler for the noise texture.
        Ogre::TextureGpu* noiseTexture;                  ///< 3-D Perlin noise volume.
    };

} // namespace NOWA

#endif // HLMS_WIND_H
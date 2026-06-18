#ifndef HLMS_WIND_H
#define HLMS_WIND_H

#include "CommandBuffer/OgreCbTexture.h"
#include "CommandBuffer/OgreCommandBuffer.h"
#include "OgreHlmsListener.h"
#include "OgreHlmsPbs.h"
#include "defines.h"

namespace NOWA
{
    // Maximum number of simultaneous interactors (players/objects pushing foliage).
    // Matches the interactors[4] array in 500_Structs_piece_vs_piece_ps.any.
    // Increasing this requires changing the shader struct AND the pass buffer size.
    static const int WIND_MAX_INTERACTORS = 4;

    /**
     * @brief One interactor slot: a world-space XZ position with a radius of
     *        influence and a push strength. The vertex shader pushes foliage
     *        vertices away from this position with quadratic falloff.
     */
    struct WindInteractor
    {
        float worldX;   // World space X position of the interactor
        float worldZ;   // World space Z position of the interactor
        float radius;   // Radius of influence in world units
        float strength; // Peak push magnitude at centre (e.g. 0.5 - 2.0)
    };

    class EXPORTED HlmsWindListener : public Ogre::HlmsListener
    {
    public:
        HlmsWindListener();

        virtual ~HlmsWindListener() = default;

        void setTime(Ogre::Real time);

        void addTime(Ogre::Real time);

        /**
         * @brief Replaces the active interactor list for this frame.
         *        Call once per frame from the logic thread before rendering.
         *        The list is copied immediately so the caller does not need to
         *        keep it alive past this call.
         * @param[in] interactors Active interactors, at most WIND_MAX_INTERACTORS.
         *        Excess entries beyond WIND_MAX_INTERACTORS are silently ignored.
         */
        void setInteractors(const std::vector<WindInteractor>& interactors);

        /**
         * @brief Clears all active interactors (no foliage push this frame).
         */
        void clearInteractors();

        virtual Ogre::uint32 getPassBufferSize(const Ogre::CompositorShadowNode* shadowNode, bool casterPass, bool dualParaboloid, Ogre::SceneManager* sceneManager) const;

        virtual float* preparePassBuffer(const Ogre::CompositorShadowNode* shadowNode, bool casterPass, bool dualParaboloid, Ogre::SceneManager* sceneManager, float* passBufferPtr);

    private:
        Ogre::Real windStrength;
        Ogre::Real globalTime;

        // Active interactors for the current frame.
        // Written from the logic thread via setInteractors(), read from the
        // render thread in preparePassBuffer(). Access is safe because
        // preparePassBuffer is called after the logic thread has submitted
        // its frame (same ordering as WindComponent::update -> addTime).
        WindInteractor activeInteractors[WIND_MAX_INTERACTORS];
        int activeInteractorCount;
    };

    //////////////////////////////////////////////////////////////////////////////

    class EXPORTED HlmsWind : public Ogre::HlmsPbs
    {
    public:
        HlmsWind(Ogre::Archive* dataFolder, Ogre::ArchiveVec* libraryFolders);

        virtual ~HlmsWind() = default;

        void addTime(float time);

        void setup(Ogre::SceneManager* sceneManager);

        void shutdown(Ogre::SceneManager* sceneManager);

        /**
         * @brief Sets the active interactor list for this frame.
         *        Forwards directly to HlmsWindListener::setInteractors.
         *        Call from the logic thread (e.g. ProceduralFoliageVolumeComponent
         *        updateTrackedClosure or a dedicated WindInteractionComponent).
         */
        void setInteractors(const std::vector<WindInteractor>& interactors);

        /**
         * @brief Clears all active interactors.
         */
        void clearInteractors();

        PropertiesMergeStatus notifyPropertiesMergedPreGenerationStep(size_t tid, Ogre::PiecesMap* inOutPieces);

        Ogre::uint32 fillBuffersForV1(const Ogre::HlmsCache* cache, const Ogre::QueuedRenderable& queuedRenderable, bool casterPass, Ogre::uint32 lastCacheHash, Ogre::CommandBuffer* commandBuffer);

        Ogre::uint32 fillBuffersForV2(const Ogre::HlmsCache* cache, const Ogre::QueuedRenderable& queuedRenderable, bool casterPass, Ogre::uint32 lastCacheHash, Ogre::CommandBuffer* commandBuffer);

        Ogre::uint32 fillBuffersFor(const Ogre::HlmsCache* cache, const Ogre::QueuedRenderable& queuedRenderable, bool casterPass, Ogre::uint32 lastCacheHash, Ogre::CommandBuffer* commandBuffer, bool isV1);

    public:
        static void getDefaultPaths(Ogre::String& outDataFolderPath, Ogre::StringVector& outLibraryFoldersPaths);

    private:
        void calculateHashForPreCreate(Ogre::Renderable* renderable, Ogre::PiecesMap* inOutPieces) override;

        void loadTexturesAndSamplers(Ogre::SceneManager* sceneManager);

    private:
        HlmsWindListener windListener;

        Ogre::HlmsSamplerblock const* noiseSamplerBlock;

        Ogre::TextureGpu* noiseTexture;
    };

}; // namespace end

#endif
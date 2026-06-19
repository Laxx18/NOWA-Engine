#ifndef HLMS_WIND_H
#define HLMS_WIND_H

#include "CommandBuffer/OgreCbTexture.h"
#include "CommandBuffer/OgreCommandBuffer.h"
#include "OgreHlmsListener.h"
#include "OgreHlmsPbs.h"
#include "defines.h"

namespace NOWA
{
    // Maximum simultaneous interactors. Matches interactors[8] in the shader struct.
    // Adapt this variable and compile to have your max number. Issue: it always then occupies so many slots, no matter whether there are as many WindInteractionComponent or not! (Performance, but cheap).
    static const int WIND_MAX_INTERACTORS = 8;

    // One interactor slot: world XZ position, radius of influence, push strength.
    // Set each frame by WindInteractionComponent::update via HlmsWind::setInteractors.
    struct WindInteractor
    {
        float worldX;
        float worldZ;
        float radius;
        float strength;
    };

    class EXPORTED HlmsWindListener : public Ogre::HlmsListener
    {
    public:
        HlmsWindListener();

        virtual ~HlmsWindListener() = default;

        void setTime(Ogre::Real time);
        void addTime(Ogre::Real time);

        void setWindStrength(Ogre::Real strength);
        Ogre::Real getWindStrength() const;

        void setWindDirection(const Ogre::Vector3& direction);
        const Ogre::Vector3& getWindDirection() const;

        void setWindFrequency(Ogre::Real frequency);
        Ogre::Real getWindFrequency() const;

        // New: interactor support for WindInteractionComponent.
        // Does not affect any of the existing wind parameters above.
        void setInteractors(const std::vector<WindInteractor>& interactors);
        void clearInteractors();
        // Append one interactor slot -- called once per WindInteractionComponent per frame.
        void addInteractor(float worldX, float worldZ, float radius, float strength);

        virtual Ogre::uint32 getPassBufferSize(const Ogre::CompositorShadowNode* shadowNode, bool casterPass, bool dualParaboloid, Ogre::SceneManager* sceneManager) const;

        virtual float* preparePassBuffer(const Ogre::CompositorShadowNode* shadowNode, bool casterPass, bool dualParaboloid, Ogre::SceneManager* sceneManager, float* passBufferPtr);

    private:
        // Existing members -- unchanged
        Ogre::Real windStrength;
        Ogre::Real globalTime;
        Ogre::Real windFrequency;
        Ogre::Vector3 windDirection;

        // New members for interactor support
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

        void setWindStrength(float strength);
        float getWindStrength() const;

        void setWindDirection(const Ogre::Vector3& direction);
        const Ogre::Vector3& getWindDirection() const;

        void setWindFrequency(float frequency);
        float getWindFrequency() const;

        void setup(Ogre::SceneManager* sceneManager);

        void shutdown(Ogre::SceneManager* sceneManager);

        // New: interactor support for WindInteractionComponent.
        // Does not affect setWindStrength/setWindDirection/setWindFrequency/addTime.
        void setInteractors(const std::vector<WindInteractor>& interactors);
        void clearInteractors();
        void addInteractor(float worldX, float worldZ, float radius, float strength);

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
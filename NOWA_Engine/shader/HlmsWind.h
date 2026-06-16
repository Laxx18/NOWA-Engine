#ifndef HLMS_WIND_H
#define HLMS_WIND_H

#include "CommandBuffer/OgreCbTexture.h"
#include "CommandBuffer/OgreCommandBuffer.h"
#include "OgreHlmsListener.h"
#include "OgreHlmsPbs.h"
#include "defines.h"

namespace NOWA
{
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

        virtual Ogre::uint32 getPassBufferSize(const Ogre::CompositorShadowNode* shadowNode, bool casterPass, bool dualParaboloid, Ogre::SceneManager* sceneManager) const;

        virtual float* preparePassBuffer(const Ogre::CompositorShadowNode* shadowNode, bool casterPass, bool dualParaboloid, Ogre::SceneManager* sceneManager, float* passBufferPtr);

    private:
        Ogre::Real windStrength;
        Ogre::Real globalTime;
        Ogre::Real windFrequency;
        Ogre::Vector3 windDirection;
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